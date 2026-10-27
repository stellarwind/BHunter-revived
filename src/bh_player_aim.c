#include "bh_player_aim.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <math.h>
#include <float.h>
static float stored(double value)
{
    float out;
    bh_round_float(&out, value);
    return out;
}
static float soft_limit(float value)
{
    if (value > 0)
        return stored((double)value - ((double)value - 1) / (1 - pow((double)value, -7.0)));
    if (value < 0)
        return stored(((-((double)value) - 1) / (pow((double)value, -7.0) - (-1.0))) + value);
    return value;
}
int __cdecl bh_player_aim_update(BHPhysicsStepBody *body)
{
    BHEntity *e;
    BHPhysicsState *p;
    float raw_x, raw_y, x, y, rounded_x;
    double angle_x, angle_y, cx, cy, sx, sy, scale;
    BHVec3 direction, local = {0, 0, 1};
    unsigned long i;
    if (!body || !(e = body->entity) || !(p = body->physics) || !bh_math_environment_supported())
        return 0;
    memcpy(&raw_x, (unsigned char *)e + 0x1f0, 4);
    memcpy(&raw_y, (unsigned char *)e + 0x1f4, 4);
    if (!_finite(raw_x) || !_finite(raw_y))
        return 0;
    if (raw_x == 0 && raw_y == 0)
    {
        for (i = 0; i < 3; ++i)
            ((float *)&direction)[i] = p->pose.m[i][2];
    }
    else
    {
        x = soft_limit(stored((double)raw_x * 3.0));
        y = soft_limit(stored((double)raw_y * 3.0));
        angle_x = (double)x * .2617993877991494;
        angle_y = (double)y * .2617993877991494;
        rounded_x = stored(angle_x);
        cx = cos(angle_x);
        cy = cos(angle_y);
        scale = cy * cx;
        for (i = 0; i < 3; ++i)
            bh_round_float(((float *)&direction) + i, (double)p->pose.m[i][2] * scale);
        if (rounded_x != 0)
        {
            sx = sin((double)rounded_x);
            for (i = 0; i < 3; ++i)
                bh_round_float(((float *)&direction) + i,
                               (double)((float *)&direction)[i] + (double)p->pose.m[i][0] * sx);
            local.x = stored(sx);
            local.z = stored((double)local.z * cx);
        }
        if (angle_y != 0)
        {
            sy = -(sin(angle_y) * cx);
            for (i = 0; i < 3; ++i)
                bh_round_float(((float *)&direction) + i,
                               (double)((float *)&direction)[i] + (double)p->pose.m[i][1] * sy);
            local.y = stored(sy);
            local.z = stored((double)local.z * cy);
        }
    }
    memcpy((unsigned char *)e + 0x280, &direction, 12);
    memcpy((unsigned char *)e + 0x28c, &local, 12);
    return 1;
}
