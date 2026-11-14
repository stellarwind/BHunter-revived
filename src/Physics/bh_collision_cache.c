#include "bh_collision_cache.h"
#include "../Math/bh_round.h"
#include <string.h>
#include <math.h>
void bh_collision_cache_dispose(BHCollisionCache *collision_cache)
{
    if (!collision_cache)
        return;
    if (collision_cache->allocator.release)
    {
        if (collision_cache->face_flags)
            collision_cache->allocator.release(collision_cache->face_flags, collision_cache->allocator.user);
        if (collision_cache->centers)
            collision_cache->allocator.release(collision_cache->centers, collision_cache->allocator.user);
        if (collision_cache->radii)
            collision_cache->allocator.release(collision_cache->radii, collision_cache->allocator.user);
        if (collision_cache->squared_radii)
            collision_cache->allocator.release(collision_cache->squared_radii, collision_cache->allocator.user);
        if (collision_cache->faces)
            collision_cache->allocator.release(collision_cache->faces, collision_cache->allocator.user);
    }
    memset(collision_cache, 0, sizeof(*collision_cache));
}
int bh_collision_cache_prepare(BHCollisionCache *collision_cache, const BHBsp *bsp, const BHSceneAllocator *a)
{
    BHCollisionCache n;
    unsigned long i, j;
    float sum, rx, rz;
    double x, y, z;
    float *center;
    if (!collision_cache || !bsp)
        return 0;
    if (collision_cache->centers)
    {
        if (collision_cache->bsp == bsp)
            return 1;
        collision_cache->error = BH_BSP_ARGUMENT;
        return 0;
    }
    if (!bsp->faces || !bsp->nodes || !bsp->counts[3] || !bsp->counts[4] || bsp->counts[3] > 65535 || bsp->counts[4] > 65535)
    {
        collision_cache->error = BH_BSP_ARGUMENT;
        return 0;
    }
    memset(&n, 0, sizeof(n));
    n.bsp = bsp;
    n.allocator = a ? *a : bsp->allocator;
    n.face_count = bsp->counts[3];
    n.node_count = bsp->counts[4];
    if (!n.allocator.allocate || !n.allocator.release)
    {
        collision_cache->error = BH_BSP_ARGUMENT;
        return 0;
    }
    n.face_flags = n.allocator.allocate(n.face_count * 4, n.allocator.user);
    if (!n.face_flags)
        goto memory;
    n.centers = n.allocator.allocate(n.node_count * 12, n.allocator.user);
    if (!n.centers)
        goto memory;
    n.radii = n.allocator.allocate(n.node_count * 4, n.allocator.user);
    if (!n.radii)
        goto memory;
    n.squared_radii = n.allocator.allocate(n.node_count * 4, n.allocator.user);
    if (!n.squared_radii)
        goto memory;
    n.faces = n.allocator.allocate(n.face_count * 12, n.allocator.user);
    if (!n.faces)
        goto memory;
    for (i = 0; i < n.face_count; ++i)
    {
        n.faces[i] = bsp->faces[i];
        n.faces[i].runtime = 0;
        n.face_flags[i] = 0;
    }
    for (i = 0; i < n.node_count; ++i)
    {
        center = &n.centers[i].x;
        for (j = 0; j < 3; ++j)
        {
            bh_round_float(&sum, (double)bsp->nodes[i].minimum[j] + bsp->nodes[i].maximum[j]);
            bh_round_float(center + j, (double)sum * 0.5);
        }
        x = (double)bsp->nodes[i].maximum[0] - center[0];
        y = (double)bsp->nodes[i].maximum[1] - center[1];
        z = (double)bsp->nodes[i].maximum[2] - center[2];
        bh_round_float(&rx, x);
        bh_round_float(&rz, z);
        bh_round_float(n.squared_radii + i, (x * rx + y * y) + z * rz);
        bh_round_float(n.radii + i, sqrt((double)n.squared_radii[i]));
    }
    *collision_cache = n;
    return 1;
memory:
    bh_collision_cache_dispose(&n);
    collision_cache->error = BH_BSP_MEMORY;
    return 0;
}
