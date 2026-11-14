#include "../bh_renderer.h"
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
static int error(BHTextureContext *texture_context, BHTextureError e)
{
    if (texture_context)
        texture_context->error = e;
    return 0;
}
static int native(BHTextureContext *texture_context, HRESULT hr)
{
    texture_context->last_hresult = hr;
    if (hr != DD_OK)
        return error(texture_context, BH_TEXTURE_BACKEND);
    return 1;
}
static int owned_format(BHTextureContext *texture_context, BHTextureFormat *f)
{
    unsigned long i;
    if (!texture_context)
        return 0;
    for (i = 0; i < 9; ++i)
        if (texture_context->formats + i == f)
            return 1;
    return 0;
}
static int context_init(BHTextureContext *texture_context, BHRenderer *draw, BHRenderDevice *device,
                        const BHSceneAllocator *allocator, int software)
{
    static const unsigned long minimum[9] = {16, 8, 1, 8, 4, 1, 4, 2, 1};
    unsigned long i;
    if (!texture_context || !draw || (!software && !device) || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(texture_context, 0, sizeof(*texture_context));
    texture_context->draw = draw;
    texture_context->device = device;
    texture_context->search = -1;
    texture_context->software = software;
    bh_renderer_retain(draw);
    if (device)
        bh_renderer_device_retain(device);
    if (allocator)
        texture_context->allocator = *allocator;
    else
    {
        texture_context->allocator.allocate = allocate;
        texture_context->allocator.release = release;
    }
    for (i = 0; i < 9; ++i)
    {
        BHTextureFormat *texture_format = texture_context->formats + i;
        texture_format->width = texture_format->height = 32UL << (i / 3);
        texture_format->requested_depth = 8 + (i % 3) * 8;
        texture_format->kind = 1;
        texture_format->minimum = minimum[i];
        texture_format->pixel.dwSize = sizeof(texture_format->pixel);
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
void bh_texture_context_dispose(BHTextureContext *texture_context)
{
    unsigned long i;
    BHTextureSlot *texture_slot, *next;
    BHTextureImage *image, *next_image;
    if (!texture_context || !texture_context->allocator.release)
        return;
    for (i = 0; i < 9; ++i)
        for (texture_slot = texture_context->formats[i].slots; texture_slot; texture_slot = next)
        {
            next = texture_slot->next;
            if (texture_slot->image)
                texture_slot->image->resident = NULL;
            if (texture_slot->palette)
                bh_renderer_palette_release(texture_slot->palette);
            if (texture_slot->surface)
                bh_renderer_surface_release(texture_slot->surface);
            texture_context->allocator.release(texture_slot, texture_context->allocator.user);
        }
    for (image = texture_context->images; image; image = next_image)
    {
        next_image = image->next;
        if (image->palette)
            bh_renderer_palette_release(image->palette);
        if (image->source)
            bh_renderer_surface_release(image->source);
        if (image->dither)
            texture_context->allocator.release(image->dither, texture_context->allocator.user);
        if (image->screen)
            texture_context->allocator.release(image->screen, texture_context->allocator.user);
        if (image->runs)
            texture_context->allocator.release(image->runs, texture_context->allocator.user);
        if (image->vertices)
            texture_context->allocator.release(image->vertices, texture_context->allocator.user);
        texture_context->allocator.release(image, texture_context->allocator.user);
    }
    if (texture_context->device)
        bh_renderer_device_release(texture_context->device);
    if (texture_context->draw)
        bh_renderer_release(texture_context->draw);
    memset(texture_context, 0, sizeof(*texture_context));
}
HRESULT CALLBACK bh_texture_format_callback(LPDDSURFACEDESC desc, LPVOID context)
{
    BHTextureFormat *texture_format = (BHTextureFormat *)context;
    DDPIXELFORMAT *format;
    if (!desc || !texture_format)
        return D3DENUMRET_CANCEL;
    format = &desc->ddpfPixelFormat;
    if (format->dwFlags & 3)
        return D3DENUMRET_OK;
    if (format->dwRGBBitCount <= 8)
    {
        if (!(format->dwFlags & 0x28))
            return D3DENUMRET_OK;
    }
    else if (!(format->dwFlags & DDPF_RGB))
        return D3DENUMRET_OK;
    if (format->dwRGBBitCount >= 8 && (!texture_format->pixel.dwRGBBitCount || (texture_format->requested_depth <= format->dwRGBBitCount &&
                                                              format->dwRGBBitCount - texture_format->requested_depth <
                                                                  texture_format->pixel.dwRGBBitCount - texture_format->requested_depth)))
        texture_format->pixel = *format;
    return D3DENUMRET_OK;
}
int bh_texture_formats(BHTextureContext *texture_context)
{
    unsigned long i;
    int ok = 1;
    if (!texture_context || (!texture_context->software && !texture_context->device))
        return error(texture_context, BH_TEXTURE_ARGUMENT);
    texture_context->error = BH_TEXTURE_OK;
    for (i = 0; i < 9; ++i)
    {
        if (texture_context->software)
        {
            texture_context->formats[i].pixel.dwSize = 32;
            texture_context->formats[i].pixel.dwFlags = 0x60;
            texture_context->formats[i].pixel.dwRGBBitCount = 8;
        }
        else if (!native(texture_context, bh_renderer_texture_formats(texture_context->device, bh_texture_format_callback, texture_context->formats + i)))
            ok = 0;
    }
    return ok;
}
BHTextureFormat *bh_texture_request_format(BHTextureContext *texture_context, unsigned long width, unsigned long height,
                                           unsigned long depth, unsigned long kind)
{
    unsigned long i;
    if (!texture_context)
        return NULL;
    for (i = 0; i < 9; ++i)
    {
        BHTextureFormat *texture_format = texture_context->formats + i;
        if (texture_format->width == width && texture_format->height == height && texture_format->requested_depth == depth && texture_format->kind == kind)
        {
            if (texture_format->references >= 65535)
            {
                error(texture_context, BH_TEXTURE_ARGUMENT);
                return NULL;
            }
            ++texture_format->references;
            return texture_format;
        }
    }
    error(texture_context, BH_TEXTURE_FORMAT);
    return NULL;
}
int bh_texture_slot_create(BHTextureContext *texture_context, BHTextureFormat *texture_format)
{
    BHTextureSlot *texture_slot;
    DDSURFACEDESC desc;
    PALETTEENTRY entries[256];
    if (!owned_format(texture_context, texture_format) || !texture_context->device || !texture_context->draw)
        return error(texture_context, BH_TEXTURE_ARGUMENT);
    texture_slot = (BHTextureSlot *)texture_context->allocator.allocate(sizeof(*texture_slot), texture_context->allocator.user);
    if (!texture_slot)
        return error(texture_context, BH_TEXTURE_MEMORY);
    memset(texture_slot, 0, sizeof(*texture_slot));
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x1007;
    desc.dwWidth = texture_format->width;
    desc.dwHeight = texture_format->height;
    desc.ddpfPixelFormat = texture_format->pixel;
    desc.ddsCaps.dwCaps = texture_context->software ? 0x1800 : 0x4005000;
    /* Explicit native RGB platform binding, not a fallback after an error.
       Zero retains the exact original software/hardware capability choice. */
    if (texture_context->slot_caps_override)
        desc.ddsCaps.dwCaps = texture_context->slot_caps_override;
    if (texture_format->kind)
    {
        desc.dwFlags = 0x11007;
        desc.ddckCKSrcBlt.dwColorSpaceLowValue = desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    }
    if (!native(texture_context, bh_renderer_surface_create(texture_context->draw, &desc, &texture_slot->surface)))
        goto failed;
    native(texture_context, bh_renderer_surface_texture_handle(texture_slot->surface, &texture_slot->handle));
    if (!texture_slot->handle)
        goto failed;
    if (texture_format->pixel.dwRGBBitCount == 8)
    {
        memset(entries, 0, sizeof(entries));
        if (native(texture_context, bh_renderer_palette_create(texture_context->draw, entries, &texture_slot->palette)))
            native(texture_context, bh_renderer_surface_set_palette(texture_slot->surface, texture_slot->palette));
    }
    texture_slot->next = texture_format->slots;
    texture_format->slots = texture_slot;
    ++texture_format->count;
    return 1;
failed:
    if (texture_slot->surface)
        bh_renderer_surface_release(texture_slot->surface);
    texture_context->allocator.release(texture_slot, texture_context->allocator.user);
    return 0;
}
int bh_texture_preallocate(BHTextureContext *texture_context)
{
    unsigned long i, remaining;
    if (!texture_context || !texture_context->device || !texture_context->draw)
        return error(texture_context, BH_TEXTURE_ARGUMENT);
    texture_context->error = BH_TEXTURE_OK;
    for (i = 0; i < 9; ++i)
        if (texture_context->formats[i].references < texture_context->formats[i].minimum)
            texture_context->formats[i].minimum = texture_context->formats[i].references;
    i = 0;
    do
    {
        BHTextureFormat *texture_format = texture_context->formats + i;
        if (texture_format->minimum)
        {
            bh_texture_slot_create(texture_context, texture_format);
            if (!texture_format->count)
                return 0;
            --texture_format->minimum;
            --texture_format->references;
        }
        i = (i + 1) % 9;
        remaining = 0;
        {
            unsigned long j;
            for (j = 0; j < 9; ++j)
                remaining += texture_context->formats[j].minimum;
        }
    } while (remaining);
    do
    {
        BHTextureFormat *source_texture_format = texture_context->formats + i;
        if (source_texture_format->references)
        {
            bh_texture_slot_create(texture_context, source_texture_format);
            --source_texture_format->references;
        }
        i = (i + 1) % 9;
        remaining = 0;
        {
            unsigned long j;
            for (j = 0; j < 9; ++j)
                remaining += texture_context->formats[j].references;
        }
    } while (remaining);
    return 1;
}
static void palette_copy(BHTextureContext *texture_context, BHTextureImage *image, BHTextureSlot *texture_slot)
{
    PALETTEENTRY entries[256];
    if (!image->palette || !texture_slot->palette)
        return;
    memset(entries, 0, sizeof(entries));
    native(texture_context, bh_renderer_palette_get(image->palette, 0, 256, entries));
    native(texture_context, bh_renderer_palette_set(texture_slot->palette, 0, 256, entries));
    ++texture_context->palette_uploads;
}
static void pixels_copy(BHTextureContext *texture_context, BHTextureImage *image, BHTextureSlot *texture_slot)
{
    native(texture_context, bh_renderer_surface_blit(texture_slot->surface, NULL, image->source, NULL, DDBLT_WAIT, NULL));

    ++texture_context->pixel_uploads;
}
D3DTEXTUREHANDLE bh_texture_resident(BHTextureContext *texture_context, BHTextureImage *image)
{
    BHTextureSlot *texture_slot, *next;
    if (!texture_context || !image || !image->source || !owned_format(texture_context, image->format))
    {
        error(texture_context, BH_TEXTURE_ARGUMENT);
        return 0;
    }
    /* Diagnostics belong to this residency operation. Optional preallocation
       can retain a usable cache while reporting an earlier allocation error.
       Native failures below remain sticky until the next operation. */
    texture_context->error = BH_TEXTURE_OK;
    texture_slot = image->resident;
    if (texture_slot)
    {
        if (texture_slot->palette_dirty && image->palette && texture_slot->palette)
        {
            palette_copy(texture_context, image, texture_slot);
            texture_slot->palette_dirty = 0;
        }
        if (texture_slot->pixels_dirty)
        {
            texture_slot->pixels_dirty = 0;
            pixels_copy(texture_context, image, texture_slot);
        }
        texture_slot->frame = texture_context->frame;
        return texture_slot->handle;
    }
    texture_slot = image->format->slots;
    if (!texture_slot)
        return 0;
    for (next = texture_slot->next; next && texture_slot->image; next = next->next)
        if (!next->image || next->frame < texture_slot->frame)
            texture_slot = next;
    texture_slot->pixels_dirty = texture_slot->palette_dirty = 0;
    if (texture_slot->frame == texture_context->frame && texture_slot->image)
    {
        native(texture_context, bh_renderer_record_draws(texture_context->device));

        ++texture_context->flushes;
    }
    if (texture_slot->image)
    {
        texture_slot->image->resident = NULL;
        ++texture_context->evictions;
    }
    image->resident = texture_slot;
    texture_slot->frame = texture_context->frame;
    texture_slot->image = image;
    palette_copy(texture_context, image, texture_slot);
    pixels_copy(texture_context, image, texture_slot);
    return texture_slot->handle;
}
