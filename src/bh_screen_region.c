#include "bh_screen_region.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
#include <string.h>
int __cdecl bh_screen_region_draw(BHScreenQuadContext *c, BHTextureImage *image, float x, float y, float width,
                                  float height, float angle, unsigned char flags, float modulation, float u,
                                  float u_extent, float uv_v, float v_extent, float output_depth)
{
    BHClipVertex v[4], *ring, *p;
    double w, h, r, cs, sn, hs, hc, conversion;
    float hw, hh, radius, cw, sw, negative_hs, stored_hc, bx, by, depth;
    float u0, u1, v0, v1;
    unsigned long i, n, color;
    long shade;
    if (!c || !c->view || !c->clip || !c->emit || !image || !_finite(x) || !_finite(y) || !_finite(width) ||
        !_finite(height) || !_finite(angle) || !_finite(modulation) || !_finite(u) || !_finite(u_extent) ||
        !_finite(uv_v) || !_finite(v_extent) || !_finite(output_depth) || !_finite(c->view->near_z))
        return 0;
    c->emitted = c->culled = 0;
    w = (double)width * .5;
    h = (double)height * .5;
    bh_round_float(&hw, w);
    bh_round_float(&hh, h);
    r = sqrt(w * hw + h * hh);
    bh_round_float(&radius, r);
    if (!(c->view->left < r + x && x - radius < c->view->right && c->view->top < radius + y &&
          y - radius < c->view->bottom))
    {
        c->culled = 1;
        return 1;
    }
    memset(v, 0, sizeof(v));
    u0 = u;
    bh_round_float(&u1, (double)u + u_extent);
    v0 = uv_v;
    bh_round_float(&v1, (double)uv_v + v_extent);
    if (flags & 1)
    {
        bh_round_float(&u0, 1.0 - u0);
        bh_round_float(&u1, 1.0 - u1);
    }
    if (flags & 2)
    {
        bh_round_float(&v0, 1.0 - v0);
        bh_round_float(&v1, 1.0 - v1);
    }
    v[0].u = u0;
    v[0].v = v0;
    v[2].u = u1;
    v[2].v = v1;
    v[1].u = (flags & 4) ? u1 : u0;
    v[1].v = (flags & 4) ? v0 : v1;
    v[3].u = (flags & 4) ? u0 : u1;
    v[3].v = (flags & 4) ? v1 : v0;
    if (angle == 0)
    {
        bh_round_float(&v[0].sx, (double)x - hw);
        bh_round_float(&v[0].sy, (double)y - hh);
        v[1].sx = v[0].sx;
        bh_round_float(&v[1].sy, (double)y + hh);
        bh_round_float(&v[2].sx, (double)x + hw);
        v[2].sy = v[1].sy;
        v[3].sx = v[2].sx;
        v[3].sy = v[0].sy;
    }
    else
    {
        cs = cos((double)angle);
        sn = sin((double)angle);
        bh_round_float(&cw, (double)hw * cs);
        bh_round_float(&sw, (double)hw * sn);
        hs = -(double)hh * sn;
        hc = (double)hh * cs;
        bh_round_float(&negative_hs, hs);
        bh_round_float(&stored_hc, hc);
        bh_round_float(&bx, (double)x - cw);
        bh_round_float(&by, (double)y - sw);
        bh_round_float(&v[0].sx, ((double)x - cw) - negative_hs);
        bh_round_float(&v[0].sy, ((double)y - sw) - stored_hc);
        bh_round_float(&v[1].sx, (double)negative_hs + bx);
        bh_round_float(&v[1].sy, (double)stored_hc + by);
        bh_round_float(&v[2].sx, (hs + cw) + x);
        bh_round_float(&v[2].sy, (hc + sw) + y);
        bh_round_float(&v[3].sx, ((double)cw + x) - negative_hs);
        bh_round_float(&v[3].sy, ((double)sw + y) - stored_hc);
    }
    for (i = 0; i < 4; ++i)
    {
        v[i].next = v + (i + 1) % 4;
        v[i].previous = v + (i + 3) % 4;
    }
    ring = bh_screen_clip_polygon(c->clip, v, c->view->near_z);
    if (!ring)
    {
        if (c->clip->error != BH_CLIP_OK)
            return 0;
        c->culled = 1;
        return 1;
    }
    conversion = (double)modulation * -255.0;
    if (!_finite(conversion) || conversion < -2147483648.0 || conversion > 2147483647.0)
        return 0;
    shade = (long)conversion;
    color = c->base_color - (unsigned long)shade * 0x10101UL;
    depth = output_depth;
    p = ring;
    n = 0;
    do
    {
        if (++n > 12)
            return 0;
        p->point.z = depth;
        p = p->next;
    } while (p != ring);
    if (!c->emit(ring, image, color, c->emit_user))
        return 0;
    c->emitted = 1;
    return 1;
}
