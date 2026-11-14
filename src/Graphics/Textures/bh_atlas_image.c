#include "../bh_renderer.h"
#include "bh_atlas_image.h"
#include "../../Assets/bh_pcx_reader.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(BHAtlasImage) == 0x13ac, "BHAtlasImage size");
static_assert(offsetof(BHAtlasImage, palette) == 0x84, "BHAtlasImage.palette offset");
static_assert(offsetof(BHAtlasImage, pixel) == 0x384, "BHAtlasImage.pixel offset");
static_assert(offsetof(BHAtlasImage, rectangles) == 0x3a8, "BHAtlasImage.rectangles offset");
static_assert(offsetof(BHAtlasImage, surface) == 0x13a8, "BHAtlasImage.surface offset");
static int fail(BHAtlasContext *context, BHTextureError error)
{
    if (context)
        context->error = error;
    return 0;
}
static int native(BHAtlasContext *context, HRESULT hr)
{
    context->last_hresult = hr;
    return hr == DD_OK ? 1 : fail(context, BH_TEXTURE_BACKEND);
}
int bh_atlas_context_init(BHAtlasContext *context, BHTextureContext *texture_context, unsigned long step)
{
    if (!context || context->images || !texture_context || !texture_context->draw || !texture_context->allocator.allocate || !texture_context->allocator.release)
        return 0;
    memset(context, 0, sizeof(*context));
    context->textures = texture_context;
    context->scan_step = step;
    return 1;
}
void bh_atlas_context_dispose(BHAtlasContext *context)
{
    BHAtlasImage *atlas_image, *next;
    if (!context)
        return;
    if (context->textures && context->textures->allocator.release)
        for (atlas_image = context->images; atlas_image; atlas_image = next)
        {
            next = atlas_image->next;
            if (atlas_image->surface)
                bh_renderer_surface_release(atlas_image->surface);
            context->textures->allocator.release(atlas_image, context->textures->allocator.user);
        }
    memset(context, 0, sizeof(*context));
}
static int format(const DDPIXELFORMAT *ddpixelformat)
{
    unsigned long masks[3], i, mask, bits, used = 0;
    if (!ddpixelformat || ddpixelformat->dwSize != sizeof(*ddpixelformat))
        return 0;
    if (ddpixelformat->dwRGBBitCount == 8)
        return (ddpixelformat->dwFlags & DDPF_PALETTEINDEXED8) != 0;
    if (!(ddpixelformat->dwFlags & DDPF_RGB) || (ddpixelformat->dwRGBBitCount != 16 && ddpixelformat->dwRGBBitCount != 24 && ddpixelformat->dwRGBBitCount != 32))
        return 0;
    masks[0] = ddpixelformat->dwRBitMask;
    masks[1] = ddpixelformat->dwGBitMask;
    masks[2] = ddpixelformat->dwBBitMask;
    for (i = 0; i < 3; ++i)
    {
        mask = masks[i];
        if (!mask || (mask & 0x80000000UL) || (mask & used) ||
            (ddpixelformat->dwRGBBitCount < 32 && mask >= (1UL << ddpixelformat->dwRGBBitCount)))
            return 0;
        used |= mask;
        bits = 0;
        while (!(mask & 1))
            mask >>= 1;
        while (mask & 1)
        {
            ++bits;
            mask >>= 1;
        }
        if (mask || bits > 8)
            return 0;
    }
    return 1;
}
static unsigned long packed(const unsigned char *rgb, const DDPIXELFORMAT *format)
{
    unsigned long masks[3], i, value = 0;
    long mask;
    int shift;
    masks[0] = format->dwRBitMask;
    masks[1] = format->dwGBitMask;
    masks[2] = format->dwBBitMask;
    for (i = 0; i < 3; ++i)
    {
        shift = -8;
        for (mask = (long)masks[i]; mask; mask /= 2)
            ++shift;
        value += (shift < 0 ? (unsigned long)rgb[i] >> (-shift) : (unsigned long)rgb[i] << shift) & masks[i];
    }
    return value;
}
static int row_blank(const unsigned char *data, unsigned long width, unsigned char transparent)
{
    unsigned long x;
    for (x = 0; x < width; ++x)
        if (data[x] != transparent)
            return 0;
    return 1;
}
static int column_blank(const unsigned char *data, unsigned long rows, unsigned long step, unsigned char transparent)
{
    unsigned long y;
    for (y = 0; y < rows; ++y)
        if (data[y * step] != transparent)
            return 0;
    return 1;
}
static int rectangles(BHAtlasContext *context, BHAtlasImage *image, const unsigned char *data, unsigned long width,
                      unsigned long height, unsigned long pitch)
{
    unsigned long top = 0, bottom, left, right, x, rows, extent = pitch * height;
    unsigned char transparent = data[0];
    BHAtlasRectangle *atlas_rectangle;
    while (top < height)
    {
        while (top < height && row_blank(data + top * pitch, width, transparent))
            ++top;
        if (top == height)
            break;
        bottom = top;
        while (bottom < height && !row_blank(data + bottom * pitch, width, transparent))
            ++bottom;
        rows = bottom - top;
        /* Original scan_step is the active screen pitch, not necessarily the
           atlas pitch. Preserve it, including zero, but check every span. */
        if (context->scan_step && (rows - 1) > (extent - top * pitch - width) / context->scan_step)
            return fail(context, BH_TEXTURE_TOPOLOGY);
        x = 0;
        while (x < width)
        {
            while (x < width && column_blank(data + top * pitch + x, rows, context->scan_step, transparent))
                ++x;
            if (x == width)
                break;
            left = x;
            while (x < width && !column_blank(data + top * pitch + x, rows, context->scan_step, transparent))
                ++x;
            right = x;
            if (image->rectangle_count == 512)
                return fail(context, BH_TEXTURE_TOPOLOGY);
            atlas_rectangle = image->rectangles + image->rectangle_count++;
            atlas_rectangle->x = (unsigned short)left;
            atlas_rectangle->y = (unsigned short)top;
            atlas_rectangle->width = (unsigned short)(right - left);
            atlas_rectangle->height = (unsigned short)(bottom - top);
        }
        top = bottom;
    }
    return 1;
}
BHAtlasImage *bh_atlas_image_load(BHAtlasContext *context, const char *name, const DDPIXELFORMAT *pixel)
{
    BHTextureContext *texture_context;
    BHAtlasImage *image, *slow, *fast;
    BHPcxReader reader;
    BHPcxPalette palette;
    DDSURFACEDESC desc, lock;
    BHRenderSurface *surface = NULL;
    unsigned char *indices = NULL, index;
    unsigned long colors[256], i, x, y, bytes, pitch;
    int locked = 0, ok = 1;
    if (!context || !(texture_context = context->textures) || !texture_context->draw || !texture_context->allocator.allocate || !texture_context->allocator.release || !name || !name[0] ||
        strlen(name) >= 128)
    {
        fail(context, BH_TEXTURE_ARGUMENT);
        return NULL;
    }
    context->error = BH_TEXTURE_OK;
    context->last_hresult = DD_OK;
    slow = fast = context->images;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
        {
            fail(context, BH_TEXTURE_TOPOLOGY);
            return NULL;
        }
    }
    for (image = context->images; image; image = image->next)
        if (!_stricmp(name, image->name))
            return image;
    if (!format(pixel))
    {
        fail(context, BH_TEXTURE_FORMAT);
        return NULL;
    }
    image = NULL;
    bh_pcx_reader_init(&reader);
    if (!bh_pcx_reader_open(&reader, name))
    {
        fail(context, BH_TEXTURE_IO);
        return NULL;
    }
    if (!reader.width || !reader.height || reader.width > 4096 || reader.height > 4096)
    {
        fail(context, BH_TEXTURE_FORMAT);
        goto failed;
    }
    if (!bh_pcx_palette(&palette, reader.palette, texture_context->palette_mode, texture_context->palette_color))
    {
        fail(context, BH_TEXTURE_FORMAT);
        goto failed;
    }
    /* Font pages use black as the neutral background when composited.
       World-texture policies can raise source black to RGB (8,8,8), which
       exposes each copied glyph rectangle over the cleared page. Preserve
       exact source black without discarding the atlas's shaded edges. */
    for (i = 0; i < 256; ++i)
        if (!reader.palette[i * 3] && !reader.palette[i * 3 + 1] && !reader.palette[i * 3 + 2])
            palette.indices[i] = 0;
    memcpy(reader.map, palette.indices, 256);
    bytes = (pixel->dwRGBBitCount + 7) / 8;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x1007;
    desc.dwWidth = reader.width;
    desc.dwHeight = reader.height;
    desc.ddpfPixelFormat = *pixel;
    desc.ddsCaps.dwCaps = 0x840;
    if (!native(context, bh_renderer_surface_create(texture_context->draw, &desc, &surface)))
        goto failed;
    image = (BHAtlasImage *)texture_context->allocator.allocate(sizeof(*image), texture_context->allocator.user);
    if (!image)
    {
        fail(context, BH_TEXTURE_MEMORY);
        goto failed;
    }
    memset(image, 0, sizeof(*image));
    strcpy(image->name, name);
    memcpy(image->palette, palette.rgb, 768);
    image->pixel = *pixel;
    image->surface = surface;
    memset(&lock, 0, sizeof(lock));
    lock.dwSize = sizeof(lock);
    if (!native(context, bh_renderer_surface_lock(surface, &lock)))
        goto failed;
    locked = 1;
    if (!lock.lpSurface || lock.lPitch < (long)(reader.width * bytes))
    {
        fail(context, BH_TEXTURE_TOPOLOGY);
        goto failed;
    }
    if (bytes > 1)
    {
        for (i = 0; i < 256; ++i)
            colors[i] = packed(palette.rgb + i * 3, pixel);
        indices =
            (unsigned char *)texture_context->allocator.allocate((unsigned long)reader.width * reader.height, texture_context->allocator.user);
        if (!indices)
        {
            fail(context, BH_TEXTURE_MEMORY);
            goto failed;
        }
    }
    for (y = 0; ok && y < reader.height; ++y)
        for (x = 0; x < reader.width; ++x)
        {
            unsigned char *out = (unsigned char *)lock.lpSurface + y * lock.lPitch + x * bytes;
            if (!bh_pcx_reader_next(&reader, &index))
            {
                ok = fail(context, BH_TEXTURE_IO);
                break;
            }
            if (bytes == 1)
                *out = index;
            else
            {
                indices[y * reader.width + x] = index;
                for (i = 0; i < bytes; ++i)
                    out[i] = (unsigned char)(colors[index] >> (i * 8));
            }
        }
    pitch = bytes == 1 ? (unsigned long)lock.lPitch : reader.width;
    if (ok)
        ok = rectangles(context, image, bytes == 1 ? (const unsigned char *)lock.lpSurface : indices, reader.width,
                        reader.height, pitch);
    if (!native(context, bh_renderer_surface_unlock(surface)))
        ok = 0;
    locked = 0;
    if (!ok)
        goto failed;
    if (!bh_pcx_reader_close(&reader))
    {
        fail(context, BH_TEXTURE_IO);
        goto failed;
    }
    if (indices)
        texture_context->allocator.release(indices, texture_context->allocator.user);
    image->next = context->images;
    context->images = image;
    return image;
failed:
    if (locked)
        bh_renderer_surface_unlock(surface);
    if (reader.file)
        bh_pcx_reader_close(&reader);
    if (indices)
        texture_context->allocator.release(indices, texture_context->allocator.user);
    if (surface)
        bh_renderer_surface_release(surface);
    if (image)
        texture_context->allocator.release(image, texture_context->allocator.user);
    return NULL;
}
