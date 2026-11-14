#include "../../Platform/bh_platform_clock.h"
#include "bh_loading_image.h"
#include <stdlib.h>
#include <string.h>
#define ARENA_BYTES 0x12c000UL
static void *allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void release(void *memory, void *user)
{
    free(memory);
}
static int fail(BHLoadingImages *loading_images, BHLoadingImageError error)
{
    if (loading_images)
    {
        loading_images->error = error;
        ++loading_images->failures;
    }
    return 0;
}
int bh_loading_images_bind(BHLoadingImages *loading_images, unsigned long width, unsigned long bytes, const unsigned long *right,
                           const unsigned long *left, const BHSceneAllocator *allocator)
{
    if (!loading_images || (bytes != 1 && (!right || !left)) || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(loading_images, 0, sizeof(*loading_images));
    loading_images->display_width = width;
    loading_images->pixel_bytes = bytes;
    if (right && left)
    {
        memcpy(loading_images->right, right, sizeof(loading_images->right));
        memcpy(loading_images->left, left, sizeof(loading_images->left));
    }
    if (allocator)
        loading_images->allocator = *allocator;
    else
    {
        loading_images->allocator.allocate = allocate;
        loading_images->allocator.release = release;
    }
    return 1;
}
void bh_loading_images_dispose(BHLoadingImages *loading_images)
{
    if (!loading_images)
        return;
    if (loading_images->arena && loading_images->allocator.release)
        loading_images->allocator.release(loading_images->arena, loading_images->allocator.user);
    loading_images->arena = NULL;
    loading_images->arena_bytes = 0;
}
static int pixel(BHLoadingImages *loading_images, BHPcxReader *reader, unsigned char *value)
{
    if (!bh_pcx_reader_next(reader, value))
        return 0;
    ++loading_images->pixels_read;
    return 1;
}
static unsigned long packed(BHLoadingImages *loading_images, const unsigned char *rgb, int quarter)
{
    unsigned long i, value = 0, component;
    for (i = 0; i < 3; ++i)
    {
        component = rgb[i];
        if (quarter)
            component >>= 2;
        value += (component >> (loading_images->right[i] & 31)) << (loading_images->left[i] & 31);
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
int bh_loading_image_load(BHLoadingImages *loading_images, const char *name, unsigned long offset)
{
    BHPcxReader reader;
    BHPcxPalette palette;
    unsigned long bytes, rows, row, x, at, i;
    unsigned char value, unused;
    int ok = 1;
    if (!loading_images)
        return 0;
    loading_images->error = BH_LOADING_IMAGE_OK;
    if (!loading_images->arena || !name)
        return fail(loading_images, BH_LOADING_IMAGE_ARGUMENT);
    bh_pcx_reader_init(&reader);
    if (!bh_pcx_reader_open(&reader, name))
        return fail(loading_images, BH_LOADING_IMAGE_IO);
    if (reader.width != 640)
    {
        bh_pcx_reader_close(&reader);
        return fail(loading_images, BH_LOADING_IMAGE_DIMENSION);
    }
    rows = loading_images->display_width == 640 ? reader.height : reader.height / 2;
    bytes = rows * (loading_images->display_width == 640 ? 640UL : 320UL);
    if (offset > loading_images->arena_bytes || bytes > loading_images->arena_bytes - offset)
    {
        bh_pcx_reader_close(&reader);
        return fail(loading_images, BH_LOADING_IMAGE_SPAN);
    }
    if (!bh_pcx_palette(&palette, reader.palette, 0, 0))
    {
        bh_pcx_reader_close(&reader);
        return fail(loading_images, BH_LOADING_IMAGE_STREAM);
    }
    memcpy(reader.map, palette.indices, 256);
    at = offset;
    if (loading_images->display_width == 640)
    {
        for (i = 0; i < bytes; ++i)
        {
            if (!pixel(loading_images, &reader, &value))
            {
                ok = 0;
                break;
            }
            loading_images->arena[at++] = value;
            ++loading_images->pixels_written;
        }
    }
    else
    {
        for (row = 0; row < rows && ok; ++row)
        {
            for (x = 0; x < 320; ++x)
            {
                if (!pixel(loading_images, &reader, &value))
                {
                    ok = 0;
                    break;
                }
                loading_images->arena[at++] = value;
                ++loading_images->pixels_written;
                if (!pixel(loading_images, &reader, &unused))
                {
                    ok = 0;
                    break;
                }
            }
            for (x = 0; x < 640 && ok; ++x)
                if (!pixel(loading_images, &reader, &unused))
                    ok = 0;
        }
    }
    loading_images->last_height = reader.height;
    loading_images->last_offset = offset;
    if (!bh_pcx_reader_close(&reader))
        ok = 0;
    if (!ok)
        return fail(loading_images, BH_LOADING_IMAGE_STREAM);
    /* Source close clears reader globals; retained palette is the original
       process-global palette that remains live after0044f1b0. */
    memcpy(loading_images->palette, palette.rgb, 768);
    memcpy(loading_images->map, palette.indices, 256);
    loading_images->frame_stride = loading_images->display_width == 640 ? 0x4b000UL : 0x12c00UL;
    for (i = 0; i < 256; ++i)
    {
        if (loading_images->pixel_bytes == 1)
        {
            loading_images->indexed[i] = indexed(palette.rgb + i * 3, 0);
            loading_images->quarter_indexed[i] = indexed(palette.rgb + i * 3, 1);
        }
        else
        {
            loading_images->native[i] = packed(loading_images, palette.rgb + i * 3, 0);
            loading_images->quarter_native[i] = packed(loading_images, palette.rgb + i * 3, 1);
        }
    }
    ++loading_images->loads;
    return 1;
}
int bh_loading_images_init(BHLoadingImages *loading_images)
{
    BHLoadingImages next;
    BHLoadingImageError error;
    if (!loading_images || !loading_images->allocator.allocate || !loading_images->allocator.release)
        return fail(loading_images, BH_LOADING_IMAGE_ARGUMENT);
    next = *loading_images;
    next.arena = (unsigned char *)loading_images->allocator.allocate(ARENA_BYTES, loading_images->allocator.user);
    next.arena_bytes = ARENA_BYTES;
    if (!next.arena)
        return fail(loading_images, BH_LOADING_IMAGE_MEMORY);
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
    next.deadline = (unsigned long)bh_platform_ticks() + 5000UL;
    next.redraw_count = 2;
    next.stage = 0;
    if (loading_images->arena)
        loading_images->allocator.release(loading_images->arena, loading_images->allocator.user);
    *loading_images = next;
    loading_images->error = BH_LOADING_IMAGE_OK;
    return 1;
failed:
    error = next.error;
    loading_images->allocator.release(next.arena, loading_images->allocator.user);
    return fail(loading_images, error);
}
