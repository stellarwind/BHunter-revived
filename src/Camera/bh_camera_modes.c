#include "bh_camera_modes.h"
#include "../Math/bh_round.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Explicit binary32 stores reproduce the stack candidates consumed by the
   smoothing owner, independently of VC5 excess-precision reuse. */
static float stored(double x)
{
    float value;
    bh_round_float(&value, x);
    return value;
}
static float component(const BHVec3 *vector, int i)
{
    return i == 0 ? vector->x : i == 1 ? vector->y : vector->z;
}
static void put(BHVec3 *vector, int i, double x)
{
    float f = stored(x);
    if (i == 0)
        vector->x = f;
    else if (i == 1)
        vector->y = f;
    else
        vector->z = f;
}
static BHVec3 history(const BHCameraModeState *state)
{
    BHVec3 position;
    memcpy(&position, state->mode_data, 12);
    return position;
}
static void remember(BHCameraModeState *state, const BHVec3 *position)
{
    memcpy(state->mode_data, position, 12);
}

void bh_camera_look_at(BHVec3 *angles, const BHVec3 *target, const BHVec3 *eye, const BHVec3 *up)
{
    BHVec3 forward, right, local_up, result;
    double yaw, pitch, c, s, cp, sp;
    float numerator, denominator;
    forward.x = stored((double)target->x - eye->x);
    forward.y = stored((double)target->y - eye->y);
    forward.z = stored((double)target->z - eye->z);
    bh_vec3_normalize(&forward);
    bh_vec3_cross(up, &forward, &right);
    bh_vec3_normalize(&right);
    bh_vec3_cross(&forward, &right, &local_up);
    yaw = atan2((double)forward.x, forward.z);
    c = cos(yaw);
    s = sin(yaw);
    pitch = atan2(-(double)forward.y, (double)forward.x * s + (double)forward.z * c);
    cp = cos(pitch);
    sp = sin(pitch);
    numerator = stored(cp * right.y - ((double)right.z * c + (double)right.x * s) * sp);
    denominator = stored((double)right.x * c - (double)right.z * s);
    result.x = stored(yaw);
    result.y = stored(pitch);
    result.z = stored(atan2((double)numerator, denominator));
    *angles = result;
}

void bh_camera_mode_0(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 position;
    int i;
    for (i = 0; i < 3; ++i)
        put(&position, i, (double)component(&in->target, i) - (double)component(&in->basis_y, i) * .3);
    bh_camera_mode_smooth(context, state, &position, &in->angles, .05, .5);
}
static void chase(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in,
                  double position_rate, double angle_rate)
{
    BHVec3 position;
    int i;
    for (i = 0; i < 3; ++i)
        put(&position, i,
            (double)component(&in->basis_y, i) * -2.5 +
                stored((double)component(&in->basis_z, i) * -7.5 + component(&in->target, i)));
    bh_camera_mode_smooth(context, state, &position, &in->angles, position_rate, angle_rate);
}
void bh_camera_mode_1(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    chase(context, state, in, .8, .9);
}
void bh_camera_mode_10(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    chase(context, state, in, .85, .98);
}
void bh_camera_mode_2(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 position, angles;
    double z;
    int i;
    for (i = 0; i < 3; ++i)
    {
        z = (double)component(&in->basis_z, i) * 15;
        if (i)
            z = stored(z);
        put(&position, i, z + component(&in->target, i) + stored((double)component(&in->basis_y, i) * 2));
    }
    angles.x = stored((double)in->angles.x - (-3.14159265358979323846));
    angles.y = -in->angles.y;
    angles.z = -in->angles.z;
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .5);
}
void bh_camera_mode_3(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 position = in->target, angles;
    position.y = stored((double)position.y - 15);
    angles.x = in->angles.x;
    angles.y = -1.5707963705062866f;
    angles.z = 0;
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .5);
}
void bh_camera_mode_4(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 position, angles;
    double co = cos((double)in->angles.x), si = sin((double)in->angles.x);
    position.x = stored((double)in->target.x - co * 15);
    position.y = in->target.y;
    position.z = stored((double)in->target.z - si * -15);
    angles.x = stored((double)in->angles.x - (-1.5707963267948966));
    angles.y = angles.z = 0;
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .5);
}
void bh_camera_mode_5(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 position, angles;
    double co = cos((double)in->angles.x), si = sin((double)in->angles.x);
    position.x = stored((double)in->target.x - co * -15);
    position.y = in->target.y;
    position.z = stored((double)in->target.z - si * 15);
    angles.x = stored((double)in->angles.x - 1.5707963267948966);
    angles.y = angles.z = 0;
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .5);
}
void bh_camera_mode_6(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 position = in->target, angles;
    position.y = stored((double)position.y - (-15));
    angles.x = in->angles.x;
    angles.y = 1.5707963705062866f;
    angles.z = 0;
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .5);
}
void bh_camera_mode_7(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 previous, d, position, angles;
    double scale, raw[3], motion;
    int i;
    if (state->first_update)
        remember(state, &in->target);
    previous = history(state);
    for (i = 0; i < 3; ++i)
    {
        motion = ((double)component(&in->target, i) - component(&previous, i)) * -30;
        if (i != 1)
            motion = stored(motion);
        raw[i] = (double)component(&in->basis_z, i) * -3 + motion;
        put(&d, i, raw[i]);
    }
    scale = 8 / sqrt((raw[0] * d.x + raw[1] * d.y) + raw[2] * d.z);
    for (i = 0; i < 3; ++i)
    {
        motion = (double)component(&d, i) * scale;
        if (i)
            motion = stored(motion);
        put(&position, i, motion + component(&in->target, i));
    }
    bh_camera_look_at(&angles, &in->target, &position, &in->basis_y);
    bh_camera_mode_smooth(context, state, &position, &angles, .965, .95);
    remember(state, &in->target);
}
static double movement_radius(const BHVec3 *position, const BHVec3 *previous)
{
    double x = (double)position->x - previous->x,
           y = (double)position->y - previous->y,
           z = (double)position->z - previous->z;
    return 10 - sqrt((x * stored(x) + y * y) + z * stored(z)) * -30;
}
void bh_camera_mode_8(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 old, position, angles, up = {0, 1, 0};
    double phase, range, si, co;
    float radius;
    if (state->first_update)
        remember(state, &in->target);
    old = history(state);
    phase = (double)state->seed_yaw + context->elapsed;
    range = movement_radius(&in->target, &old);
    radius = stored(range);
    si = sin(phase);
    co = cos(phase);
    position.x = stored((double)radius * si + in->target.x);
    position.y = stored((double)in->target.y +
                        stored((sin(context->elapsed * 1.3) * cos(context->elapsed * 1.7)) * range * .1));
    position.z = stored((double)in->target.z + stored((double)radius * co));
    bh_camera_look_at(&angles, &in->target, &position, &up);
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .5);
    remember(state, &in->target);
}
void bh_camera_mode_11(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 old, position, angles, up = {0, 1, 0};
    double phase, warped, range;
    float radius, t;
    if (state->first_update)
        remember(state, &in->target);
    old = history(state);
    warped = (context->elapsed - sin(context->elapsed * 3.7) * -0.2631578947368421) * .65;
    t = stored(warped);
    range = movement_radius(&in->target, &old);
    radius = stored(range);
    phase = (double)state->seed_yaw + t;
    position.x = stored(sin(phase) * radius + in->target.x);
    position.y = stored((double)in->target.y + stored((sin(warped * 1.3) * cos((double)t * 1.7)) * range * .1));
    position.z = stored((double)in->target.z + stored((double)radius * cos((double)stored(phase))));
    bh_camera_look_at(&angles, &in->target, &position, &up);
    bh_camera_mode_smooth(context, state, &position, &angles, .5, .95);
    remember(state, &in->target);
}
void bh_camera_mode_9(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    BHVec3 old, d, position, angles, up = {0, 1, 0};
    double scale, distance, raw[3], v, base_x;
    int i;
    if (state->first_update)
    {
        remember(state, &in->target);
        for (i = 0; i < 3; ++i)
        {
            raw[i] = (double)component(&in->basis_z, i) * .02 * 75;
            put(&d, i, raw[i]);
        }
        scale = 50 / sqrt((raw[0] * d.x + raw[1] * d.y) + raw[2] * d.z);
        for (i = 0; i < 3; ++i)
        {
            v = (double)component(&d, i) * scale;
            if (i == 1)
                v = stored(v);
            put(&state->position, i, v + component(&in->target, i));
        }
    }
    for (i = 0; i < 3; ++i)
        raw[i] = (double)component(&in->target, i) - component(&state->position, i);
    distance = sqrt((raw[0] * raw[0] + raw[1] * stored(raw[1])) + raw[2] * stored(raw[2]));
    if (distance <= 75)
        position = state->position;
    else
    {
        old = history(state);
        for (i = 0; i < 3; ++i)
        {
            v = (double)component(&in->target, i) - component(&old, i);
            if (i)
                v = stored(v);
            raw[i] = v * 75;
            put(&d, i, raw[i]);
        }
        scale = 50 / sqrt((raw[0] * d.x + raw[1] * d.y) + raw[2] * raw[2]);
        base_x = (double)d.x * scale + in->target.x;
        position.x = stored(base_x);
        position.y = stored((double)in->target.y + stored((double)d.y * scale));
        position.z = stored((double)in->target.z + stored(raw[2] * scale));
        for (i = 0; i < 3; ++i)
            put(&position, i, (i ? component(&position, i) : base_x) - ((double)rand() - 16384) * -0.00048828125);
    }
    bh_camera_look_at(&angles, &in->target, &position, &up);
    bh_camera_mode_smooth(context, state, &position, &angles, 0, 0);
    remember(state, &in->target);
}
int bh_camera_mode_update(BHCameraStateContext *context, BHCameraModeState *state, const BHCameraInput *in)
{
    static void (*const modes[12])(BHCameraStateContext *, BHCameraModeState *, const BHCameraInput *) = {
        bh_camera_mode_0, bh_camera_mode_1, bh_camera_mode_2, bh_camera_mode_3, bh_camera_mode_4,  bh_camera_mode_5,
        bh_camera_mode_6, bh_camera_mode_7, bh_camera_mode_8, bh_camera_mode_9, bh_camera_mode_10, bh_camera_mode_11};
    if (!context || !state || !in || state->mode < 0 || state->mode >= 12)
        return 0;
    modes[state->mode](context, state, in);
    return 1;
}
