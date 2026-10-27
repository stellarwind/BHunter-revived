#include "bh_script_actor_path.h"
#include "bh_round.h"
#include "bh_callback_helpers.h"
#include <math.h>
#include <float.h>
static float store(double v)
{
    float r;
    bh_round_float(&r, v);
    return r;
}
void __cdecl bh_script_actor_linear(float t, BHVec3 *o, const BHVec3 *a, const BHVec3 *b, const BHVec3 *unused1,
                                    const BHVec3 *unused2)
{
    (void)unused1;
    (void)unused2;
    bh_vec3_interpolate(t, o, a, b);
}
void __cdecl bh_script_actor_curve(float t, BHVec3 *o, const BHVec3 *a, const BHVec3 *b, const BHVec3 *c,
                                   const BHVec3 *d)
{
    float angle = store((double)t * 1.5707963267948966), u, v, ly, lz, one;
    double cs, sn, mu, mv, lx, rx, ry, rz;
    /* The raw C0|C3 test sends unordered t through the first branch. */
    if (_isnan(t) || t <= 0)
    {
        bh_script_actor_linear(t, o, a, b, 0, 0);
        return;
    }
    if (t >= 1)
    {
        double q = 1.0 - (double)t, dy = (double)c->y - d->y, dz = (double)c->z - d->z;
        o->x = store(d->x + ((double)c->x - d->x) * q);
        o->y = store(d->y + dy * q);
        o->z = store(d->z + dz * q);
        return;
    }
    cs = cos(angle);
    sn = sin(angle);
    u = cs > 0 ? store((1.0 - sn) / cs) : 0;
    mu = store(1.0 - (double)u);
    lx = store((double)b->x * mu + (double)a->x * u);
    ly = store((double)a->y * u + (double)b->y * mu);
    lz = store((double)a->z * u + (double)b->z * mu);
    v = sn > 0 ? store((1.0 - cs) / sn) : 0;
    mv = store(1.0 - (double)v);
    rx = (double)c->x * mv + (double)d->x * v;
    ry = (double)d->y * v + (double)c->y * mv;
    rz = (double)d->z * v + (double)c->z * mv;
    one = store(1.0 - (double)t);
    o->x = store((double)one * lx + rx * t);
    o->y = store((double)one * ly + (double)ry * t);
    o->z = store((double)one * lz + (double)rz * t);
}
