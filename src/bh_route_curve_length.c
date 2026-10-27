#include "bh_route_curve_length.h"
#include "bh_camera_curve.h"
#include "bh_round.h"
#include <math.h>
float __cdecl bh_route_curve_length(const BHVec3 controls[4])
{
    BHVec3 previous, current;
    float sum = 0, rx, ry;
    double x, y, z, length;
    unsigned long quarter;
    bh_camera_curve(0, &previous, controls, controls + 1, controls + 2, controls + 3);
    for (quarter = 1; quarter < 4; ++quarter)
    {
        bh_camera_curve((float)quarter * 0.25f, &current, controls, controls + 1, controls + 2, controls + 3);
        x = (double)current.x - previous.x;
        y = (double)current.y - previous.y;
        z = (double)current.z - previous.z;
        bh_round_float(&rx, x);
        bh_round_float(&ry, y);
        length = sqrt((x * rx + y * ry) + z * z);
        bh_round_float(&sum, length + sum);
        previous = current;
    }
    return sum;
}
