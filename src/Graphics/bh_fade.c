#include "bh_fade.h"
#include "Textures/bh_texture_draw.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include "bh_renderer.h"
#ifdef BH_SDL_BACKEND
#include "../Platform/bh_platform.h"
#endif
static int native_call(BHFadeContext *context, HRESULT h)
{
    ++context->render->native_calls;
    context->last_hresult = context->render->last_hresult = h;
    if (FAILED(h))
    {
        ++context->failures;
        ++context->render->failures;
        return 0;
    }
    return 1;
}
void bh_fade_set(BHFadeContext *context, float amount, unsigned long color)
{
    if (context)
    {
        memcpy(&context->amount, &amount, 4);
        context->color = color;
    }
}
int bh_fade_queue(BHDrawQueue *queue, float amount, unsigned long color)
{
    BHDrawCommand *command;
    if (!queue)
        return -1;
    command = bh_queue_reserve(queue, 8);
    if (!command)
        return queue->error == BH_DRAW_FULL ? 0 : -1;
    command->kind = BH_DRAW_FADE;
    memcpy(&command->resource, &amount, 4);
    command->words[0] = color;
    return 1;
}
int bh_fade_execute(BHFadeContext *c, const BHDrawCommand *command, unsigned long *next)
{
    float f;
    if (!c || !command || !next || command->kind != BH_DRAW_FADE)
        return 0;
    memcpy(&f, &command->resource, 4);
    bh_fade_set(c, f, command->words[0]);
    *next = command->next;
    return 1;
}
void bh_fade_dispose(BHFadeContext *context)
{
    if (context)
    {
        free(context->table);
        context->table = 0;
        context->table_bytes = context->table_ready = 0;
    }
}
static long tint(unsigned long color, unsigned shift, float amount)
{
    long v = (long)color;
    unsigned long bias = v < 0 ? ((1UL << shift) - 1) : 0;
    return (long)((double)(((long)(color + bias) >> shift) & 255) * amount);
}
int bh_fade_apply(BHFadeContext *context)
{
    BHTextureContext *texture_context;
    D3DTLVERTEX v[4];
    unsigned long i, j, color, need;
    long x, y, r, g, b;
    double keep;
    unsigned char *row;
    unsigned short *row16, *table16;
    int ok = 1;
    if (!context || !context->render || !(texture_context = context->render->textures))
        return 0;
    keep = 1.0 - (double)context->amount;
    if (!texture_context->software)
    {
        if (!texture_context->device || context->near_plane == 0)
            return 0;
        if (!bh_render_mode(context->render, 4))
            ok = 0;
        color = context->color - ((unsigned long)(long)((double)context->amount * -255.0) << 24);
        memset(v, 0, sizeof(v));
        for (i = 0; i < 4; ++i)
        {
            v[i].sz = context->near_plane / context->near_plane;
            v[i].rhw = (float)(1.0 / context->near_plane);
            v[i].color = color;
        }
        v[1].sy = v[2].sy = (float)context->height;
        v[2].sx = v[3].sx = (float)context->width;
#ifdef BH_SDL_BACKEND
        {
            float mx = 0, my = 0;
            bh_sdl_canvas_margins(&mx, &my);
            v[0].sx = v[1].sx = -mx;
            v[2].sx = v[3].sx = context->width + mx;
            v[0].sy = v[3].sy = -my;
            v[1].sy = v[2].sy = context->height + my;
        }
#endif
        if (!native_call(context, bh_renderer_set_state(texture_context->device, D3DRENDERSTATE_TEXTUREHANDLE, 0)))
            ok = 0;
        ++context->native_draws;
        if (!native_call(context, bh_renderer_draw_fan(texture_context->device, v, 4)))
            ok = 0;
        if (!bh_render_mode(context->render, 0))
            ok = 0;
        return ok;
    }
    r = tint(context->color, 16, context->amount);
    g = tint(context->color, 8, context->amount);
    b = (long)((double)(context->color & 255) * context->amount);
    if (context->pixel_bytes != 1 && context->pixel_bytes != 2)
        return 1;
    if (context->width > 0 && context->height > 0 && (!context->pixels || context->pitch < context->width * (long)context->pixel_bytes))
        return 0;
    need = context->pixel_bytes == 1 ? 256 : 131072;
    if (!context->table)
    {
        context->table = malloc(need);
        if (!context->table)
            return 0;
        context->table_bytes = need;
        context->table_ready = 0;
    }
    if (context->table_bytes < need)
        return 0;
    if (context->amount != context->cached_amount || context->color != context->cached_color)
    {
        /* Captured keep/tints belong to pre-copy active state. */
        context->amount = context->cached_amount;
        context->color = context->cached_color;
        if (context->pixel_bytes == 1)
        {
            for (i = 0; i < 256; ++i)
            {
                long rr = (long)((double)r - ((double)(i / 42 * 255) * keep) * -.2);
                long gg = (long)((double)g - ((double)((i % 42) / 6 * 255) * keep) * (-1.0 / 6.0));
                long bb = (long)((double)b - ((double)((i % 42) % 6 * 255) * keep) * -.2);
                ((unsigned char *)context->table)[i] =
                    (unsigned char)(((rr * 5 / 255 * 7) + (gg * 6 / 255)) * 6 + bb * 5 / 255);
            }
        }
        else
        {
            table16 = (unsigned short *)context->table;
            for (i = 0; i < 65536; ++i)
            {
                unsigned long packed = 0;
                long q;
                double add;
                for (j = 0; j < 3; ++j)
                {
                    q = (long)((unsigned long)((long)(i & context->masks[j]) >> (context->extract[j] & 31)) << (context->expand[j] & 31));
                    add = j == 0 ? r : j == 1 ? g : b;
                    q = (long)((double)q * keep + add);
                    packed += ((unsigned long)(q >> (context->expand[j] & 31)) << (context->extract[j] & 31));
                }
                table16[i] = (unsigned short)packed;
            }
        }
        context->table_ready = 1;
        ++context->rebuilds;
    }
    if (!context->table_ready)
        return 0;
    for (y = 0; y < context->height; ++y)
    {
        row = context->pixels + y * context->pitch;
        if (context->pixel_bytes == 1)
            for (x = 0; x < context->width; ++x)
            {
                row[x] = ((unsigned char *)context->table)[row[x]];
                ++context->pixels_changed;
            }
        else
        {
            row16 = (unsigned short *)row;
            for (x = 0; x < context->width; ++x)
            {
                row16[x] = ((unsigned short *)context->table)[row16[x]];
                ++context->pixels_changed;
            }
        }
    }
    return 1;
}
