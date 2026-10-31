#include "bh_radar.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
#include <string.h>
static int vector(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int fail(BHRadarContext *c, BHRadarError e)
{
    if (c)
        c->error = e;
    return 0;
}
int __cdecl bh_radar_bind(BHRadarContext *c, BHDrawQueue *q, const BHRadarFrame *frame, unsigned long marker,
                          unsigned long bar)
{
    if (!c || !q || !frame)
        return 0;
    memset(c, 0, sizeof(*c));
    c->queue = q;
    c->frame = frame;
    c->marker_image = marker;
    c->bar_image = bar;
    return 1;
}
static void submitted(BHRadarContext *c, int result)
{
    ++c->attempted;
    if (result > 0)
        ++c->queued;
    else if (!result)
        ++c->dropped;
    else
        c->error = BH_RADAR_QUEUE;
}
static void point(BHVec3 *out, const BHRadarFrame *f, double x, double y)
{
    bh_round_float(&out->x, ((double)f->axis_x.x * x + f->origin.x) + (double)f->axis_y.x * y);
    bh_round_float(&out->y, ((double)f->axis_x.y * x + f->origin.y) + (double)f->axis_y.y * y);
    bh_round_float(&out->z, ((double)f->axis_x.z * x + f->origin.z) + (double)f->axis_y.z * y);
}
int __cdecl bh_radar_marker(BHRadarContext *c, const BHVec3 *player, const BHVec3 *target, unsigned long kind,
                            unsigned long extended, unsigned long skip_marker)
{
    const BHRadarFrame *f;
    BHVec3 delta, center;
    double raw_y, height, remaining, raw_x, raw_z;
    float x, z, squared, radius, fade, size, u, v, width, screen_x, screen_y, depth;
    if (!c || !(f = c->frame) || !c->queue || !vector(player) || !vector(target) || !vector(&f->origin) ||
        !vector(&f->axis_x) || !vector(&f->axis_y) || !vector(&f->normal) || !vector(&f->yaw_x) || !vector(&f->yaw_y))
        return fail(c, BH_RADAR_ARGUMENT);
    c->error = BH_RADAR_OK;
    c->attempted = c->queued = c->dropped = c->culled = 0;
    raw_y = (double)target->y - player->y;
    bh_round_float(&delta.y, raw_y);
    bh_round_float(&delta.x, (double)target->x - player->x);
    bh_round_float(&delta.z, (double)target->z - player->z);
    height = raw_y * .01;
    if (height < 0)
        height = -height;
    if (extended)
    {
        if (height > 1)
            height = 1;
        height *= .7;
    }
    if (height > 1)
    {
        ++c->culled;
        return 1;
    }
    remaining = 1 - height;
    bh_round_float(&fade, remaining);
    bh_round_float(&size, remaining + remaining);
    raw_x = (((double)delta.z * f->yaw_x.z + (double)delta.y * f->yaw_x.y) + (double)delta.x * f->yaw_x.x) * .0025;
    raw_z = (((double)delta.z * f->yaw_y.z + (double)delta.y * f->yaw_y.y) + (double)delta.x * f->yaw_y.x) * .0025;
    bh_round_float(&x, raw_x);
    bh_round_float(&z, raw_z);
    /* Original FST retains each coordinate for one factor and reloads the
       stored binary32 coordinate for the other factor before squared store. */
    bh_round_float(&squared, raw_x * x + raw_z * z);
    if (!_finite(squared) || squared < 0)
        return fail(c, BH_RADAR_NUMERICAL);
    if (!extended && squared > .9)
    {
        ++c->culled;
        return 1;
    }
    bh_round_float(&radius, sqrt((double)squared));
    if (extended && squared > .9)
    {
        if (f->visibility_word & 8)
        {
            ++c->culled;
            return 1;
        }
        bh_round_float(&x, (double)x / radius);
        bh_round_float(&z, (double)z / radius);
        radius = .5f;
    }
    u = (kind & 1) ? .5f : 0;
    v = (kind & 2) ? .5f : 0;
    if (kind == 1 || kind == 2)
        bh_round_float(&size, (double)size * .5);
    bh_round_float(&fade, (1.0 - radius) * fade);
    if (kind == 1)
        fade = 1;
    if (!f->flat)
    {
        point(&center, f, (double)x * .1, (double)z * .1);
        bh_round_float(&width, (double)size * .01);
        if (!vector(&center) || !_finite(width) || !_finite(fade))
            return fail(c, BH_RADAR_NUMERICAL);
        if (!skip_marker)
            submitted(c, bh_quad_queue(c->queue, 3, c->marker_image, &center, &f->axis_y, &f->normal, width, width, 0,
                                       0, fade, u, .5f, v, .5f));
        if (kind == 1)
        {
            point(&center, f, (double)x * .065, (double)z * .085);
            if (!vector(&center))
                return fail(c, BH_RADAR_NUMERICAL);
            submitted(c, bh_quad_queue(c->queue, 3, c->bar_image, &center, &f->axis_y, &f->normal, .075f, .015f, 0, 0,
                                       radius, 0, .5f, .5f, .125f));
        }
    }
    else
    {
        if (!_finite(f->near_z) || f->near_z <= 0)
            return fail(c, BH_RADAR_ARGUMENT);
        bh_round_float(&width, ((double)f->unit * size) * .7);
        bh_round_float(&depth, (double)f->near_z - (-.001));
        bh_round_float(&screen_x, (double)x * f->screen_radius + f->screen_x);
        bh_round_float(&screen_y, (double)f->screen_y - (double)z * f->screen_radius);
        if (!_finite(width) || !_finite(screen_x) || !_finite(screen_y) || !_finite(fade))
            return fail(c, BH_RADAR_NUMERICAL);
        if (!skip_marker)
            submitted(c, bh_pickup_rect_queue(c->queue, 8, c->marker_image, screen_x, screen_y, width, width, 0,
                                              BH_HUD_ANCHOR_RIGHT, fade, u, .5f, v, .5f, depth));
        if (kind == 1)
        {
            bh_round_float(&screen_x, (double)f->screen_x - ((double)x * f->screen_radius) * (-.65));
            bh_round_float(&screen_y, (double)f->screen_y - ((double)z * f->screen_radius) * .85);
            submitted(c, bh_pickup_rect_queue(c->queue, 8, c->bar_image, screen_x, screen_y,
                                              (float)(long)((unsigned long)f->unit * 5UL), (float)f->unit, 0,
                                              BH_HUD_ANCHOR_RIGHT, radius, 0, .5f, .5f, .125f, depth));
        }
    }
    return c->error == BH_RADAR_OK;
}
