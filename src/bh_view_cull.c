#include "bh_view_cull.h"
#include "bh_round.h"
typedef char cull_mask_size[sizeof(unsigned long) == 4 ? 1 : -1];
static double plane_distance(const BHViewPlane *p, float x, float y, float z)
{
    return (((double)x * p->normal.x + (double)y * p->normal.y) + (double)z * p->normal.z) - p->offset;
}
unsigned long __cdecl bh_view_cull_sphere(const BHViewConfig *c, const BHMainCamera *camera, float x, float y, float z,
                                          float radius, unsigned long mask, float *depth)
{
    double distance;
    unsigned long bit, slot;
    if (mask & 3)
    {
        distance = plane_distance(camera->planes, x, y, z);
        if (depth)
            bh_round_float(depth, distance);
        if (distance < (double)c->near_z - radius)
            return 0xffffffffUL;
        if ((double)radius + c->near_z < distance)
            mask &= 0x3eUL;
        if ((double)radius + c->far_z < distance)
            return 0xffffffffUL;
        if (distance < (double)c->far_z - radius)
            mask &= 0x3dUL;
    }
    else if (depth)
        bh_round_float(depth, plane_distance(camera->planes, x, y, z));
    for (bit = 4, slot = 2; bit <= 32; bit *= 2, ++slot)
        if (mask & bit)
        {
            distance = plane_distance(camera->planes + slot, x, y, z);
            if (radius < distance)
                return 0xffffffffUL;
            if (distance < -(double)radius)
                mask -= bit;
        }
    return mask;
}
