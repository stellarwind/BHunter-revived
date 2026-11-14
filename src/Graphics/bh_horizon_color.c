#include "bh_horizon_color.h"
#include <string.h>
int bh_horizon_color_init(BHHorizonColor *horizon_color, BHTextureContext *textures)
{
    if (!horizon_color || !textures || !textures->allocator.allocate || !textures->allocator.release)
        return 0;
    memset(horizon_color, 0, sizeof(*horizon_color));
    horizon_color->textures = textures;
    horizon_color->allocator = textures->allocator;
    return 1;
}
void bh_horizon_color_dispose(BHHorizonColor *horizon_color)
{
    if (!horizon_color)
        return;
    if (horizon_color->indexed)
        horizon_color->allocator.release(horizon_color->indexed, horizon_color->allocator.user);
    if (horizon_color->packed)
        horizon_color->allocator.release(horizon_color->packed, horizon_color->allocator.user);
    memset(horizon_color, 0, sizeof(*horizon_color));
}
int bh_horizon_color_prepare(BHHorizonColor *horizon_color, unsigned long mode, unsigned long color)
{
    unsigned long row, i, base[3], constant[3], value[3], packed, j;
    BHTextureContext *texture_context;
    if (!horizon_color || !horizon_color->textures || !horizon_color->allocator.allocate || !horizon_color->allocator.release)
    {
        if (horizon_color)
            horizon_color->error = BH_HORIZON_COLOR_ARGUMENT;
        return 0;
    }
    texture_context = horizon_color->textures;
    if (!bh_texture_tables_init(texture_context))
    {
        horizon_color->error = BH_HORIZON_COLOR_ARGUMENT;
        return 0;
    }
    if (mode == 1)
    {
        if (horizon_color->indexed && color == horizon_color->color)
        {
            horizon_color->error = BH_HORIZON_COLOR_OK;
            return 1;
        }
        if (!horizon_color->indexed)
            horizon_color->indexed = (unsigned char *)horizon_color->allocator.allocate(65536, horizon_color->allocator.user);
        if (!horizon_color->indexed)
        {
            horizon_color->error = BH_HORIZON_COLOR_MEMORY;
            return 0;
        }
    }
    else
    {
        if (horizon_color->packed && color == horizon_color->color)
        {
            horizon_color->error = BH_HORIZON_COLOR_OK;
            return 1;
        }
        if (!horizon_color->packed)
            horizon_color->packed = (unsigned short *)horizon_color->allocator.allocate(131072, horizon_color->allocator.user);
        if (!horizon_color->packed)
        {
            horizon_color->error = BH_HORIZON_COLOR_MEMORY;
            return 0;
        }
    }
    constant[0] = (color >> 16) & 255;
    constant[1] = (color >> 8) & 255;
    constant[2] = color & 255;
    for (row = 0; row < 256; ++row)
        for (i = 0; i < 256; ++i)
        {
            base[0] = (((i / 42) % 6) * 255) / 5;
            base[1] = (((i / 6) % 7) * 255) / 6;
            base[2] = ((i % 6) * 255) / 5;
            for (j = 0; j < 3; ++j)
                value[j] = (base[j] * row + constant[j] * (255 - row)) / 255;
            if (mode == 1)
                horizon_color->indexed[row * 256 + i] =
                    (unsigned char)((texture_context->tables.quant[1][value[0]] * 7 + texture_context->tables.quant[2][value[1]]) * 6 +
                                    texture_context->tables.quant[1][value[2]]);
            else
            {
                packed = 0;
                for (j = 0; j < 3; ++j)
                    packed += (value[j] >> (texture_context->display_right[j] & 31)) << (texture_context->display_left[j] & 31);
                horizon_color->packed[row * 256 + i] = (unsigned short)packed;
            }
        }
    horizon_color->color = color;
    ++horizon_color->fills;
    horizon_color->error = BH_HORIZON_COLOR_OK;
    return 1;
}
