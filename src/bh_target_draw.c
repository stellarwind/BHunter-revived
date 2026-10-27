#include "bh_target_draw.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
typedef char target_layout
    [sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && offsetof(BHEntity, opaque0) == 0x18 ? 1 : -1];
int __cdecl bh_target_draw_state_read(const BHEntity *e, BHTargetDrawState *s)
{
    if (!e || !s)
        return 0;
    memcpy(&s->local_anchor, (const unsigned char *)e + 0xf0, 12);
    memcpy(&s->number, (const unsigned char *)e + 0x118, 4);
    memcpy(&s->flags, (const unsigned char *)e + 0x120, 4);
    memcpy(&s->resource, (const unsigned char *)e + 0x124, 4);
    memcpy(&s->owner, (const unsigned char *)e + 0x128, 4);
    return 1;
}
int __cdecl bh_target_draw_state_write(BHEntity *e, const BHTargetDrawState *s)
{
    if (!e || !s)
        return 0;
    memcpy((unsigned char *)e + 0xf0, &s->local_anchor, 12);
    memcpy((unsigned char *)e + 0x118, &s->number, 4);
    memcpy((unsigned char *)e + 0x120, &s->flags, 4);
    memcpy((unsigned char *)e + 0x124, &s->resource, 4);
    memcpy((unsigned char *)e + 0x128, &s->owner, 4);
    return 1;
}
static float rounded(double value)
{
    float f;
    bh_round_float(&f, value);
    return f;
}
static void result(int *total, int r)
{
    if (r < 0)
        *total = -1;
    else if (!r && *total > 0)
        *total = 0;
}
static int rect(BHTargetDrawContext *c, unsigned long image, float x, float y, float w, float h, float u, float ue,
                float v, float ve, float depth)
{
    return bh_pickup_rect_queue(c->queue, 8, image, x, y, w, h, 0, 0, 1, u, ue, v, ve, depth);
}
static int greater(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a > b;
}
static int greater_equal(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a >= b;
}
static void marker(BHTargetDrawContext *c, float scale, float x, float y, float depth, int fallback, int *status)
{
    if (fallback)
    {
        x = rounded((double)scale * 320);
        y = rounded((double)scale * 27);
    }
    result(status, rect(c, c->marker_image, x, y, rounded((double)scale * (fallback ? 100 : 80)),
                        rounded((double)scale * (fallback ? 50 : 40)), 0, .5f, .5f, .218f, depth));
}
int __cdecl bh_target_draw(BHTargetDrawContext *c, const BHEntity *e)
{
    BHTargetDrawState s;
    BHVec3 model, angles;
    float basis[9], x, y, z, scale, depth, partial_x, partial_y, urow = .50390625f, vrow = .81640625f;
    double partial_z;
    long value, divisor, ds[8];
    int i, first = 7, count, status = 1, visible;
    unsigned long bits;
    if (!c || !c->queue || !c->camera || !c->view || !e || c->screen_width <= 0 || !bh_math_environment_supported())
        return -1;
    bh_target_draw_state_read(e, &s);
    memcpy(basis, e->opaque0, 36);
    memcpy(&angles, (const unsigned char *)e + 0x3c, 12);
    /* Original X/Y offsets spill before position addition; Z does not. */
    partial_x = rounded(((double)basis[0] * s.local_anchor.x + (double)basis[3] * s.local_anchor.y) +
                        (double)basis[6] * s.local_anchor.z);
    partial_y = rounded(((double)basis[1] * s.local_anchor.x + (double)basis[4] * s.local_anchor.y) +
                        (double)basis[7] * s.local_anchor.z);
    partial_z = ((double)basis[5] * s.local_anchor.y + (double)basis[2] * s.local_anchor.x) +
                (double)basis[8] * s.local_anchor.z;
    model.x = rounded((double)e->position.x + partial_x);
    model.y = rounded((double)e->position.y + partial_y);
    model.z = rounded((double)e->position.z + partial_z);
    result(&status, bh_draw_submit_resource(c->queue, 0, s.resource, &model, &angles, 1));
    if (s.owner != c->player || !s.number)
        return status;
    scale = rounded((double)c->screen_width * .0015625);
    value = s.number;
    if (value < 0)
    {
        urow = .59765625f;
        vrow = .91015625f;
        bits = 0UL - (unsigned long)value;
        memcpy(&value, &bits, 4);
    }
    x = e->position.x;
    y = e->position.y;
    z = e->position.z;
    visible = bh_project_point_scale(c->camera, c->view, &x, &y, &z);
    depth = rounded((double)c->view->near_z - (-.01));
    if (!visible)
    {
        if (s.flags & 4)
            marker(c, scale, 0, 0, depth, 1, &status);
        return status;
    }
    result(&status, rect(c, c->atlas_image, x, rounded((double)y - (double)scale * 24), rounded((double)scale * 64),
                         rounded((double)scale * 14), .00390625f, .4921875f, urow, .1015625f, depth));
    if (s.flags & 4)
    {
        int inside = !greater((double)scale * -50, x) && greater_equal((double)scale * 690, x) &&
                     !greater((double)scale * -25, y) && greater_equal((double)scale * 505, y);
        marker(c, scale, x, y, depth, !inside, &status);
    }
    divisor = 10000000;
    for (i = 0; i < 8; ++i)
    {
        ds[i] = value / divisor;
        value -= divisor * ds[i];
        if (i < first && ds[i])
            first = i;
        divisor /= 10;
    }
    count = 8 - first;
    y = rounded((double)y - (double)scale * (-24));
    x = rounded((double)x - ((double)count * scale) * 4);
    for (i = first; i < 8; ++i)
    {
        float u = rounded((.5 - (double)ds[i] * (-8.0)) * .0078125);
        result(&status, rect(c, c->atlas_image, x, y, rounded((double)scale * 8), rounded((double)scale * 12), u,
                             .0546875f, vrow, .0859375f, depth));
        x = rounded((double)x - rounded((double)scale * (-8)));
    }
    return status;
}
