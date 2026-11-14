#include "bh_texture.h"

int bh_texture_bind_display(BHTextureContext *texture_context, const DDPIXELFORMAT *pixel)
{
    unsigned long masks[3], i, mask, low, bits;
    if (!texture_context || !pixel || !(pixel->dwFlags & DDPF_RGB) || pixel->dwRGBBitCount > 16 || texture_context->tables.initialized)
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
        texture_context->display_right[i] = 8 - bits;
        texture_context->display_left[i] = low;
    }
    return 1;
}
int bh_texture_tables_init(BHTextureContext *texture_context)
{
    static const unsigned long steps[4] = {4, 6, 7, 8};
    unsigned long i, j;
    BHTextureTables *texture_tables;
    if (!texture_context)
        return 0;
    texture_tables = &texture_context->tables;
    if (texture_tables->initialized)
        return 1;
    texture_tables->initialized = 1;
    for (i = 0; i < 256; ++i)
    {
        for (j = 0; j < 4; ++j)
            texture_tables->quant[j][i] = (i * steps[j]) / 256;
        for (j = 0; j < 3; ++j)
        {
            texture_tables->right[j][i] = i >> (texture_context->display_right[j] & 31);
            texture_tables->left[j][i] = i << (texture_context->display_right[j] & 31);
        }
    }
    for (j = 0; j < 4; ++j)
        for (i = 0; i < steps[j]; ++i)
            texture_tables->levels[j][i] = (i * 255) / (steps[j] - 1);
    return 1;
}
static unsigned long clipped(long value)
{
    return value < 0 ? 0 : value > 255 ? 255 : (unsigned long)value;
}
int bh_texture_palette_tables(BHTextureContext *texture_context, BHTextureImage *image, const unsigned char *palette,
                              unsigned long bytes)
{
    static const unsigned long phase[4] = {0, 1, 3, 2};
    unsigned long i, j, k, stride, original[3], value[3], quant[3], packed;
    BHTextureTables *texture_tables;
    if (!texture_context || !image || !image->format)
        return 0;
    if (!texture_context->software || image->format->pixel.dwRGBBitCount >= 9)
        return 1;
    stride = bytes > 768 ? 4 : 3;
    if (!palette || bytes < 256 * stride)
    {
        texture_context->error = BH_TEXTURE_ARGUMENT;
        return 0;
    }
    if (!image->dither)
        image->dither = (unsigned char *)texture_context->allocator.allocate(1024, texture_context->allocator.user);
    if (!image->dither)
    {
        texture_context->error = BH_TEXTURE_MEMORY;
        return 0;
    }
    if (!image->screen)
        image->screen = (unsigned short *)texture_context->allocator.allocate(2048, texture_context->allocator.user);
    if (!image->screen)
    {
        texture_context->error = BH_TEXTURE_MEMORY;
        return 0;
    }
    bh_texture_tables_init(texture_context);
    texture_tables = &texture_context->tables;
    for (i = 0; i < 256; ++i)
    {
        for (j = 0; j < 3; ++j)
            original[j] = value[j] = palette[i * stride + j];
        for (k = 0; k < 4; ++k)
        {
            quant[0] = texture_tables->quant[1][value[0]];
            quant[1] = texture_tables->quant[2][value[1]];
            quant[2] = texture_tables->quant[1][value[2]];
            image->dither[phase[k] * 256 + i] = (unsigned char)((quant[0] * 7 + quant[1]) * 6 + quant[2]);
            if (k != 3)
                for (j = 0; j < 3; ++j)
                    value[j] = clipped((long)(original[j] * 2) - (long)texture_tables->levels[j == 1 ? 2 : 1][quant[j]]);
        }
        packed = 0;
        for (j = 0; j < 3; ++j)
            packed += texture_tables->right[j][original[j]] << (texture_context->display_left[j] & 31);
        for (k = 0; k < 4; ++k)
            image->screen[k * 256 + i] = (unsigned short)packed;
    }
    return 1;
}
