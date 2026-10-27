#include "bh_atlas_image.h"
#include "bh_pcx_reader.h"
#include <stddef.h>
#include <string.h>
typedef char atlas_size[sizeof(BHAtlasImage) == 0x13ac ? 1 : -1];
typedef char atlas_palette[offsetof(BHAtlasImage, palette) == 0x84 ? 1 : -1];
typedef char atlas_format[offsetof(BHAtlasImage, pixel) == 0x384 ? 1 : -1];
typedef char atlas_rectangles[offsetof(BHAtlasImage, rectangles) == 0x3a8 ? 1 : -1];
typedef char atlas_surface[offsetof(BHAtlasImage, surface) == 0x13a8 ? 1 : -1];
static int fail(BHAtlasContext *c, BHTextureError error)
{
    if (c)
        c->error = error;
    return 0;
}
static int native(BHAtlasContext *c, HRESULT hr)
{
    c->last_hresult = hr;
    return hr == DD_OK ? 1 : fail(c, BH_TEXTURE_BACKEND);
}
int __cdecl bh_atlas_context_init(BHAtlasContext *c, BHTextureContext *q, unsigned long step)
{
    if (!c || c->images || !q || !q->draw || !q->allocator.allocate || !q->allocator.release)
        return 0;
    memset(c, 0, sizeof(*c));
    c->textures = q;
    c->scan_step = step;
    return 1;
}
void __cdecl bh_atlas_context_dispose(BHAtlasContext *c)
{
    BHAtlasImage *p, *next;
    if (!c)
        return;
    if (c->textures && c->textures->allocator.release)
        for (p = c->images; p; p = next)
        {
            next = p->next;
            if (p->surface)
                IDirectDrawSurface_Release(p->surface);
            c->textures->allocator.release(p, c->textures->allocator.user);
        }
    memset(c, 0, sizeof(*c));
}
static int format(const DDPIXELFORMAT *p)
{
    unsigned long masks[3], i, mask, bits, used = 0;
    if (!p || p->dwSize != sizeof(*p))
        return 0;
    if (p->dwRGBBitCount == 8)
        return (p->dwFlags & DDPF_PALETTEINDEXED8) != 0;
    if (!(p->dwFlags & DDPF_RGB) || (p->dwRGBBitCount != 16 && p->dwRGBBitCount != 24 && p->dwRGBBitCount != 32))
        return 0;
    masks[0] = p->dwRBitMask;
    masks[1] = p->dwGBitMask;
    masks[2] = p->dwBBitMask;
    for (i = 0; i < 3; ++i)
    {
        mask = masks[i];
        if (!mask || (mask & 0x80000000UL) || (mask & used) ||
            (p->dwRGBBitCount < 32 && mask >= (1UL << p->dwRGBBitCount)))
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
static unsigned long packed(const unsigned char *rgb, const DDPIXELFORMAT *p)
{
    unsigned long masks[3], i, value = 0;
    long mask;
    int shift;
    masks[0] = p->dwRBitMask;
    masks[1] = p->dwGBitMask;
    masks[2] = p->dwBBitMask;
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
static int rectangles(BHAtlasContext *c, BHAtlasImage *image, const unsigned char *data, unsigned long width,
                      unsigned long height, unsigned long pitch)
{
    unsigned long top = 0, bottom, left, right, x, rows, extent = pitch * height;
    unsigned char transparent = data[0];
    BHAtlasRectangle *r;
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
        if (c->scan_step && (rows - 1) > (extent - top * pitch - width) / c->scan_step)
            return fail(c, BH_TEXTURE_TOPOLOGY);
        x = 0;
        while (x < width)
        {
            while (x < width && column_blank(data + top * pitch + x, rows, c->scan_step, transparent))
                ++x;
            if (x == width)
                break;
            left = x;
            while (x < width && !column_blank(data + top * pitch + x, rows, c->scan_step, transparent))
                ++x;
            right = x;
            if (image->rectangle_count == 512)
                return fail(c, BH_TEXTURE_TOPOLOGY);
            r = image->rectangles + image->rectangle_count++;
            r->x = (unsigned short)left;
            r->y = (unsigned short)top;
            r->width = (unsigned short)(right - left);
            r->height = (unsigned short)(bottom - top);
        }
        top = bottom;
    }
    return 1;
}
BHAtlasImage *__cdecl bh_atlas_image_load(BHAtlasContext *c, const char *name, const DDPIXELFORMAT *pixel)
{
    BHTextureContext *q;
    BHAtlasImage *image, *slow, *fast;
    BHPcxReader reader;
    BHPcxPalette palette;
    DDSURFACEDESC desc, lock;
    LPDIRECTDRAWSURFACE surface = NULL;
    unsigned char *indices = NULL, index;
    unsigned long colors[256], i, x, y, bytes, pitch;
    int locked = 0, ok = 1;
    if (!c || !(q = c->textures) || !q->draw || !q->allocator.allocate || !q->allocator.release || !name || !name[0] ||
        strlen(name) >= 128)
    {
        fail(c, BH_TEXTURE_ARGUMENT);
        return NULL;
    }
    c->error = BH_TEXTURE_OK;
    c->last_hresult = DD_OK;
    slow = fast = c->images;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
        {
            fail(c, BH_TEXTURE_TOPOLOGY);
            return NULL;
        }
    }
    for (image = c->images; image; image = image->next)
        if (!_stricmp(name, image->name))
            return image;
    if (!format(pixel))
    {
        fail(c, BH_TEXTURE_FORMAT);
        return NULL;
    }
    image = NULL;
    bh_pcx_reader_init(&reader);
    if (!bh_pcx_reader_open(&reader, name))
    {
        fail(c, BH_TEXTURE_IO);
        return NULL;
    }
    if (!reader.width || !reader.height || reader.width > 4096 || reader.height > 4096)
    {
        fail(c, BH_TEXTURE_FORMAT);
        goto failed;
    }
    if (!bh_pcx_palette(&palette, reader.palette, q->palette_mode, q->palette_color))
    {
        fail(c, BH_TEXTURE_FORMAT);
        goto failed;
    }
    memcpy(reader.map, palette.indices, 256);
    bytes = (pixel->dwRGBBitCount + 7) / 8;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x1007;
    desc.dwWidth = reader.width;
    desc.dwHeight = reader.height;
    desc.ddpfPixelFormat = *pixel;
    desc.ddsCaps.dwCaps = 0x840;
    if (!native(c, IDirectDraw_CreateSurface(q->draw, &desc, &surface, NULL)))
        goto failed;
    image = (BHAtlasImage *)q->allocator.allocate(sizeof(*image), q->allocator.user);
    if (!image)
    {
        fail(c, BH_TEXTURE_MEMORY);
        goto failed;
    }
    memset(image, 0, sizeof(*image));
    strcpy(image->name, name);
    memcpy(image->palette, palette.rgb, 768);
    image->pixel = *pixel;
    image->surface = surface;
    memset(&lock, 0, sizeof(lock));
    lock.dwSize = sizeof(lock);
    if (!native(c, IDirectDrawSurface_Lock(surface, NULL, &lock, 0, NULL)))
        goto failed;
    locked = 1;
    if (!lock.lpSurface || lock.lPitch < (long)(reader.width * bytes))
    {
        fail(c, BH_TEXTURE_TOPOLOGY);
        goto failed;
    }
    if (bytes > 1)
    {
        for (i = 0; i < 256; ++i)
            colors[i] = packed(palette.rgb + i * 3, pixel);
        indices =
            (unsigned char *)q->allocator.allocate((unsigned long)reader.width * reader.height, q->allocator.user);
        if (!indices)
        {
            fail(c, BH_TEXTURE_MEMORY);
            goto failed;
        }
    }
    for (y = 0; ok && y < reader.height; ++y)
        for (x = 0; x < reader.width; ++x)
        {
            unsigned char *out = (unsigned char *)lock.lpSurface + y * lock.lPitch + x * bytes;
            if (!bh_pcx_reader_next(&reader, &index))
            {
                ok = fail(c, BH_TEXTURE_IO);
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
        ok = rectangles(c, image, bytes == 1 ? (const unsigned char *)lock.lpSurface : indices, reader.width,
                        reader.height, pitch);
    if (!native(c, IDirectDrawSurface_Unlock(surface, NULL)))
        ok = 0;
    locked = 0;
    if (!ok)
        goto failed;
    if (!bh_pcx_reader_close(&reader))
    {
        fail(c, BH_TEXTURE_IO);
        goto failed;
    }
    if (indices)
        q->allocator.release(indices, q->allocator.user);
    image->next = c->images;
    c->images = image;
    return image;
failed:
    if (locked)
        IDirectDrawSurface_Unlock(surface, NULL);
    if (reader.file)
        bh_pcx_reader_close(&reader);
    if (indices)
        q->allocator.release(indices, q->allocator.user);
    if (surface)
        IDirectDrawSurface_Release(surface);
    if (image)
        q->allocator.release(image, q->allocator.user);
    return NULL;
}
