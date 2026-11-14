#include "bh_renderer.h"
#include "bh_render_startup.h"
#include <string.h>
#include <stdio.h>

static void extents(BHRenderStartup *render_startup)
{
    render_startup->extent_x = (float)render_startup->width;
    render_startup->extent_y = (float)render_startup->height;
    render_startup->clip_x = render_startup->extent_x;
    render_startup->clip_y = render_startup->extent_y;
    render_startup->scale_x = render_startup->scale_y = 0;
    render_startup->offset_x = render_startup->offset_y = 0;
}
static int format(BHRenderStartup *render_startup, BHRenderSurface *surface, unsigned long width, unsigned long height,
                  unsigned long bits, int software)
{
    if (!render_startup || !surface || !width || !height || width > 0x7fffffffUL || height > 0x7fffffffUL ||
        (bits != 8 && bits != 16 && bits != 24 && bits != 32) || render_startup->depth || render_startup->render || render_startup->textures.draw)
        return 0;
    render_startup->width = width;
    render_startup->height = height;
    render_startup->bits = bits;
    render_startup->software = software;
    render_startup->bytes_per_pixel = bits == 8 ? 1 : bits == 16 ? 2 : 4;
    memset(&render_startup->pixel, 0, sizeof(render_startup->pixel));
    render_startup->pixel.dwSize = sizeof(render_startup->pixel);
    /* The original ignores GetPixelFormat failure; source guards retain the
       error rather than continuing with an unspecified display format. */
    render_startup->last_hresult = bh_renderer_surface_format(surface, &render_startup->pixel);
    return render_startup->last_hresult == DD_OK;
}
void bh_render_startup_dispose(BHRenderStartup *render_startup)
{
    unsigned long i;
    HRESULT hr;
    int retained = 0;
    if (!render_startup)
        return;
    bh_software_tables_dispose(&render_startup->tables);
    bh_texture_context_dispose(&render_startup->textures);
    if (render_startup->device)
    {
        bh_renderer_device_release(render_startup->device);
        render_startup->device = NULL;
    }
    if (render_startup->renderer)
    {
        bh_renderer_release(render_startup->renderer);
        render_startup->renderer = NULL;
    }
    for (i = 0; i < 2; ++i)
        if (render_startup->depth_attachments[i])
        {
            hr = bh_renderer_surface_detach_depth(render_startup->depth_attachments[i], render_startup->depth);
            render_startup->last_detach_hresult = hr;
            if (hr != DD_OK && hr != DDERR_SURFACENOTATTACHED)
            {
                ++render_startup->detach_failures;
                retained = 1;
                continue;
            }
            bh_renderer_surface_release(render_startup->depth_attachments[i]);
            render_startup->depth_attachments[i] = NULL;
            --render_startup->depth_attachment_count;
        }
    if (retained)
        return;
    if (render_startup->depth)
        bh_renderer_surface_release(render_startup->depth);
    if (render_startup->palette)
        bh_renderer_palette_release(render_startup->palette);
    if (render_startup->render)
        bh_renderer_surface_release(render_startup->render);
    if (render_startup->screen)
        bh_renderer_surface_release(render_startup->screen);
    memset(render_startup, 0, sizeof(*render_startup));
}
HRESULT bh_render_startup_attach_depth(BHRenderStartup *render_startup, BHRenderSurface *target)
{
    unsigned long i;
    HRESULT hr;
    if (!render_startup || !render_startup->depth || !target)
        return DDERR_INVALIDPARAMS;
    for (i = 0; i < 2 && render_startup->depth_attachments[i]; ++i)
    {
    }
    if (i == 2)
        return DDERR_INVALIDPARAMS;
    hr = bh_renderer_surface_attach_depth(target, render_startup->depth);
    if (hr == DD_OK)
    {
        bh_renderer_surface_retain(target);
        render_startup->depth_attachments[i] = target;
        ++render_startup->depth_attachment_count;
    }
    return hr;
}
static int failed(BHRenderStartup *render_startup)
{
    HRESULT hr = render_startup->last_hresult;
    bh_render_startup_dispose(render_startup);
    render_startup->last_hresult = hr;
    return 0;
}
int bh_render_startup_hardware(BHRenderStartup *render_startup, BHRenderer *draw, BHRenderSurface *render, BHRenderSurface *screen,
                               unsigned long width, unsigned long height, unsigned long bits)
{
    DDSURFACEDESC depth;
    if (!draw || !format(render_startup, render, width, height, bits, 0))
        return 0;
    if (render_startup->bytes_per_pixel < 2)
        return 0;
    memset(&depth, 0, sizeof(depth));
    depth.dwSize = sizeof(depth);
    depth.dwFlags = 0x47;
    depth.dwWidth = width;
    depth.dwHeight = height;
    depth.dwZBufferBitDepth = 16;
    depth.ddsCaps.dwCaps = 0x24000;
    render_startup->last_hresult = bh_renderer_surface_create(draw, &depth, &render_startup->depth);
    if (render_startup->last_hresult != DD_OK)
        return failed(render_startup);
    if (screen)
        bh_render_startup_attach_depth(render_startup, screen);
    bh_render_startup_attach_depth(render_startup, render);
    if (!render_startup->renderer)
    {
        render_startup->renderer = draw;
        bh_renderer_retain(draw);
    }
    render_startup->last_hresult = bh_renderer_device_create(render_startup->renderer, render, &render_startup->device);
    if (render_startup->last_hresult != DD_OK)
        return failed(render_startup);
    bh_renderer_surface_retain(render);
    render_startup->render = render;
    if (screen)
    {
        bh_renderer_surface_retain(screen);
        render_startup->screen = screen;
    }
    extents(render_startup);
    if (!bh_texture_context_init(&render_startup->textures, draw, render_startup->device, NULL) || !bh_texture_formats(&render_startup->textures))
        return failed(render_startup);
    return 1;
}
int bh_render_startup_software(BHRenderStartup *render_startup, BHRenderer *draw, BHRenderSurface *format_surface,
                               BHRenderSurface *render, BHRenderSurface *screen, unsigned long width,
                               unsigned long height, unsigned long bits)
{
    DDSURFACEDESC desc;
    PALETTEENTRY palette[256];
    unsigned long i, masks[3];
    if (!format_surface)
        format_surface = render ? render : screen;
    if (!draw || !format(render_startup, format_surface, width, height, bits, 1))
        return 0;
    if (render_startup->bytes_per_pixel > 2)
        return 0;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x47;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.dwZBufferBitDepth = 16;
    desc.ddsCaps.dwCaps = 0x20800;
    render_startup->last_hresult = bh_renderer_surface_create(draw, &desc, &render_startup->depth);
    if (render_startup->last_hresult != DD_OK)
        return failed(render_startup);
    if (!render)
    {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        desc.dwFlags = 0x1007;
        desc.dwWidth = width;
        desc.dwHeight = height;
        desc.ddsCaps.dwCaps = 0x840;
        desc.ddpfPixelFormat = render_startup->pixel;
        render_startup->last_hresult = bh_renderer_surface_create(draw, &desc, &render_startup->render);
        if (render_startup->last_hresult != DD_OK)
            return failed(render_startup);
        render_startup->owns_render = 1;
    }
    else
    {
        bh_renderer_surface_retain(render);
        render_startup->render = render;
    }
    if (screen)
    {
        bh_renderer_surface_retain(screen);
        render_startup->screen = screen;
    }
    if (render_startup->bytes_per_pixel == 1 && screen)
    {
        memset(palette, 0, sizeof(palette));
        for (i = 0; i < 256; ++i)
        {
            palette[i].peRed = (unsigned char)(((i / 42) % 6) * 255 / 5);
            palette[i].peGreen = (unsigned char)(((i / 6) % 7) * 255 / 6);
            palette[i].peBlue = (unsigned char)((i % 6) * 255 / 5);
        }
        render_startup->last_hresult = bh_renderer_palette_create(draw, palette, &render_startup->palette);
        if (render_startup->last_hresult == DD_OK)
            bh_renderer_surface_set_palette(screen, render_startup->palette);
    }
    if (!bh_texture_context_init_software(&render_startup->textures, draw, NULL) ||
        (render_startup->bytes_per_pixel == 2 && !bh_texture_bind_display(&render_startup->textures, &render_startup->pixel)) ||
        !bh_texture_formats(&render_startup->textures))
        return failed(render_startup);
    if (render_startup->bytes_per_pixel == 1)
        for (i = 0; i < 3; ++i)
            render_startup->textures.display_right[i] = 8;
    extents(render_startup);
    masks[0] = render_startup->pixel.dwRBitMask;
    masks[1] = render_startup->pixel.dwGBitMask;
    masks[2] = render_startup->pixel.dwBBitMask;
    if (!bh_software_tables_initialize(&render_startup->tables, &render_startup->textures, render_startup->bytes_per_pixel, masks))
        return failed(render_startup);
    return 1;
}
static void mode_error(BHRenderMode *render_mode, const char *reason, unsigned long w, unsigned long h, unsigned long bits)
{
    char text[128];
    unsigned long used = (unsigned long)strlen(render_mode->errors), need;
    sprintf(text, "%s\n%lu x %lu %lu bit\n", reason, w, h, bits);
    need = (unsigned long)strlen(text);
    if (used + need < sizeof(render_mode->errors))
        memcpy(render_mode->errors + used, text, need + 1);
}
void bh_render_mode_dispose(BHRenderMode *render_mode)
{
    if (!render_mode)
        return;
    bh_render_startup_dispose(&render_mode->engine);
    if (render_mode->render)
        bh_renderer_surface_release(render_mode->render);
    if (render_mode->screen)
        bh_renderer_surface_release(render_mode->screen);
    render_mode->render = render_mode->screen = NULL;
}
int bh_render_mode_startup(BHRenderMode *render_mode, BHRenderer *draw, unsigned long width, unsigned long height,
                           unsigned long bits, int software)
{
    DDSURFACEDESC desc;
    DDBLTFX fill;
    const char *reason;
    if (!render_mode || !draw || render_mode->screen || render_mode->render || render_mode->engine.render)
        return 0;
    /* SDL owns the physical display; these dimensions describe render surfaces. */
    if (!width || !height || (bits != 8 && bits != 16 && bits != 32))
    {
        mode_error(render_mode, "Invalid render dimensions:", width, height, bits);
        render_mode->last_hresult = DDERR_INVALIDPARAMS;
        return 0;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x21;
    desc.dwBackBufferCount = 1;
    desc.ddsCaps.dwCaps = software ? 0x4218 : 0x6218;
    render_mode->last_hresult = bh_renderer_surface_create(draw, &desc, &render_mode->screen);
    if (render_mode->last_hresult != DD_OK)
    {
        desc.ddsCaps.dwCaps &= ~0x4000UL;
        render_mode->last_hresult = bh_renderer_surface_create(draw, &desc, &render_mode->screen);
    }
    reason = "Couldn't create screenbuffer in mode:";
    if (render_mode->last_hresult != DD_OK)
        goto fail;
    render_mode->last_hresult = bh_renderer_surface_backbuffer(render_mode->screen, &render_mode->render);
    reason = "Couldn't create renderbuffer in mode:";
    if (render_mode->last_hresult != DD_OK)
        goto fail;
    memset(&fill, 0, sizeof(fill));
    fill.dwSize = sizeof(fill);
    bh_renderer_surface_blit(render_mode->screen, NULL, NULL, NULL, 0x1000400, &fill);
    bh_renderer_surface_blit(render_mode->render, NULL, NULL, NULL, 0x1000400, &fill);
    reason = software ? "Couldn't init 3D Engine in software mode:" : "Couldn't init 3D Engine in hardware mode:";
    if (software ? bh_render_startup_software(&render_mode->engine, draw, NULL, render_mode->render, render_mode->screen, width, height, bits)
                 : bh_render_startup_hardware(&render_mode->engine, draw, render_mode->render, render_mode->screen, width, height, bits))
        return 1;
    render_mode->last_hresult = render_mode->engine.last_hresult;
fail:
    mode_error(render_mode, reason, width, height, bits);
    bh_render_mode_dispose(render_mode);
    return 0;
}
