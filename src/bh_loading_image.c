#include "bh_loading_image.h"
#include <stdlib.h>
#include <string.h>
#define ARENA_BYTES 0x12c000UL
static void *__cdecl allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void __cdecl release(void *memory, void *user)
{
    free(memory);
}
static int fail(BHLoadingImages *c, BHLoadingImageError error)
{
    if (c)
    {
        c->error = error;
        ++c->failures;
    }
    return 0;
}
int __cdecl bh_loading_images_bind(BHLoadingImages *c, unsigned long width, unsigned long bytes,
                                   const unsigned long *right, const unsigned long *left,
                                   const BHSceneAllocator *allocator)
{
    if (!c || (bytes != 1 && (!right || !left)) || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(c, 0, sizeof(*c));
    c->display_width = width;
    c->pixel_bytes = bytes;
    if (right && left)
    {
        memcpy(c->right, right, sizeof(c->right));
        memcpy(c->left, left, sizeof(c->left));
    }
    if (allocator)
        c->allocator = *allocator;
    else
    {
        c->allocator.allocate = allocate;
        c->allocator.release = release;
    }
    return 1;
}
void __cdecl bh_loading_images_dispose(BHLoadingImages *c)
{
    if (!c)
        return;
    if (c->arena && c->allocator.release)
        c->allocator.release(c->arena, c->allocator.user);
    c->arena = NULL;
    c->arena_bytes = 0;
}
static int pixel(BHLoadingImages *c, BHPcxReader *reader, unsigned char *value)
{
    if (!bh_pcx_reader_next(reader, value))
        return 0;
    ++c->pixels_read;
    return 1;
}
static unsigned long packed(BHLoadingImages *c, const unsigned char *rgb, int quarter)
{
    unsigned long i, value = 0, component;
    for (i = 0; i < 3; ++i)
    {
        component = rgb[i];
        if (quarter)
            component >>= 2;
        value += (component >> (c->right[i] & 31)) << (c->left[i] & 31);
    }
    return value;
}
static unsigned char indexed(const unsigned char *rgb, int quarter)
{
    unsigned long r = rgb[0], g = rgb[1], b = rgb[2];
    if (quarter)
    {
        r >>= 2;
        g >>= 2;
        b >>= 2;
    }
    return (unsigned char)(((r * 5) / 255) * 42 + ((g * 6) / 255) * 6 + (b * 5) / 255);
}
int __cdecl bh_loading_image_load(BHLoadingImages *c, const char *name, unsigned long offset)
{
    BHPcxReader reader;
    BHPcxPalette palette;
    unsigned long bytes, rows, row, x, at, i;
    unsigned char value, unused;
    int ok = 1;
    if (!c)
        return 0;
    c->error = BH_LOADING_IMAGE_OK;
    if (!c->arena || !name)
        return fail(c, BH_LOADING_IMAGE_ARGUMENT);
    bh_pcx_reader_init(&reader);
    if (!bh_pcx_reader_open(&reader, name))
        return fail(c, BH_LOADING_IMAGE_IO);
    if (reader.width != 640)
    {
        bh_pcx_reader_close(&reader);
        return fail(c, BH_LOADING_IMAGE_DIMENSION);
    }
    rows = c->display_width == 640 ? reader.height : reader.height / 2;
    bytes = rows * (c->display_width == 640 ? 640UL : 320UL);
    if (offset > c->arena_bytes || bytes > c->arena_bytes - offset)
    {
        bh_pcx_reader_close(&reader);
        return fail(c, BH_LOADING_IMAGE_SPAN);
    }
    if (!bh_pcx_palette(&palette, reader.palette, 0, 0))
    {
        bh_pcx_reader_close(&reader);
        return fail(c, BH_LOADING_IMAGE_STREAM);
    }
    memcpy(reader.map, palette.indices, 256);
    at = offset;
    if (c->display_width == 640)
    {
        for (i = 0; i < bytes; ++i)
        {
            if (!pixel(c, &reader, &value))
            {
                ok = 0;
                break;
            }
            c->arena[at++] = value;
            ++c->pixels_written;
        }
    }
    else
    {
        for (row = 0; row < rows && ok; ++row)
        {
            for (x = 0; x < 320; ++x)
            {
                if (!pixel(c, &reader, &value))
                {
                    ok = 0;
                    break;
                }
                c->arena[at++] = value;
                ++c->pixels_written;
                if (!pixel(c, &reader, &unused))
                {
                    ok = 0;
                    break;
                }
            }
            for (x = 0; x < 640 && ok; ++x)
                if (!pixel(c, &reader, &unused))
                    ok = 0;
        }
    }
    c->last_height = reader.height;
    c->last_offset = offset;
    if (!bh_pcx_reader_close(&reader))
        ok = 0;
    if (!ok)
        return fail(c, BH_LOADING_IMAGE_STREAM);
    /* Source close clears reader globals; retained palette is the original
       process-global palette that remains live after0044f1b0. */
    memcpy(c->palette, palette.rgb, 768);
    memcpy(c->map, palette.indices, 256);
    c->frame_stride = c->display_width == 640 ? 0x4b000UL : 0x12c00UL;
    for (i = 0; i < 256; ++i)
    {
        if (c->pixel_bytes == 1)
        {
            c->indexed[i] = indexed(palette.rgb + i * 3, 0);
            c->quarter_indexed[i] = indexed(palette.rgb + i * 3, 1);
        }
        else
        {
            c->native[i] = packed(c, palette.rgb + i * 3, 0);
            c->quarter_native[i] = packed(c, palette.rgb + i * 3, 1);
        }
    }
    ++c->loads;
    return 1;
}
int __cdecl bh_loading_images_init(BHLoadingImages *c)
{
    BHLoadingImages next;
    BHLoadingImageError error;
    if (!c || !c->allocator.allocate || !c->allocator.release)
        return fail(c, BH_LOADING_IMAGE_ARGUMENT);
    next = *c;
    next.arena = (unsigned char *)c->allocator.allocate(ARENA_BYTES, c->allocator.user);
    next.arena_bytes = ARENA_BYTES;
    if (!next.arena)
        return fail(c, BH_LOADING_IMAGE_MEMORY);
    if (!bh_loading_image_load(&next, "textures\\midas.pcx", 0x96000UL))
        goto failed;
    memcpy(next.midas_indexed, next.indexed, 256);
    memcpy(next.midas_native, next.native, 1024);
    if (!bh_loading_image_load(&next, "textures\\iel.pcx", 0xe1000UL))
        goto failed;
    memcpy(next.iel_indexed, next.indexed, 256);
    memcpy(next.iel_native, next.native, 1024);
    if (!bh_loading_image_load(&next, "textures\\bad.pcx", 0))
        goto failed;
    ++next.tick_calls;
    next.deadline = (unsigned long)GetTickCount() + 5000UL;
    next.redraw_count = 2;
    next.stage = 0;
    if (c->arena)
        c->allocator.release(c->arena, c->allocator.user);
    *c = next;
    c->error = BH_LOADING_IMAGE_OK;
    return 1;
failed:
    error = next.error;
    c->allocator.release(next.arena, c->allocator.user);
    return fail(c, error);
}
