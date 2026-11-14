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
void bh_camera_curve(float t, BHVec3 *out, const BHVec3 *first, const BHVec3 *first_tangent, const BHVec3 *last_tangent,
                     const BHVec3 *last)
{
    float square, u, blend, inverse, last_z;
    double cube, blend_inverse, last_x, last_y;
    BHVec3 first_point;
    if (t <= 0)
    {
        out->x = ((double)first_tangent->x - first->x) * t + first->x;
        out->y = (double)first->y + ((double)first_tangent->y - first->y) * t;
        out->z = ((double)first_tangent->z - first->z) * t + first->z;
    }
    else if (t >= 1)
    {
        double v = 1.0 - (double)t;
        out->x = (double)last->x + ((double)last_tangent->x - last->x) * v;
        out->y = (double)last->y + ((double)last_tangent->y - last->y) * v;
        out->z = (double)last->z + ((double)last_tangent->z - last->z) * v;
    }
    else
    {
        square = curve_float_spill((double)t * t);
        u = curve_float_spill(1.0 - (double)t);
        first_point.x = curve_float_spill((double)first->x * u + (double)first_tangent->x * t);
        first_point.y = curve_float_spill((double)first_tangent->y * t + (double)first->y * u);
        first_point.z = curve_float_spill((double)first_tangent->z * t + (double)first->z * u);
        last_x = (double)last->x * t + (double)last_tangent->x * u;
        last_y = (double)last_tangent->y * u + (double)last->y * t;
        last_z = curve_float_spill((double)last_tangent->z * u + (double)last->z * t);
        cube = (double)t * square;
        blend = curve_float_spill((double)square * 3.0 - (cube + cube));
        blend_inverse = 1.0 - (double)blend;
        inverse = curve_float_spill(blend_inverse);
        out->x = blend_inverse * first_point.x + last_x * blend;
        out->y = (double)inverse * first_point.y + last_y * blend;
        out->z = (double)inverse * first_point.z + (double)last_z * blend;
    }
}
