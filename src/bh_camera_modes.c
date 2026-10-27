#include "bh_camera_modes.h"
#include "bh_round.h"
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
static float component(const BHVec3 *v, int i)
{
    return i == 0 ? v->x : i == 1 ? v->y : v->z;
}
static void put(BHVec3 *v, int i, double x)
{
    float f = stored(x);
    if (i == 0)
        v->x = f;
    else if (i == 1)
        v->y = f;
    else
        v->z = f;
}
static BHVec3 history(const BHCameraModeState *s)
{
    BHVec3 v;
    memcpy(&v, s->mode_data, 12);
    return v;
}
static void remember(BHCameraModeState *s, const BHVec3 *p)
{
    memcpy(s->mode_data, p, 12);
}

void __cdecl bh_camera_look_at(BHVec3 *angles, const BHVec3 *target, const BHVec3 *eye, const BHVec3 *up)
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

void __cdecl bh_camera_mode_0(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 p;
    int i;
    for (i = 0; i < 3; ++i)
        put(&p, i, (double)component(&in->target, i) - (double)component(&in->basis_y, i) * .3);
    bh_camera_mode_smooth(c, s, &p, &in->angles, .05, .5);
}
static void chase(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in, double pr, double ar)
{
    BHVec3 p;
    int i;
    for (i = 0; i < 3; ++i)
        put(&p, i,
            (double)component(&in->basis_y, i) * -2.5 +
                stored((double)component(&in->basis_z, i) * -7.5 + component(&in->target, i)));
    bh_camera_mode_smooth(c, s, &p, &in->angles, pr, ar);
}
void __cdecl bh_camera_mode_1(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    chase(c, s, in, .8, .9);
}
void __cdecl bh_camera_mode_10(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    chase(c, s, in, .85, .98);
}
void __cdecl bh_camera_mode_2(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 p, a;
    double z;
    int i;
    for (i = 0; i < 3; ++i)
    {
        z = (double)component(&in->basis_z, i) * 15;
        if (i)
            z = stored(z);
        put(&p, i, z + component(&in->target, i) + stored((double)component(&in->basis_y, i) * 2));
    }
    a.x = stored((double)in->angles.x - (-3.14159265358979323846));
    a.y = -in->angles.y;
    a.z = -in->angles.z;
    bh_camera_mode_smooth(c, s, &p, &a, .5, .5);
}
void __cdecl bh_camera_mode_3(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 p = in->target, a;
    p.y = stored((double)p.y - 15);
    a.x = in->angles.x;
    a.y = -1.5707963705062866f;
    a.z = 0;
    bh_camera_mode_smooth(c, s, &p, &a, .5, .5);
}
void __cdecl bh_camera_mode_4(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 p, a;
    double co = cos((double)in->angles.x), si = sin((double)in->angles.x);
    p.x = stored((double)in->target.x - co * 15);
    p.y = in->target.y;
    p.z = stored((double)in->target.z - si * -15);
    a.x = stored((double)in->angles.x - (-1.5707963267948966));
    a.y = a.z = 0;
    bh_camera_mode_smooth(c, s, &p, &a, .5, .5);
}
void __cdecl bh_camera_mode_5(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 p, a;
    double co = cos((double)in->angles.x), si = sin((double)in->angles.x);
    p.x = stored((double)in->target.x - co * -15);
    p.y = in->target.y;
    p.z = stored((double)in->target.z - si * 15);
    a.x = stored((double)in->angles.x - 1.5707963267948966);
    a.y = a.z = 0;
    bh_camera_mode_smooth(c, s, &p, &a, .5, .5);
}
void __cdecl bh_camera_mode_6(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 p = in->target, a;
    p.y = stored((double)p.y - (-15));
    a.x = in->angles.x;
    a.y = 1.5707963705062866f;
    a.z = 0;
    bh_camera_mode_smooth(c, s, &p, &a, .5, .5);
}
void __cdecl bh_camera_mode_7(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 previous, d, p, a;
    double scale, raw[3], motion;
    int i;
    if (s->first_update)
        remember(s, &in->target);
    previous = history(s);
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
        put(&p, i, motion + component(&in->target, i));
    }
    bh_camera_look_at(&a, &in->target, &p, &in->basis_y);
    bh_camera_mode_smooth(c, s, &p, &a, .965, .95);
    remember(s, &in->target);
}
static double movement_radius(const BHVec3 *p, const BHVec3 *previous)
{
    double x = (double)p->x - previous->x, y = (double)p->y - previous->y, z = (double)p->z - previous->z;
    return 10 - sqrt((x * stored(x) + y * y) + z * stored(z)) * -30;
}
void __cdecl bh_camera_mode_8(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 old, p, a, up = {0, 1, 0};
    double phase, range, si, co;
    float radius;
    if (s->first_update)
        remember(s, &in->target);
    old = history(s);
    phase = (double)s->seed_yaw + c->elapsed;
    range = movement_radius(&in->target, &old);
    radius = stored(range);
    si = sin(phase);
    co = cos(phase);
    p.x = stored((double)radius * si + in->target.x);
    p.y = stored((double)in->target.y + stored((sin(c->elapsed * 1.3) * cos(c->elapsed * 1.7)) * range * .1));
    p.z = stored((double)in->target.z + stored((double)radius * co));
    bh_camera_look_at(&a, &in->target, &p, &up);
    bh_camera_mode_smooth(c, s, &p, &a, .5, .5);
    remember(s, &in->target);
}
void __cdecl bh_camera_mode_11(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 old, p, a, up = {0, 1, 0};
    double phase, warped, range;
    float radius, t;
    if (s->first_update)
        remember(s, &in->target);
    old = history(s);
    warped = (c->elapsed - sin(c->elapsed * 3.7) * -0.2631578947368421) * .65;
    t = stored(warped);
    range = movement_radius(&in->target, &old);
    radius = stored(range);
    phase = (double)s->seed_yaw + t;
    p.x = stored(sin(phase) * radius + in->target.x);
    p.y = stored((double)in->target.y + stored((sin(warped * 1.3) * cos((double)t * 1.7)) * range * .1));
    p.z = stored((double)in->target.z + stored((double)radius * cos((double)stored(phase))));
    bh_camera_look_at(&a, &in->target, &p, &up);
    bh_camera_mode_smooth(c, s, &p, &a, .5, .95);
    remember(s, &in->target);
}
void __cdecl bh_camera_mode_9(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    BHVec3 old, d, p, a, up = {0, 1, 0};
    double scale, distance, raw[3], v, base_x;
    int i;
    if (s->first_update)
    {
        remember(s, &in->target);
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
            put(&s->position, i, v + component(&in->target, i));
        }
    }
    for (i = 0; i < 3; ++i)
        raw[i] = (double)component(&in->target, i) - component(&s->position, i);
    distance = sqrt((raw[0] * raw[0] + raw[1] * stored(raw[1])) + raw[2] * stored(raw[2]));
    if (distance <= 75)
        p = s->position;
    else
    {
        old = history(s);
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
        p.x = stored(base_x);
        p.y = stored((double)in->target.y + stored((double)d.y * scale));
        p.z = stored((double)in->target.z + stored(raw[2] * scale));
        for (i = 0; i < 3; ++i)
            put(&p, i, (i ? component(&p, i) : base_x) - ((double)rand() - 16384) * -0.00048828125);
    }
    bh_camera_look_at(&a, &in->target, &p, &up);
    bh_camera_mode_smooth(c, s, &p, &a, 0, 0);
    remember(s, &in->target);
}
int __cdecl bh_camera_mode_update(BHCameraStateContext *c, BHCameraModeState *s, const BHCameraInput *in)
{
    static void(__cdecl *const modes[12])(BHCameraStateContext *, BHCameraModeState *, const BHCameraInput *) = {
        bh_camera_mode_0, bh_camera_mode_1, bh_camera_mode_2, bh_camera_mode_3, bh_camera_mode_4,  bh_camera_mode_5,
        bh_camera_mode_6, bh_camera_mode_7, bh_camera_mode_8, bh_camera_mode_9, bh_camera_mode_10, bh_camera_mode_11};
    if (!c || !s || !in || s->mode < 0 || s->mode >= 12)
        return 0;
    modes[s->mode](c, s, in);
    return 1;
}
