#include "bh_renderer.h"
#include "bh_render_startup.h"
#include <string.h>
#include <stdio.h>

static void extents(BHRenderStartup *s)
{
    s->extent_x = (float)s->width;
    s->extent_y = (float)s->height;
    s->clip_x = s->extent_x;
    s->clip_y = s->extent_y;
    s->scale_x = s->scale_y = 0;
    s->offset_x = s->offset_y = 0;
}
static int format(BHRenderStartup *s, BHRenderSurface *surface, unsigned long width, unsigned long height,
                  unsigned long bits, int software)
{
    if (!s || !surface || !width || !height || width > 0x7fffffffUL || height > 0x7fffffffUL ||
        (bits != 8 && bits != 16 && bits != 24 && bits != 32) || s->depth || s->render || s->textures.draw)
        return 0;
    s->width = width;
    s->height = height;
    s->bits = bits;
    s->software = software;
    s->bytes_per_pixel = bits == 8 ? 1 : bits == 16 ? 2 : 4;
    memset(&s->pixel, 0, sizeof(s->pixel));
    s->pixel.dwSize = sizeof(s->pixel);
    /* The original ignores GetPixelFormat failure; source guards retain the
       error rather than continuing with an unspecified display format. */
    s->last_hresult = bh_renderer_surface_format(surface, &s->pixel);
    return s->last_hresult == DD_OK;
}
void bh_render_startup_dispose(BHRenderStartup *s)
{
    unsigned long i;
    HRESULT hr;
    int retained = 0;
    if (!s)
        return;
    bh_software_tables_dispose(&s->tables);
    bh_texture_context_dispose(&s->textures);
    if (s->device)
    {
        bh_renderer_device_release(s->device);
        s->device = NULL;
    }
    if (s->renderer)
    {
        bh_renderer_release(s->renderer);
        s->renderer = NULL;
    }
    for (i = 0; i < 2; ++i)
        if (s->depth_attachments[i])
        {
            hr = bh_renderer_surface_detach_depth(s->depth_attachments[i], s->depth);
            s->last_detach_hresult = hr;
            if (hr != DD_OK && hr != DDERR_SURFACENOTATTACHED)
            {
                ++s->detach_failures;
                retained = 1;
                continue;
            }
            bh_renderer_surface_release(s->depth_attachments[i]);
            s->depth_attachments[i] = NULL;
            --s->depth_attachment_count;
        }
    if (retained)
        return;
    if (s->depth)
        bh_renderer_surface_release(s->depth);
    if (s->palette)
        bh_renderer_palette_release(s->palette);
    if (s->render)
        bh_renderer_surface_release(s->render);
    if (s->screen)
        bh_renderer_surface_release(s->screen);
    memset(s, 0, sizeof(*s));
}
HRESULT bh_render_startup_attach_depth(BHRenderStartup *s, BHRenderSurface *target)
{
    unsigned long i;
    HRESULT hr;
    if (!s || !s->depth || !target)
        return DDERR_INVALIDPARAMS;
    for (i = 0; i < 2 && s->depth_attachments[i]; ++i)
    {
    }
    if (i == 2)
        return DDERR_INVALIDPARAMS;
    hr = bh_renderer_surface_attach_depth(target, s->depth);
    if (hr == DD_OK)
    {
        bh_renderer_surface_retain(target);
        s->depth_attachments[i] = target;
        ++s->depth_attachment_count;
    }
    return hr;
}
static int failed(BHRenderStartup *s)
{
    HRESULT hr = s->last_hresult;
    bh_render_startup_dispose(s);
    s->last_hresult = hr;
    return 0;
}
int bh_render_startup_hardware(BHRenderStartup *s, BHRenderer *draw, BHRenderSurface *render, BHRenderSurface *screen,
                               unsigned long width, unsigned long height, unsigned long bits)
{
    DDSURFACEDESC depth;
    if (!draw || !format(s, render, width, height, bits, 0))
        return 0;
    if (s->bytes_per_pixel < 2)
        return 0;
    memset(&depth, 0, sizeof(depth));
    depth.dwSize = sizeof(depth);
    depth.dwFlags = 0x47;
    depth.dwWidth = width;
    depth.dwHeight = height;
    depth.dwZBufferBitDepth = 16;
    depth.ddsCaps.dwCaps = 0x24000;
    s->last_hresult = bh_renderer_surface_create(draw, &depth, &s->depth);
    if (s->last_hresult != DD_OK)
        return failed(s);
    if (screen)
        bh_render_startup_attach_depth(s, screen);
    bh_render_startup_attach_depth(s, render);
    if (!s->renderer)
    {
        s->renderer = draw;
        bh_renderer_retain(draw);
    }
    s->last_hresult = bh_renderer_device_create(s->renderer, render, &s->device);
    if (s->last_hresult != DD_OK)
        return failed(s);
    bh_renderer_surface_retain(render);
    s->render = render;
    if (screen)
    {
        bh_renderer_surface_retain(screen);
        s->screen = screen;
    }
    extents(s);
    if (!bh_texture_context_init(&s->textures, draw, s->device, NULL) || !bh_texture_formats(&s->textures))
        return failed(s);
    return 1;
}
int bh_render_startup_software(BHRenderStartup *s, BHRenderer *draw, BHRenderSurface *format_surface,
                               BHRenderSurface *render, BHRenderSurface *screen, unsigned long width,
                               unsigned long height, unsigned long bits)
{
    DDSURFACEDESC desc;
    PALETTEENTRY palette[256];
    unsigned long i, masks[3];
    if (!format_surface)
        format_surface = render ? render : screen;
    if (!draw || !format(s, format_surface, width, height, bits, 1))
        return 0;
    if (s->bytes_per_pixel > 2)
        return 0;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x47;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.dwZBufferBitDepth = 16;
    desc.ddsCaps.dwCaps = 0x20800;
    s->last_hresult = bh_renderer_surface_create(draw, &desc, &s->depth);
    if (s->last_hresult != DD_OK)
        return failed(s);
    if (!render)
    {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        desc.dwFlags = 0x1007;
        desc.dwWidth = width;
        desc.dwHeight = height;
        desc.ddsCaps.dwCaps = 0x840;
        desc.ddpfPixelFormat = s->pixel;
        s->last_hresult = bh_renderer_surface_create(draw, &desc, &s->render);
        if (s->last_hresult != DD_OK)
            return failed(s);
        s->owns_render = 1;
    }
    else
    {
        bh_renderer_surface_retain(render);
        s->render = render;
    }
    if (screen)
    {
        bh_renderer_surface_retain(screen);
        s->screen = screen;
    }
    if (s->bytes_per_pixel == 1 && screen)
    {
        memset(palette, 0, sizeof(palette));
        for (i = 0; i < 256; ++i)
        {
            palette[i].peRed = (unsigned char)(((i / 42) % 6) * 255 / 5);
            palette[i].peGreen = (unsigned char)(((i / 6) % 7) * 255 / 6);
            palette[i].peBlue = (unsigned char)((i % 6) * 255 / 5);
        }
        s->last_hresult = bh_renderer_palette_create(draw, palette, &s->palette);
        if (s->last_hresult == DD_OK)
            bh_renderer_surface_set_palette(screen, s->palette);
    }
    if (!bh_texture_context_init_software(&s->textures, draw, NULL) ||
        (s->bytes_per_pixel == 2 && !bh_texture_bind_display(&s->textures, &s->pixel)) ||
        !bh_texture_formats(&s->textures))
        return failed(s);
    if (s->bytes_per_pixel == 1)
        for (i = 0; i < 3; ++i)
            s->textures.display_right[i] = 8;
    extents(s);
    masks[0] = s->pixel.dwRBitMask;
    masks[1] = s->pixel.dwGBitMask;
    masks[2] = s->pixel.dwBBitMask;
    if (!bh_software_tables_initialize(&s->tables, &s->textures, s->bytes_per_pixel, masks))
        return failed(s);
    return 1;
}
static void mode_error(BHRenderMode *m, const char *reason, unsigned long w, unsigned long h, unsigned long bits)
{
    char text[128];
    unsigned long used = (unsigned long)strlen(m->errors), need;
    sprintf(text, "%s\n%lu x %lu %lu bit\n", reason, w, h, bits);
    need = (unsigned long)strlen(text);
    if (used + need < sizeof(m->errors))
        memcpy(m->errors + used, text, need + 1);
}
void bh_render_mode_dispose(BHRenderMode *m)
{
    if (!m)
        return;
    bh_render_startup_dispose(&m->engine);
    if (m->render)
        bh_renderer_surface_release(m->render);
    if (m->screen)
        bh_renderer_surface_release(m->screen);
    m->render = m->screen = NULL;
}
int bh_render_mode_startup(BHRenderMode *m, BHRenderer *draw, unsigned long width, unsigned long height,
                           unsigned long bits, int software)
{
    DDSURFACEDESC desc;
    DDBLTFX fill;
    const char *reason;
    if (!m || !draw || m->screen || m->render || m->engine.render)
        return 0;
    /* SDL owns the physical display; these dimensions describe render surfaces. */
    if (!width || !height || (bits != 8 && bits != 16 && bits != 32))
    {
        mode_error(m, "Invalid render dimensions:", width, height, bits);
        m->last_hresult = DDERR_INVALIDPARAMS;
        return 0;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x21;
    desc.dwBackBufferCount = 1;
    desc.ddsCaps.dwCaps = software ? 0x4218 : 0x6218;
    m->last_hresult = bh_renderer_surface_create(draw, &desc, &m->screen);
    if (m->last_hresult != DD_OK)
    {
        desc.ddsCaps.dwCaps &= ~0x4000UL;
        m->last_hresult = bh_renderer_surface_create(draw, &desc, &m->screen);
    }
    reason = "Couldn't create screenbuffer in mode:";
    if (m->last_hresult != DD_OK)
        goto fail;
    m->last_hresult = bh_renderer_surface_backbuffer(m->screen, &m->render);
    reason = "Couldn't create renderbuffer in mode:";
    if (m->last_hresult != DD_OK)
        goto fail;
    memset(&fill, 0, sizeof(fill));
    fill.dwSize = sizeof(fill);
    bh_renderer_surface_blit(m->screen, NULL, NULL, NULL, 0x1000400, &fill);
    bh_renderer_surface_blit(m->render, NULL, NULL, NULL, 0x1000400, &fill);
    reason = software ? "Couldn't init 3D Engine in software mode:" : "Couldn't init 3D Engine in hardware mode:";
    if (software ? bh_render_startup_software(&m->engine, draw, NULL, m->render, m->screen, width, height, bits)
                 : bh_render_startup_hardware(&m->engine, draw, m->render, m->screen, width, height, bits))
        return 1;
    m->last_hresult = m->engine.last_hresult;
fail:
    mode_error(m, reason, width, height, bits);
    bh_render_mode_dispose(m);
    return 0;
}
