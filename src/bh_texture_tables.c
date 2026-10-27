#include "bh_texture.h"

int __cdecl bh_texture_bind_display(BHTextureContext *q, const DDPIXELFORMAT *pixel)
{
    unsigned long masks[3], i, mask, low, bits;
    if (!q || !pixel || !(pixel->dwFlags & DDPF_RGB) || pixel->dwRGBBitCount > 16 || q->tables.initialized)
        return 0;
    masks[0] = pixel->dwRBitMask;
    masks[1] = pixel->dwGBitMask;
    masks[2] = pixel->dwBBitMask;
    for (i = 0; i < 3; ++i)
    {
        mask = masks[i];
        if (!mask)
            return 0;
        low = bits = 0;
        while (!(mask & 1))
        {
            ++low;
            mask >>= 1;
        }
        while (mask & 1)
        {
            ++bits;
            mask >>= 1;
        }
        if (mask || bits > 8)
            return 0;
        q->display_right[i] = 8 - bits;
        q->display_left[i] = low;
    }
    return 1;
}
int __cdecl bh_texture_tables_init(BHTextureContext *q)
{
    static const unsigned long steps[4] = {4, 6, 7, 8};
    unsigned long i, j;
    BHTextureTables *t;
    if (!q)
        return 0;
    t = &q->tables;
    if (t->initialized)
        return 1;
    t->initialized = 1;
    for (i = 0; i < 256; ++i)
    {
        for (j = 0; j < 4; ++j)
            t->quant[j][i] = (i * steps[j]) / 256;
        for (j = 0; j < 3; ++j)
        {
            t->right[j][i] = i >> (q->display_right[j] & 31);
            t->left[j][i] = i << (q->display_right[j] & 31);
        }
    }
    for (j = 0; j < 4; ++j)
        for (i = 0; i < steps[j]; ++i)
            t->levels[j][i] = (i * 255) / (steps[j] - 1);
    return 1;
}
static unsigned long clipped(long value)
{
    return value < 0 ? 0 : value > 255 ? 255 : (unsigned long)value;
}
int __cdecl bh_texture_palette_tables(BHTextureContext *q, BHTextureImage *image, const unsigned char *palette,
                                      unsigned long bytes)
{
    static const unsigned long phase[4] = {0, 1, 3, 2};
    unsigned long i, j, k, stride, original[3], value[3], quant[3], packed;
    BHTextureTables *t;
    if (!q || !image || !image->format)
        return 0;
    if (!q->software || image->format->pixel.dwRGBBitCount >= 9)
        return 1;
    stride = bytes > 768 ? 4 : 3;
    if (!palette || bytes < 256 * stride)
    {
        q->error = BH_TEXTURE_ARGUMENT;
        return 0;
    }
    if (!image->dither)
        image->dither = (unsigned char *)q->allocator.allocate(1024, q->allocator.user);
    if (!image->dither)
    {
        q->error = BH_TEXTURE_MEMORY;
        return 0;
    }
    if (!image->screen)
        image->screen = (unsigned short *)q->allocator.allocate(2048, q->allocator.user);
    if (!image->screen)
    {
        q->error = BH_TEXTURE_MEMORY;
        return 0;
    }
    bh_texture_tables_init(q);
    t = &q->tables;
    for (i = 0; i < 256; ++i)
    {
        for (j = 0; j < 3; ++j)
            original[j] = value[j] = palette[i * stride + j];
        for (k = 0; k < 4; ++k)
        {
            quant[0] = t->quant[1][value[0]];
            quant[1] = t->quant[2][value[1]];
            quant[2] = t->quant[1][value[2]];
            image->dither[phase[k] * 256 + i] = (unsigned char)((quant[0] * 7 + quant[1]) * 6 + quant[2]);
            if (k != 3)
                for (j = 0; j < 3; ++j)
                    value[j] = clipped((long)(original[j] * 2) - (long)t->levels[j == 1 ? 2 : 1][quant[j]]);
        }
        packed = 0;
        for (j = 0; j < 3; ++j)
            packed += t->right[j][original[j]] << (q->display_left[j] & 31);
        for (k = 0; k < 4; ++k)
            image->screen[k * 256 + i] = (unsigned short)packed;
    }
    return 1;
}
