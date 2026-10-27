#include "bh_axis_snap.h"
#include <math.h>
#include <float.h>
double __cdecl bh_vec3_snap_axis(BHVec3 *v)
{
    double length = sqrt(((double)v->y * v->y + (double)v->z * v->z) + (double)v->x * v->x);
    float x = v->x, y = v->y, z = v->z;
    unsigned long axis;
    if (x < 0 || _isnan((double)x))
        x = -x;
    if (y < 0 || _isnan((double)y))
        y = -y;
    if (z < 0 || _isnan((double)z))
        z = -z;
    axis = x > y && x > z ? 0 : (y > z ? 1 : 2);
    if (((float *)v)[axis] > 0)
        ((float *)v)[axis] = (float)length;
    else
        ((float *)v)[axis] = (float)-length;
    if (axis != 0)
        v->x = 0;
    if (axis != 1)
        v->y = 0;
    if (axis != 2)
        v->z = 0;
    return length;
}
