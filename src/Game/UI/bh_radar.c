#include "bh_radar.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <float.h>
#include <string.h>
static int vector(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int fail(BHRadarContext *radar, BHRadarError error)
{
    if (radar)
        radar->error = error;
    return 0;
}
int bh_radar_bind(BHRadarContext *radar, BHDrawQueue *queue, const BHRadarFrame *frame, unsigned long marker_image,
                  unsigned long bar_image)
{
    if (!radar || !queue || !frame)
        return 0;
    memset(radar, 0, sizeof(*radar));
    radar->queue = queue;
    radar->frame = frame;
    radar->marker_image = marker_image;
    radar->bar_image = bar_image;
    return 1;
}
static void submitted(BHRadarContext *radar, int result)
{
    ++radar->attempted;
    if (result > 0)
        ++radar->queued;
    else if (!result)
        ++radar->dropped;
    else
        radar->error = BH_RADAR_QUEUE;
}
static void point(BHVec3 *out, const BHRadarFrame *frame, double x, double y)
{
    bh_round_float(&out->x, ((double)frame->axis_x.x * x + frame->origin.x) + (double)frame->axis_y.x * y);
    bh_round_float(&out->y, ((double)frame->axis_x.y * x + frame->origin.y) + (double)frame->axis_y.y * y);
    bh_round_float(&out->z, ((double)frame->axis_x.z * x + frame->origin.z) + (double)frame->axis_y.z * y);
}
int bh_radar_marker(BHRadarContext *radar, const BHVec3 *player, const BHVec3 *target, unsigned long kind,
                    unsigned long extended, unsigned long skip_marker)
{
    const BHRadarFrame *frame;
    BHVec3 delta, center;
    double raw_y, height, remaining, raw_x, raw_z;
    float x, z, squared, radius, fade, size, u, v, width, screen_x, screen_y, depth;
    if (!radar || !(frame = radar->frame) || !radar->queue || !vector(player) || !vector(target) ||
        !vector(&frame->origin) || !vector(&frame->axis_x) || !vector(&frame->axis_y) || !vector(&frame->normal) ||
        !vector(&frame->yaw_x) || !vector(&frame->yaw_y))
        return fail(radar, BH_RADAR_ARGUMENT);
    radar->error = BH_RADAR_OK;
    radar->attempted = radar->queued = radar->dropped = radar->culled = 0;
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
        ++radar->culled;
        return 1;
    }
    remaining = 1 - height;
    bh_round_float(&fade, remaining);
    bh_round_float(&size, remaining + remaining);
    raw_x = (((double)delta.z * frame->yaw_x.z + (double)delta.y * frame->yaw_x.y) + (double)delta.x * frame->yaw_x.x) *
            .0025;
    raw_z = (((double)delta.z * frame->yaw_y.z + (double)delta.y * frame->yaw_y.y) + (double)delta.x * frame->yaw_y.x) *
            .0025;
    bh_round_float(&x, raw_x);
    bh_round_float(&z, raw_z);
    /* Original FST retains each coordinate for one factor and reloads the
       stored binary32 coordinate for the other factor before squared store. */
    bh_round_float(&squared, raw_x * x + raw_z * z);
    if (!_finite(squared) || squared < 0)
        return fail(radar, BH_RADAR_NUMERICAL);
    if (!extended && squared > .9)
    {
        ++radar->culled;
        return 1;
    }
    bh_round_float(&radius, sqrt((double)squared));
    if (extended && squared > .9)
    {
        if (frame->visibility_word & 8)
        {
            ++radar->culled;
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
    if (!frame->flat)
    {
        point(&center, frame, (double)x * .1, (double)z * .1);
        bh_round_float(&width, (double)size * .01);
        if (!vector(&center) || !_finite(width) || !_finite(fade))
            return fail(radar, BH_RADAR_NUMERICAL);
        if (!skip_marker)
            submitted(radar, bh_quad_queue(radar->queue, 3, radar->marker_image, &center, &frame->axis_y,
                                           &frame->normal, width, width, 0, 0, fade, u, .5f, v, .5f));
        if (kind == 1)
        {
            point(&center, frame, (double)x * .065, (double)z * .085);
            if (!vector(&center))
                return fail(radar, BH_RADAR_NUMERICAL);
            submitted(radar, bh_quad_queue(radar->queue, 3, radar->bar_image, &center, &frame->axis_y, &frame->normal,
                                           .075f, .015f, 0, 0, radius, 0, .5f, .5f, .125f));
        }
    }
    else
    {
        if (!_finite(frame->near_z) || frame->near_z <= 0)
            return fail(radar, BH_RADAR_ARGUMENT);
        bh_round_float(&width, ((double)frame->unit * size) * .7);
        bh_round_float(&depth, (double)frame->near_z - (-.001));
        bh_round_float(&screen_x, (double)x * frame->screen_radius + frame->screen_x);
        bh_round_float(&screen_y, (double)frame->screen_y - (double)z * frame->screen_radius);
        if (!_finite(width) || !_finite(screen_x) || !_finite(screen_y) || !_finite(fade))
            return fail(radar, BH_RADAR_NUMERICAL);
        if (!skip_marker)
            submitted(radar, bh_pickup_rect_queue(radar->queue, 8, radar->marker_image, screen_x, screen_y, width,
                                                  width, 0, BH_HUD_ANCHOR_RIGHT, fade, u, .5f, v, .5f, depth));
        if (kind == 1)
        {
            bh_round_float(&screen_x, (double)frame->screen_x - ((double)x * frame->screen_radius) * (-.65));
            bh_round_float(&screen_y, (double)frame->screen_y - ((double)z * frame->screen_radius) * .85);
            submitted(radar, bh_pickup_rect_queue(radar->queue, 8, radar->bar_image, screen_x, screen_y,
                                                  (float)(long)((unsigned long)frame->unit * 5UL), (float)frame->unit,
                                                  0, BH_HUD_ANCHOR_RIGHT, radius, 0, .5f, .5f, .125f, depth));
        }
    }
    return radar->error == BH_RADAR_OK;
}
