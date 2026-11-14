#include "../bh_renderer.h"
#include "bh_texture.h"
#include "../../Assets/bh_pcx_reader.h"
#include <string.h>
#include <ctype.h>

static int fail(BHTextureContext *texture_context, BHTextureError error)
{
    if (texture_context)
        texture_context->error = error;
    return 0;
}
static int native(BHTextureContext *texture_context, HRESULT hr)
{
    texture_context->last_hresult = hr;
    return hr == DD_OK ? 1 : fail(texture_context, BH_TEXTURE_BACKEND);
}
int bh_texture_set_path(BHTextureContext *texture_context, const char *prefix, long policy)
{
    unsigned long n;
    if (!texture_context || !prefix || (n = strlen(prefix)) > 254)
        return fail(texture_context, BH_TEXTURE_ARGUMENT);
    strcpy(texture_context->prefix, prefix);
    texture_context->search = -1;
    if (n)
    {
        if (prefix[n - 1] != '\\')
        {
            texture_context->prefix[n] = '\\';
            texture_context->prefix[n + 1] = 0;
        }
        if (policy >= 0 && policy < 3)
            texture_context->search = policy;
    }
    return 1;
}
static void message(BHTextureContext *texture_context, const char *tag, const char *name, int deduplicate)
{
    unsigned long used = strlen(texture_context->errors), a = strlen(tag), b = strlen(name);
    if (deduplicate && strstr(texture_context->errors, name))
        return;
    if (used + a + b < sizeof(texture_context->errors))
    {
        memcpy(texture_context->errors + used, tag, a);
        memcpy(texture_context->errors + used + a, name, b + 1);
    }
}
static int canonicalize_texture_name(const char *name, char *out)
{
    unsigned long i, n;
    if (!name || !(n = strlen(name)) || n >= 28)
        return 0;
    for (i = 0; i <= n; ++i)
        out[i] = (char)toupper((unsigned char)name[i]);
    if (n >= 4 && (!_stricmp(out + n - 4, ".BMP") || !_stricmp(out + n - 4, ".GIF") || !_stricmp(out + n - 4, ".TGA") ||
                   !_stricmp(out + n - 4, ".JPG")))
        strcpy(out + n - 4, ".PCX");
    return 1;
}
static int open_reader(BHTextureContext *texture_context, BHPcxReader *reader, const char *name)
{
    char path[284];
    strcpy(path, texture_context->prefix);
    strcat(path, name);
    switch (texture_context->search)
    {
    case -1:
        return bh_pcx_reader_open(reader, name);
    case 0:
        return bh_pcx_reader_open(reader, path) || bh_pcx_reader_open(reader, name);
    case 1:
        return bh_pcx_reader_open(reader, name) || bh_pcx_reader_open(reader, path);
    case 2:
        return bh_pcx_reader_open(reader, path);
    default:
        return 0;
    }
}
static unsigned long packed(const unsigned char *rgb, const DDPIXELFORMAT *format)
{
    unsigned long masks[3], i, mask, out = 0;
    int shift;
    masks[0] = format->dwRBitMask;
    masks[1] = format->dwGBitMask;
    masks[2] = format->dwBBitMask;
    for (i = 0; i < 3; ++i)
    {
        shift = -8;
        for (mask = masks[i]; mask; mask /= 2)
            ++shift;
        out += ((shift < 0 ? (unsigned long)rgb[i] >> (-shift) : (unsigned long)rgb[i] << shift) & masks[i]);
    }
    return out;
}
static void discard(BHTextureContext *texture_context, BHTextureImage *image)
{
    if (!image)
        return;
    if (image->palette)
        bh_renderer_palette_release(image->palette);
    if (image->source)
        bh_renderer_surface_release(image->source);
    if (image->runs)
        texture_context->allocator.release(image->runs, texture_context->allocator.user);
    if (image->vertices)
        texture_context->allocator.release(image->vertices, texture_context->allocator.user);
    if (image->dither)
        texture_context->allocator.release(image->dither, texture_context->allocator.user);
    if (image->screen)
        texture_context->allocator.release(image->screen, texture_context->allocator.user);
    texture_context->allocator.release(image, texture_context->allocator.user);
}
BHTextureImage *bh_texture_load(BHTextureContext *texture_context, const char *name)
{
    char key[28];
    BHTextureImage *image;
    BHPcxReader reader;
    BHPcxPalette palette;
    DDSURFACEDESC desc, lock;
    PALETTEENTRY entries[256];
    unsigned long colors[256], i, j, x, y, bytes;
    unsigned char index;
    int ok = 1;
    if (!texture_context || !texture_context->allocator.allocate || !texture_context->draw || !canonicalize_texture_name(name, key))
    {
        fail(texture_context, BH_TEXTURE_ARGUMENT);
        return NULL;
    }
    texture_context->error = BH_TEXTURE_OK;
    for (image = texture_context->images; image; image = image->next)
        if (!_stricmp(key, image->name))
            return image;
    image = (BHTextureImage *)texture_context->allocator.allocate(sizeof(*image), texture_context->allocator.user);
    if (!image)
    {
        fail(texture_context, BH_TEXTURE_MEMORY);
        return NULL;
    }
    memset(image, 0, sizeof(*image));
    strcpy(image->name, key);
    image->flags = 1;
    bh_pcx_reader_init(&reader);
    if (!open_reader(texture_context, &reader, key))
    {
        fail(texture_context, BH_TEXTURE_IO);
        message(texture_context, "\nLOADERR:", key, 1);
        goto failed;
    }
    bh_pcx_palette(&palette, reader.palette, texture_context->palette_mode, texture_context->palette_color);
    memcpy(reader.map, palette.indices, 256);
    image->format = bh_texture_request_format(texture_context, reader.width, reader.height, 8, 1);
    if (!image->format)
    {
        message(texture_context, "\nFORMATERR:", key, 0);
        goto failed;
    }
    bytes = (image->format->pixel.dwRGBBitCount + 7) / 8;
    if (bytes < 1 || bytes > 4)
    {
        fail(texture_context, BH_TEXTURE_FORMAT);
        message(texture_context, "\nFORMATERR:", key, 0);
        goto failed;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x1007;
    desc.dwWidth = reader.width;
    desc.dwHeight = reader.height;
    desc.ddpfPixelFormat = image->format->pixel;
    desc.ddsCaps.dwCaps = texture_context->software ? 0x840 : 0x1800;
    if (!native(texture_context, bh_renderer_surface_create(texture_context->draw, &desc, &image->source)))
    {
        message(texture_context, "\nALLOCERR:", key, 0);
        goto failed;
    }
    if (bytes == 1)
    {
        for (i = 0; i < 256; ++i)
        {
            entries[i].peRed = palette.rgb[i * 3];
            entries[i].peGreen = palette.rgb[i * 3 + 1];
            entries[i].peBlue = palette.rgb[i * 3 + 2];
            entries[i].peFlags = 0;
        }
        if (!native(texture_context, bh_renderer_palette_create(texture_context->draw, entries, &image->palette)) ||
            !native(texture_context, bh_renderer_surface_set_palette(image->source, image->palette)))
            goto failed;
        if (!bh_texture_palette_tables(texture_context, image, (const unsigned char *)entries, 1024))
            goto failed;
    }
    else
        for (i = 0; i < 256; ++i)
            colors[i] = packed(palette.rgb + i * 3, &image->format->pixel);
    memset(&lock, 0, sizeof(lock));
    lock.dwSize = sizeof(lock);
    if (!native(texture_context, bh_renderer_surface_lock(image->source, &lock)))
        goto failed;
    if (lock.lPitch < (long)(reader.width * bytes))
        ok = fail(texture_context, BH_TEXTURE_TOPOLOGY);
    for (y = 0; ok && y < reader.height; ++y)
        for (x = 0; x < reader.width; ++x)
        {
            unsigned char *out = (unsigned char *)lock.lpSurface + y * lock.lPitch + x * bytes;
            if (!bh_pcx_reader_next(&reader, &index))
            {
                ok = fail(texture_context, BH_TEXTURE_IO);
                break;
            }
            if (bytes == 1)
                *out = index;
            else
                for (j = 0; j < bytes; ++j)
                    out[j] = (unsigned char)(colors[index] >> (j * 8));
        }
    if (!native(texture_context, bh_renderer_surface_unlock(image->source)))
        ok = 0;
    if (!ok)
        goto failed;
    if (!bh_pcx_reader_close(&reader))
    {
        fail(texture_context, BH_TEXTURE_IO);
        goto failed;
    }
    image->runs = (unsigned long *)texture_context->allocator.allocate(0x80, texture_context->allocator.user);
    image->vertices = (D3DTLVERTEX *)texture_context->allocator.allocate(0xc00, texture_context->allocator.user);
    if (!image->runs || !image->vertices)
    {
        fail(texture_context, BH_TEXTURE_MEMORY);
        goto failed;
    }
    image->vertex_capacity = 0x60;
    image->next = texture_context->images;
    texture_context->images = image;
    return image;
failed:
    if (reader.file)
        bh_pcx_reader_close(&reader);
    discard(texture_context, image);
    return NULL;
}
