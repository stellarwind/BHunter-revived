#include "bh_renderer.h"
#include "bh_texture.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static_assert(sizeof(BHTextureFormat) == 64, "BHTextureFormat size");
static_assert(sizeof(BHTextureSlot) == 28, "BHTextureSlot size");
static_assert(offsetof(BHTextureSlot, palette_dirty) == 0x16, "BHTextureSlot.palette_dirty offset");
static_assert(sizeof(BHTextureImage) == 0x70, "BHTextureImage size");
static_assert(offsetof(BHTextureImage, name) == 0x1c, "BHTextureImage.name offset");
static_assert(offsetof(BHTextureImage, runs) == 0x48, "BHTextureImage.runs offset");
static_assert(offsetof(BHTextureImage, flags) == 0x50, "BHTextureImage.flags offset");
static void *allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void release(void *p, void *user)
{
    free(p);
}
static int error(BHTextureContext *q, BHTextureError e)
{
    if (q)
        q->error = e;
    return 0;
}
static int native(BHTextureContext *q, HRESULT hr)
{
    q->last_hresult = hr;
    if (hr != DD_OK)
        return error(q, BH_TEXTURE_BACKEND);
    return 1;
}
static int owned_format(BHTextureContext *q, BHTextureFormat *f)
{
    unsigned long i;
    if (!q)
        return 0;
    for (i = 0; i < 9; ++i)
        if (q->formats + i == f)
            return 1;
    return 0;
}
static int context_init(BHTextureContext *q, BHRenderer *draw, BHRenderDevice *device,
                        const BHSceneAllocator *allocator, int software)
{
    static const unsigned long minimum[9] = {16, 8, 1, 8, 4, 1, 4, 2, 1};
    unsigned long i;
    if (!q || !draw || (!software && !device) || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(q, 0, sizeof(*q));
    q->draw = draw;
    q->device = device;
    q->search = -1;
    q->software = software;
    bh_renderer_retain(draw);
    if (device)
        bh_renderer_device_retain(device);
    if (allocator)
        q->allocator = *allocator;
    else
    {
        q->allocator.allocate = allocate;
        q->allocator.release = release;
    }
    for (i = 0; i < 9; ++i)
    {
        BHTextureFormat *f = q->formats + i;
        f->width = f->height = 32UL << (i / 3);
        f->requested_depth = 8 + (i % 3) * 8;
        f->kind = 1;
        f->minimum = minimum[i];
        f->pixel.dwSize = sizeof(f->pixel);
    }
    return 1;
}
int bh_texture_context_init(BHTextureContext *q, BHRenderer *draw, BHRenderDevice *device,
                            const BHSceneAllocator *allocator)
{
    return context_init(q, draw, device, allocator, 0);
}
int bh_texture_context_init_software(BHTextureContext *q, BHRenderer *draw, const BHSceneAllocator *allocator)
{
    return context_init(q, draw, NULL, allocator, 1);
}
void bh_texture_context_dispose(BHTextureContext *q)
{
    unsigned long i;
    BHTextureSlot *s, *next;
    BHTextureImage *image, *next_image;
    if (!q || !q->allocator.release)
        return;
    for (i = 0; i < 9; ++i)
        for (s = q->formats[i].slots; s; s = next)
        {
            next = s->next;
            if (s->image)
                s->image->resident = NULL;
            if (s->palette)
                bh_renderer_palette_release(s->palette);
            if (s->surface)
                bh_renderer_surface_release(s->surface);
            q->allocator.release(s, q->allocator.user);
        }
    for (image = q->images; image; image = next_image)
    {
        next_image = image->next;
        if (image->palette)
            bh_renderer_palette_release(image->palette);
        if (image->source)
            bh_renderer_surface_release(image->source);
        if (image->dither)
            q->allocator.release(image->dither, q->allocator.user);
        if (image->screen)
            q->allocator.release(image->screen, q->allocator.user);
        if (image->runs)
            q->allocator.release(image->runs, q->allocator.user);
        if (image->vertices)
            q->allocator.release(image->vertices, q->allocator.user);
        q->allocator.release(image, q->allocator.user);
    }
    if (q->device)
        bh_renderer_device_release(q->device);
    if (q->draw)
        bh_renderer_release(q->draw);
    memset(q, 0, sizeof(*q));
}
HRESULT CALLBACK bh_texture_format_callback(LPDDSURFACEDESC desc, LPVOID context)
{
    BHTextureFormat *f = (BHTextureFormat *)context;
    DDPIXELFORMAT *p;
    if (!desc || !f)
        return D3DENUMRET_CANCEL;
    p = &desc->ddpfPixelFormat;
    if (p->dwFlags & 3)
        return D3DENUMRET_OK;
    if (p->dwRGBBitCount <= 8)
    {
        if (!(p->dwFlags & 0x28))
            return D3DENUMRET_OK;
    }
    else if (!(p->dwFlags & DDPF_RGB))
        return D3DENUMRET_OK;
    if (p->dwRGBBitCount >= 8 && (!f->pixel.dwRGBBitCount || (f->requested_depth <= p->dwRGBBitCount &&
                                                              p->dwRGBBitCount - f->requested_depth <
                                                                  f->pixel.dwRGBBitCount - f->requested_depth)))
        f->pixel = *p;
    return D3DENUMRET_OK;
}
int bh_texture_formats(BHTextureContext *q)
{
    unsigned long i;
    int ok = 1;
    if (!q || (!q->software && !q->device))
        return error(q, BH_TEXTURE_ARGUMENT);
    q->error = BH_TEXTURE_OK;
    for (i = 0; i < 9; ++i)
    {
        if (q->software)
        {
            q->formats[i].pixel.dwSize = 32;
            q->formats[i].pixel.dwFlags = 0x60;
            q->formats[i].pixel.dwRGBBitCount = 8;
        }
        else if (!native(q, bh_renderer_texture_formats(q->device, bh_texture_format_callback, q->formats + i)))
            ok = 0;
    }
    return ok;
}
BHTextureFormat *bh_texture_request_format(BHTextureContext *q, unsigned long width, unsigned long height,
                                           unsigned long depth, unsigned long kind)
{
    unsigned long i;
    if (!q)
        return NULL;
    for (i = 0; i < 9; ++i)
    {
        BHTextureFormat *f = q->formats + i;
        if (f->width == width && f->height == height && f->requested_depth == depth && f->kind == kind)
        {
            if (f->references >= 65535)
            {
                error(q, BH_TEXTURE_ARGUMENT);
                return NULL;
            }
            ++f->references;
            return f;
        }
    }
    error(q, BH_TEXTURE_FORMAT);
    return NULL;
}
int bh_texture_slot_create(BHTextureContext *q, BHTextureFormat *f)
{
    BHTextureSlot *s;
    DDSURFACEDESC desc;
    PALETTEENTRY entries[256];
    if (!owned_format(q, f) || !q->device || !q->draw)
        return error(q, BH_TEXTURE_ARGUMENT);
    s = (BHTextureSlot *)q->allocator.allocate(sizeof(*s), q->allocator.user);
    if (!s)
        return error(q, BH_TEXTURE_MEMORY);
    memset(s, 0, sizeof(*s));
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x1007;
    desc.dwWidth = f->width;
    desc.dwHeight = f->height;
    desc.ddpfPixelFormat = f->pixel;
    desc.ddsCaps.dwCaps = q->software ? 0x1800 : 0x4005000;
    /* Explicit native RGB platform binding, not a fallback after an error.
       Zero retains the exact original software/hardware capability choice. */
    if (q->slot_caps_override)
        desc.ddsCaps.dwCaps = q->slot_caps_override;
    if (f->kind)
    {
        desc.dwFlags = 0x11007;
        desc.ddckCKSrcBlt.dwColorSpaceLowValue = desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    }
    if (!native(q, bh_renderer_surface_create(q->draw, &desc, &s->surface)))
        goto failed;
    native(q, bh_renderer_surface_texture_handle(s->surface, &s->handle));
    if (!s->handle)
        goto failed;
    if (f->pixel.dwRGBBitCount == 8)
    {
        memset(entries, 0, sizeof(entries));
        if (native(q, bh_renderer_palette_create(q->draw, entries, &s->palette)))
            native(q, bh_renderer_surface_set_palette(s->surface, s->palette));
    }
    s->next = f->slots;
    f->slots = s;
    ++f->count;
    return 1;
failed:
    if (s->surface)
        bh_renderer_surface_release(s->surface);
    q->allocator.release(s, q->allocator.user);
    return 0;
}
int bh_texture_preallocate(BHTextureContext *q)
{
    unsigned long i, remaining;
    if (!q || !q->device || !q->draw)
        return error(q, BH_TEXTURE_ARGUMENT);
    q->error = BH_TEXTURE_OK;
    for (i = 0; i < 9; ++i)
        if (q->formats[i].references < q->formats[i].minimum)
            q->formats[i].minimum = q->formats[i].references;
    i = 0;
    do
    {
        BHTextureFormat *f = q->formats + i;
        if (f->minimum)
        {
            bh_texture_slot_create(q, f);
            if (!f->count)
                return 0;
            --f->minimum;
            --f->references;
        }
        i = (i + 1) % 9;
        remaining = 0;
        {
            unsigned long j;
            for (j = 0; j < 9; ++j)
                remaining += q->formats[j].minimum;
        }
    } while (remaining);
    do
    {
        BHTextureFormat *f = q->formats + i;
        if (f->references)
        {
            bh_texture_slot_create(q, f);
            --f->references;
        }
        i = (i + 1) % 9;
        remaining = 0;
        {
            unsigned long j;
            for (j = 0; j < 9; ++j)
                remaining += q->formats[j].references;
        }
    } while (remaining);
    return 1;
}
static void palette_copy(BHTextureContext *q, BHTextureImage *image, BHTextureSlot *s)
{
    PALETTEENTRY entries[256];
    if (!image->palette || !s->palette)
        return;
    memset(entries, 0, sizeof(entries));
    native(q, bh_renderer_palette_get(image->palette, 0, 256, entries));
    native(q, bh_renderer_palette_set(s->palette, 0, 256, entries));
    ++q->palette_uploads;
}
static void pixels_copy(BHTextureContext *q, BHTextureImage *image, BHTextureSlot *s)
{
    native(q, bh_renderer_surface_blit(s->surface, NULL, image->source, NULL, DDBLT_WAIT, NULL));

    ++q->pixel_uploads;
}
D3DTEXTUREHANDLE bh_texture_resident(BHTextureContext *q, BHTextureImage *image)
{
    BHTextureSlot *s, *next;
    if (!q || !image || !image->source || !owned_format(q, image->format))
    {
        error(q, BH_TEXTURE_ARGUMENT);
        return 0;
    }
    /* Diagnostics belong to this residency operation. Optional preallocation
       can retain a usable cache while reporting an earlier allocation error.
       Native failures below remain sticky until the next operation. */
    q->error = BH_TEXTURE_OK;
    s = image->resident;
    if (s)
    {
        if (s->palette_dirty && image->palette && s->palette)
        {
            palette_copy(q, image, s);
            s->palette_dirty = 0;
        }
        if (s->pixels_dirty)
        {
            s->pixels_dirty = 0;
            pixels_copy(q, image, s);
        }
        s->frame = q->frame;
        return s->handle;
    }
    s = image->format->slots;
    if (!s)
        return 0;
    for (next = s->next; next && s->image; next = next->next)
        if (!next->image || next->frame < s->frame)
            s = next;
    s->pixels_dirty = s->palette_dirty = 0;
    if (s->frame == q->frame && s->image)
    {
        native(q, bh_renderer_record_draws(q->device));

        ++q->flushes;
    }
    if (s->image)
    {
        s->image->resident = NULL;
        ++q->evictions;
    }
    image->resident = s;
    s->frame = q->frame;
    s->image = image;
    palette_copy(q, image, s);
    pixels_copy(q, image, s);
    return s->handle;
}
