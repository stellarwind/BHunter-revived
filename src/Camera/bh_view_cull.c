#include "bh_view_cull.h"
#include "../Math/bh_round.h"
static_assert(sizeof(unsigned long) == 4, "cull mask size");
static double plane_distance(const BHViewPlane *plane, float x, float y, float z)
{
    return (((double)x * plane->normal.x + (double)y * plane->normal.y) + (double)z * plane->normal.z) - plane->offset;
}
unsigned long bh_view_cull_sphere(const BHViewConfig *view, const BHMainCamera *camera, float x, float y, float z,
                                  float radius, unsigned long mask, float *depth)
{
    double distance;
    unsigned long bit, plane_index;
    if (mask & 3)
    {
        distance = plane_distance(camera->planes, x, y, z);
        if (depth)
            bh_round_float(depth, distance);
        if (distance < (double)view->near_z - radius)
            return 0xffffffffUL;
        if ((double)radius + view->near_z < distance)
            mask &= 0x3eUL;
        if ((double)radius + view->far_z < distance)
            return 0xffffffffUL;
        if (distance < (double)view->far_z - radius)
            mask &= 0x3dUL;
    }
    else if (depth)
        bh_round_float(depth, plane_distance(camera->planes, x, y, z));
    for (bit = 4, plane_index = 2; bit <= 32; bit *= 2, ++plane_index)
        if (mask & bit)
        {
            distance = plane_distance(camera->planes + plane_index, x, y, z);
            if (radius < distance)
                return 0xffffffffUL;
            if (distance < -(double)radius)
                mask -= bit;
        }
    return mask;
}
