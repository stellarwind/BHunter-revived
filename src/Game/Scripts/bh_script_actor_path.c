#include "bh_script_actor_path.h"
#include "../../Math/bh_round.h"
#include "../../Core/bh_callback_helpers.h"
#include <math.h>
#include <float.h>
static float store(double v)
{
    float r;
    bh_round_float(&r, v);
    return r;
}
void bh_script_actor_linear(float t, BHVec3 *o, const BHVec3 *a, const BHVec3 *b, const BHVec3 *unused1,
                            const BHVec3 *unused2)
{
    (void)unused1;
    (void)unused2;
    bh_vec3_interpolate(t, o, a, b);
}
void bh_script_actor_curve(float t, BHVec3 *out, const BHVec3 *start, const BHVec3 *first, const BHVec3 *second, const BHVec3 *end)
{
    float angle = store((double)t * 1.5707963267948966), u, v, ly, lz, one;
    double cs, sn, mu, mv, lx, rx, ry, rz;
    /* The raw C0|C3 test sends unordered t through the first branch. */
    if (_isnan(t) || t <= 0)
    {
        bh_script_actor_linear(t, out, start, first, 0, 0);
        return;
    }
    if (t >= 1)
    {
        double q = 1.0 - (double)t, dy = (double)second->y - end->y, dz = (double)second->z - end->z;
        out->x = store(end->x + ((double)second->x - end->x) * q);
        out->y = store(end->y + dy * q);
        out->z = store(end->z + dz * q);
        return;
    }
    cs = cos(angle);
    sn = sin(angle);
    u = cs > 0 ? store((1.0 - sn) / cs) : 0;
    mu = store(1.0 - (double)u);
    lx = store((double)first->x * mu + (double)start->x * u);
    ly = store((double)start->y * u + (double)first->y * mu);
    lz = store((double)start->z * u + (double)first->z * mu);
    v = sn > 0 ? store((1.0 - cs) / sn) : 0;
    mv = store(1.0 - (double)v);
    rx = (double)second->x * mv + (double)end->x * v;
    ry = (double)end->y * v + (double)second->y * mv;
    rz = (double)end->z * v + (double)second->z * mv;
    one = store(1.0 - (double)t);
    out->x = store((double)one * lx + rx * t);
    out->y = store((double)one * ly + (double)ry * t);
    out->z = store((double)one * lz + (double)rz * t);
}
