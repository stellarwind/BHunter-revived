#include "bh_camera_curve.h"
/* Force the original binary32 reload through its representation: VC5 can
   otherwise retain a volatile float's extended value. Keep retained X/Y and
   inverse values separate below; rounding every component uniformly differs. */
static __inline float curve_float_spill(double value)
{
    union {
        float value;
        volatile unsigned long bits;
    } stored, reloaded;
    stored.value = (float)value;
    reloaded.bits = stored.bits;
    return reloaded.value;
}
void __cdecl bh_camera_curve(float t, BHVec3 *out, const BHVec3 *a, const BHVec3 *b, const BHVec3 *c, const BHVec3 *d)
{
    float square, u, blend, inverse, last_z;
    double cube, blend_inverse, last_x, last_y;
    BHVec3 p;
    if (t <= 0)
    {
        out->x = ((double)b->x - a->x) * t + a->x;
        out->y = (double)a->y + ((double)b->y - a->y) * t;
        out->z = ((double)b->z - a->z) * t + a->z;
    }
    else if (t >= 1)
    {
        double v = 1.0 - (double)t;
        out->x = (double)d->x + ((double)c->x - d->x) * v;
        out->y = (double)d->y + ((double)c->y - d->y) * v;
        out->z = (double)d->z + ((double)c->z - d->z) * v;
    }
    else
    {
        square = curve_float_spill((double)t * t);
        u = curve_float_spill(1.0 - (double)t);
        p.x = curve_float_spill((double)a->x * u + (double)b->x * t);
        p.y = curve_float_spill((double)b->y * t + (double)a->y * u);
        p.z = curve_float_spill((double)b->z * t + (double)a->z * u);
        last_x = (double)d->x * t + (double)c->x * u;
        last_y = (double)c->y * u + (double)d->y * t;
        last_z = curve_float_spill((double)c->z * u + (double)d->z * t);
        cube = (double)t * square;
        blend = curve_float_spill((double)square * 3.0 - (cube + cube));
        blend_inverse = 1.0 - (double)blend;
        inverse = curve_float_spill(blend_inverse);
        out->x = blend_inverse * p.x + last_x * blend;
        out->y = (double)inverse * p.y + last_y * blend;
        out->z = (double)inverse * p.z + (double)last_z * blend;
    }
}
