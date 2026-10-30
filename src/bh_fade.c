#include "bh_fade.h"
#include "bh_texture_draw.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
#ifdef BH_SDL_BACKEND
#include "bh_sdl_adapter.h"
#endif
static int native_call(BHFadeContext *c, HRESULT h)
{
    ++c->render->native_calls;
    c->last_hresult = c->render->last_hresult = h;
    if (FAILED(h))
    {
        ++c->failures;
        ++c->render->failures;
        return 0;
    }
    return 1;
}
void __cdecl bh_fade_set(BHFadeContext *c, float amount, unsigned long color)
{
    if (c)
    {
        memcpy(&c->amount, &amount, 4);
        c->color = color;
    }
}
int __cdecl bh_fade_queue(BHDrawQueue *q, float amount, unsigned long color)
{
    BHDrawCommand *p;
    if (!q)
        return -1;
    p = bh_queue_reserve(q, 8);
    if (!p)
        return q->error == BH_DRAW_FULL ? 0 : -1;
    p->kind = BH_DRAW_FADE;
    memcpy(&p->resource, &amount, 4);
    p->words[0] = color;
    return 1;
}
int __cdecl bh_fade_execute(BHFadeContext *c, const BHDrawCommand *p, unsigned long *next)
{
    float f;
    if (!c || !p || !next || p->kind != BH_DRAW_FADE)
        return 0;
    memcpy(&f, &p->resource, 4);
    bh_fade_set(c, f, p->words[0]);
    *next = p->next;
    return 1;
}
void __cdecl bh_fade_dispose(BHFadeContext *c)
{
    if (c)
    {
        free(c->table);
        c->table = 0;
        c->table_bytes = c->table_ready = 0;
    }
}
static long tint(unsigned long color, unsigned shift, float amount)
{
    long v = (long)color;
    unsigned long bias = v < 0 ? ((1UL << shift) - 1) : 0;
    return (long)((double)(((long)(color + bias) >> shift) & 255) * amount);
}
int __cdecl bh_fade_apply(BHFadeContext *c)
{
    BHTextureContext *t;
    D3DTLVERTEX v[4];
    unsigned long i, j, color, need;
    long x, y, r, g, b;
    double keep;
    unsigned char *row;
    unsigned short *row16, *table16;
    int ok = 1;
    if (!c || !c->render || !(t = c->render->textures))
        return 0;
    keep = 1.0 - (double)c->amount;
    if (!t->software)
    {
        if (!t->device || c->near_plane == 0)
            return 0;
        if (!bh_render_mode(c->render, 4))
            ok = 0;
        color = c->color - ((unsigned long)(long)((double)c->amount * -255.0) << 24);
        memset(v, 0, sizeof(v));
        for (i = 0; i < 4; ++i)
        {
            v[i].sz = c->near_plane / c->near_plane;
            v[i].rhw = (float)(1.0 / c->near_plane);
            v[i].color = color;
        }
        v[1].sy = v[2].sy = (float)c->height;
        v[2].sx = v[3].sx = (float)c->width;
#ifdef BH_SDL_BACKEND
        {
            float mx = 0, my = 0;
            bh_sdl_canvas_margins(&mx, &my);
            /* Bleed past the pixel-center boundary at every output scale. */
            v[0].sx = v[1].sx = -mx - 1;
            v[2].sx = v[3].sx = c->width + mx;
            v[0].sy = v[3].sy = -my - 1;
            v[1].sy = v[2].sy = c->height + my;
        }
#endif
        if (!native_call(c, IDirect3DDevice2_SetRenderState(t->device, D3DRENDERSTATE_TEXTUREHANDLE, 0)))
            ok = 0;
        ++c->native_draws;
        if (!native_call(c, IDirect3DDevice2_DrawPrimitive(t->device, D3DPT_TRIANGLEFAN, D3DVT_TLVERTEX, v, 4, 0xc)))
            ok = 0;
        if (!bh_render_mode(c->render, 0))
            ok = 0;
        return ok;
    }
    r = tint(c->color, 16, c->amount);
    g = tint(c->color, 8, c->amount);
    b = (long)((double)(c->color & 255) * c->amount);
    if (c->pixel_bytes != 1 && c->pixel_bytes != 2)
        return 1;
    if (c->width > 0 && c->height > 0 && (!c->pixels || c->pitch < c->width * (long)c->pixel_bytes))
        return 0;
    need = c->pixel_bytes == 1 ? 256 : 131072;
    if (!c->table)
    {
        c->table = malloc(need);
        if (!c->table)
            return 0;
        c->table_bytes = need;
        c->table_ready = 0;
    }
    if (c->table_bytes < need)
        return 0;
    if (c->amount != c->cached_amount || c->color != c->cached_color)
    {
        /* Captured keep/tints belong to pre-copy active state. */
        c->amount = c->cached_amount;
        c->color = c->cached_color;
        if (c->pixel_bytes == 1)
        {
            for (i = 0; i < 256; ++i)
            {
                long rr = (long)((double)r - ((double)(i / 42 * 255) * keep) * -.2);
                long gg = (long)((double)g - ((double)((i % 42) / 6 * 255) * keep) * (-1.0 / 6.0));
                long bb = (long)((double)b - ((double)((i % 42) % 6 * 255) * keep) * -.2);
                ((unsigned char *)c->table)[i] =
                    (unsigned char)(((rr * 5 / 255 * 7) + (gg * 6 / 255)) * 6 + bb * 5 / 255);
            }
        }
        else
        {
            table16 = (unsigned short *)c->table;
            for (i = 0; i < 65536; ++i)
            {
                unsigned long packed = 0;
                long q;
                double add;
                for (j = 0; j < 3; ++j)
                {
                    q = (long)((unsigned long)((long)(i & c->masks[j]) >> (c->extract[j] & 31)) << (c->expand[j] & 31));
                    add = j == 0 ? r : j == 1 ? g : b;
                    q = (long)((double)q * keep + add);
                    packed += ((unsigned long)(q >> (c->expand[j] & 31)) << (c->extract[j] & 31));
                }
                table16[i] = (unsigned short)packed;
            }
        }
        c->table_ready = 1;
        ++c->rebuilds;
    }
    if (!c->table_ready)
        return 0;
    for (y = 0; y < c->height; ++y)
    {
        row = c->pixels + y * c->pitch;
        if (c->pixel_bytes == 1)
            for (x = 0; x < c->width; ++x)
            {
                row[x] = ((unsigned char *)c->table)[row[x]];
                ++c->pixels_changed;
            }
        else
        {
            row16 = (unsigned short *)row;
            for (x = 0; x < c->width; ++x)
            {
                row16[x] = ((unsigned short *)c->table)[row16[x]];
                ++c->pixels_changed;
            }
        }
    }
    return 1;
}
