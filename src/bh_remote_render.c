#include "bh_remote_render.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>
static float stored(double value)
{
    float out;
    bh_round_float(&out, value);
    return out;
}
int __cdecl bh_remote_predict_pose(const BHEntity *e, unsigned short tick, BHVec3 *position, BHVec3 *angles)
{
    BHVec3 anchor[2], velocity[2], acceleration[2];
    unsigned short stamp, elapsed;
    long dt;
    unsigned long i, j;
    BHVec3 *out[2];
    double t, half_squared;
    if (!e || !position || !angles || !bh_math_environment_supported())
        return 0;
    memcpy(&stamp, (const char *)e + 0x17c, 2);
    elapsed = (unsigned short)(tick - stamp);
    dt = elapsed & 0x8000 ? (long)elapsed - 65536 : (long)elapsed;
    memcpy(anchor, (const char *)e + 0x180, sizeof(anchor));
    if (dt <= 0)
    {
        *position = anchor[0];
        *angles = anchor[1];
        return 1;
    }
    memcpy(velocity, (const char *)e + 0x48, sizeof(velocity));
    memcpy(acceleration, (const char *)e + 0x60, sizeof(acceleration));
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++)
            if (!_finite((&anchor[j].x)[i]) || !_finite((&velocity[j].x)[i]) || !_finite((&acceleration[j].x)[i]))
                return 0;
    t = (double)dt;
    half_squared = (t * t) * .5;
    out[0] = position;
    out[1] = angles;
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++)
            bh_round_float((&out[j]->x) + i, (double)(&velocity[j].x)[i] * t + (&anchor[j].x)[i]);
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++)
            bh_round_float((&out[j]->x) + i, (double)(&acceleration[j].x)[i] * half_squared + (&out[j]->x)[i]);
    return 1;
}
static void queued(BHRemoteRender *c, int result)
{
    ++c->attempted;
    if (result == 0)
        ++c->dropped;
    else if (result < 0)
        c->error = 1;
}
int __cdecl bh_remote_render(BHRemoteRender *c, BHEntity *e)
{
    BHVec3 position, angles, velocity, normal, point, up = {0, -1, 0};
    unsigned long handle, resource, i, j, frames = 60;
    short model;
    unsigned char flags;
    float angle;
    int value;
    if (!c || !e || !c->queue || !c->sound)
        return 0;
    c->attempted = c->dropped = 0;
    c->error = 0;
    memcpy(&model, (char *)e + 0x114, 2);
    if (model < 0 || model >= 8)
        return 0;
    if (!bh_remote_predict_pose(e, c->tick, &position, &angles))
        return 0;
    queued(c, bh_draw_submit_resource(c->queue, 0, c->models[model], &position, &angles, 1));
    for (i = 0; i < 3; i++)
    {
        float component;
        memcpy(&component, (char *)e + 0x48 + i * 4, 4);
        (&velocity.x)[i] = stored((double)component * 30.0);
    }
    memcpy((char *)e + 0x108, &frames, 4);
    memcpy(&handle, (char *)e + 0x104, 4);
    memcpy(&resource, (char *)e + 0x10c, 4);
    if (handle == BH_SOUND_INVALID)
    {
        handle = bh_sound_request(c->sound, resource, 1, 1, &position, &velocity);
        memcpy((char *)e + 0x104, &handle, 4);
    }
    else
        bh_sound_update(c->sound, handle, 1, 1, &position, &velocity);
    memcpy(&flags, (char *)e + 0x11a, 1);
    if (flags & 1)
        for (j = 0; j < 24; j++)
        {
            for (i = 0; i < 3; i++)
            {
                value = rand();
                (&normal.x)[i] = (float)(value - 16384);
            }
            bh_vec3_normalize(&normal);
            for (i = 0; i < 3; i++)
            {
                (&normal.x)[i] = stored((double)(&normal.x)[i] * 3.0);
                (&point.x)[i] = stored((double)(&normal.x)[i] + (&position.x)[i]);
            }
            value = rand();
            angle = stored((double)value * .0001953125);
            queued(c, bh_draw_oriented(c->queue, 3, c->spark_texture, &point, &up, &normal, 3, 3, angle, 8, .3f));
        }
    return !c->error;
}
