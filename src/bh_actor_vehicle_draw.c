#include "bh_actor_vehicle_draw.h"
#include "bh_draw_submit.h"
#include "bh_camera_curve.h"
#include "bh_rotation.h"
#include "bh_fp.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* Projection of the actual entity words read/written by 00434920. Controls
   are interleaved in the entity; do not snapshot or reorder them in a state. */
typedef struct BHActorVehiclePoseImage
{
    unsigned char other000[0x104];
    unsigned long sound, sound_frames, sound_resource, other110, resource;
    unsigned char other118[8];
    BHVec3 start, end, center, first, second, offset;
    unsigned char other168[0x34];
    float fade;
    unsigned char other1a0[0x44];
    unsigned short tick, duration_ticks;
} BHActorVehiclePoseImage;
typedef char pose_sound_offset[offsetof(BHActorVehiclePoseImage, sound) == 0x104 ? 1 : -1];
typedef char pose_resource_offset[offsetof(BHActorVehiclePoseImage, resource) == 0x114 ? 1 : -1];
typedef char pose_start_offset[offsetof(BHActorVehiclePoseImage, start) == 0x120 ? 1 : -1];
typedef char pose_end_offset[offsetof(BHActorVehiclePoseImage, end) == 0x12c ? 1 : -1];
typedef char pose_first_offset[offsetof(BHActorVehiclePoseImage, first) == 0x144 ? 1 : -1];
typedef char pose_second_offset[offsetof(BHActorVehiclePoseImage, second) == 0x150 ? 1 : -1];
typedef char pose_offset_offset[offsetof(BHActorVehiclePoseImage, offset) == 0x15c ? 1 : -1];
typedef char pose_fade_offset[offsetof(BHActorVehiclePoseImage, fade) == 0x19c ? 1 : -1];
typedef char pose_tick_offset[offsetof(BHActorVehiclePoseImage, tick) == 0x1e4 ? 1 : -1];
typedef char pose_duration_offset[offsetof(BHActorVehiclePoseImage, duration_ticks) == 0x1e6 ? 1 : -1];
/* Most assignments use the pinned compiler's retained FST value. Only the
   tangent operands consumed from float memory need a forced representation
   reload; rounding all intermediates changes the original operation graph. */
static __inline float natural(double x)
{
    return (float)x;
}
static __inline float stored(double x)
{
    union { float value; volatile unsigned long bits; } written, read;
    written.value = (float)x;
    read.bits = written.bits;
    return read.value;
}
void __cdecl bh_actor_vehicle_pose_core(BHActorVehicleDrawContext *c, BHEntity *e, unsigned short tick)
{
    BHActorVehiclePoseImage *a = (BHActorVehiclePoseImage *)e;
    BHVec3 before, p, after, angles, velocity, local, glow;
    BHMat34 matrix;
    short se;
    float phase, dx, dz, px, size, alpha, angle;
    double bank, clock, next_x, next_z, wide_dx, wide_dz, wide_px, wide_pz;
    float fade;
    unsigned long damage_frame;
    se = (short)(tick - a->tick);
    phase = natural((double)se / (short)a->duration_ticks);
    bh_camera_curve(natural((double)phase - 1.0 / 30.0), &before, &a->start, &a->first, &a->second, &a->end);
    bh_camera_curve(phase, &p, &a->start, &a->first, &a->second, &a->end);
    bh_camera_curve(natural((double)phase - (-1.0 / 30.0)), &after, &a->start, &a->first, &a->second, &a->end);
    before.x = natural((double)a->offset.x + before.x);
    before.y = natural((double)a->offset.y + before.y);
    before.z = natural((double)a->offset.z + before.z);
    p.x = natural((double)a->offset.x + p.x);
    p.y = natural((double)a->offset.y + p.y);
    p.z = natural((double)a->offset.z + p.z);
    /* 00434a28..00434b0c: FST publishes the rounded after position and
       differences without discarding their retained x87 values. The second
       tangent's Z difference never reloads a float for its square/product. */
    next_x = (double)a->offset.x + after.x;
    next_z = (double)a->offset.z + after.z;
    after.x = natural(next_x);
    after.z = natural(next_z);
    wide_dx = next_x - p.x;
    wide_dz = next_z - p.z;
    dx = stored(wide_dx);
    dz = stored(wide_dz);
    wide_px = (double)p.x - before.x;
    wide_pz = (double)p.z - before.z;
    px = stored(wide_px);
    after.y = natural((double)a->offset.y + after.y);
    bank = ((wide_pz * dx - (double)px * dz) / sqrt(wide_dx * dx + wide_dz * dz)) /
           sqrt(wide_px * px + wide_pz * wide_pz);
    angles.x = natural(atan2(dx, dz));
    angles.y = 0;
    angles.z = natural(bank * 3.141592653589793);

#ifdef DEBUGGING
    {
    BHActorRouteState s;
    bh_actor_route_read(e, &s);
    if (c->clock && c->clock->nonfinite_curve && (!_finite(angles.x) || !_finite(angles.y) || !_finite(angles.z)))
        c->clock->nonfinite_curve(c->clock->nonfinite_user, 0x434920, e, &s, bh_actor_route_now(c->clock), &before, &p,
                                  &after, &angles);
    }
#endif
    bh_draw_submit_resource(c->queue, 0, a->resource, &p, &angles, 1);
    fade = a->fade;
    if (fade < 1)
    {
        size = natural(4.0 - (double)rand() * (-1.0 / 16384.0));
        alpha = natural(1.0 - fade);
        damage_frame = (unsigned long)rand() & 7;
        angle = natural((double)rand() * (1.0 / 4096.0));
        bh_draw_billboard(c->queue, 3, c->damage_resource, &p, size, size, angle, damage_frame, alpha);
    }
    if (a->resource == c->glowing_model)
    {
        matrix.m[0][0] = 1; matrix.m[0][1] = 0; matrix.m[0][2] = 0; matrix.m[0][3] = 0;
        matrix.m[1][0] = 0; matrix.m[1][1] = 1; matrix.m[1][2] = 0; matrix.m[1][3] = 0;
        matrix.m[2][0] = 0; matrix.m[2][1] = 0; matrix.m[2][2] = 1; matrix.m[2][3] = 0;
        bh_mat34_translate_world(p.x, p.y, p.z, &matrix);
        bh_mat34_rotate_y(angles.x, &matrix);
        bh_mat34_rotate_x(angles.y, &matrix);
        bh_mat34_rotate_z(angles.z, &matrix);
        clock = (double)c->animation_clock * .03125;
#define POSE_GLOW(gx, rate, resource) \
        local.x = (gx); local.y = -.95f; local.z = -.65f; \
        bh_mat34_transform_point(&matrix, &local, &glow); \
        alpha = natural((sin(clock * (rate)) - (-1.0)) * .5); \
        bh_draw_billboard(c->queue, 3, (resource), &glow, 2, 2, 0, 0, alpha)
        POSE_GLOW(.6f, 25.5, c->glow_first);
        POSE_GLOW(.2f, 18.5, c->glow_first);
        POSE_GLOW(-.2f, 28.5, c->glow_second);
        POSE_GLOW(-.6f, 23.5, c->glow_second);
#undef POSE_GLOW
    }
    a->sound_frames = 2;
    velocity.x = natural((double)a->fade * ((double)after.x - p.x) * 30);
    velocity.y = natural((double)a->fade * ((double)after.y - p.y) * 30);
    velocity.z = natural(((double)after.z - p.z) * a->fade * 30);
    if (a->sound == BH_SOUND_INVALID)
        a->sound = bh_sound_request(c->sound, a->sound_resource, 1, 1, &p, &velocity);
    else
        bh_sound_update(c->sound, a->sound, 1, 1, &p, &velocity);
}

int __cdecl bh_actor_vehicle_draw(BHActorVehicleDrawContext *c, BHEntity *e)
{
    if (!c || !c->queue || !c->sound || !e)
    {
        if (c)
            c->error = BH_ACTOR_ROUTE_ARGUMENT;
        return 0;
    }
    if (!bh_math_environment_supported() || !((BHActorVehiclePoseImage *)e)->duration_ticks)
    {
        c->error = BH_ACTOR_ROUTE_NUMERICAL;
        return 0;
    }
    bh_actor_vehicle_pose_core(c, e, c->clock ? bh_actor_route_now(c->clock) : c->tick);
    c->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
