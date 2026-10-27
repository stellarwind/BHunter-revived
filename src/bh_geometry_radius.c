#include "bh_geometry_radius.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
double __cdecl bh_geometry_radius(const BHBsp *bsp)
{
    const BHBspNode *node;
    double x, y, z, minimum, maximum;
    float rounded_maximum;
    int i;
    if (!bsp || !bsp->nodes || !bsp->counts[4] || !bh_math_environment_supported())
        return 0;
    node = bsp->nodes;
    for (i = 0; i < 3; ++i)
        if (!_finite(node->minimum[i]) || !_finite(node->maximum[i]))
            return 0;
    x = node->minimum[0];
    y = node->minimum[1];
    z = node->minimum[2];
    minimum = sqrt((x * x + y * y) + z * z);
    x = node->maximum[0];
    y = node->maximum[1];
    z = node->maximum[2];
    maximum = sqrt((x * x + y * y) + z * z);
    bh_round_float(&rounded_maximum, maximum);
    return minimum > (double)rounded_maximum ? minimum : (double)rounded_maximum;
}
