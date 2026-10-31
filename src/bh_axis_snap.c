#include "bh_axis_snap.h"
#include <math.h>
#define SNAP_V(i) (*(float *)((unsigned char *)v + 4 * (i)))
double __cdecl bh_vec3_snap_axis(BHVec3 *v)
{
    double length = sqrt((SNAP_V(1) * SNAP_V(1) + SNAP_V(2) * SNAP_V(2)) + SNAP_V(0) * SNAP_V(0));
    float x = SNAP_V(0), y, z;
    if (x < 0)
        x = -x;
    y = SNAP_V(1);
    if (y < 0)
        y = -y;
    z = SNAP_V(2);
    if (z < 0)
        z = -z;
    if (x > y && x > z)
    {
        if (SNAP_V(0) > 0)
            SNAP_V(0) = length;
        else
            SNAP_V(0) = -length;
        SNAP_V(1) = 0;
        SNAP_V(2) = 0;
    }
    else if (y > z)
    {
        if (SNAP_V(1) > 0)
            SNAP_V(1) = length;
        else
            SNAP_V(1) = -length;
        SNAP_V(0) = 0;
        SNAP_V(2) = 0;
    }
    else
    {
        if (SNAP_V(2) > 0)
            SNAP_V(2) = length;
        else
            SNAP_V(2) = -length;
        SNAP_V(0) = 0;
        SNAP_V(1) = 0;
    }
    return length;
}

#undef SNAP_V
