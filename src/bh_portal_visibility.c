#include "bh_portal_visibility.h"
#include "bh_matrix_inverse.h"
#include <float.h>
#include <math.h>

double bh_portal_camera_radius(const BHMat34 *projected, float near_z, const float bounds[4], float motion)
{
    BHMat34 inverse;
    BHVec3 point, direction;
    double squared, radius = 0;
    unsigned long i;
    if (!_finite(near_z) || near_z <= 0 || !_finite(motion) || motion < 0 ||
        !bh_mat34_inverse_general(projected, &inverse))
        return 0;
    for (i = 0; i < 4; ++i)
        if (!_finite(bounds[i]))
            return 0;
    for (i = 0; i < 4; ++i)
    {
        point.x = bounds[(i & 1) ? 2 : 0] * near_z;
        point.y = bounds[(i & 2) ? 3 : 1] * near_z;
        point.z = near_z;
        bh_mat34_transform_direction(&inverse, &point, &direction);
        squared =
            (double)direction.x * direction.x + (double)direction.y * direction.y + (double)direction.z * direction.z;
        if (!_finite(squared))
            return 0;
        if (radius < squared)
            radius = squared;
    }
    radius = sqrt(radius) + motion;
    return radius * radius;
}

int bh_portal_camera_touches(const BHVec3 corners[4], const BHVec3 *eye, double radius_squared)
{
    unsigned long axis, i;
    double distance = 0, delta;
    float low, high, value;
    if (!_finite(radius_squared) || radius_squared <= 0)
        return 0;
    for (axis = 0; axis < 3; ++axis)
    {
        low = high = ((const float *)corners)[axis];
        value = ((const float *)eye)[axis];
        if (!_finite(low) || !_finite(value))
            return 0;
        for (i = 1; i < 4; ++i)
        {
            float coordinate = ((const float *)(corners + i))[axis];
            if (!_finite(coordinate))
                return 0;
            if (coordinate < low)
                low = coordinate;
            if (high < coordinate)
                high = coordinate;
        }
        delta = value < low ? (double)low - value : high < value ? (double)value - high : 0;
        distance += delta * delta;
    }
    return distance <= radius_squared;
}
