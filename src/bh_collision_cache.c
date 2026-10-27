#include "bh_collision_cache.h"
#include "bh_round.h"
#include <string.h>
#include <math.h>
void __cdecl bh_collision_cache_dispose(BHCollisionCache *c)
{
    if (!c)
        return;
    if (c->allocator.release)
    {
        if (c->face_flags)
            c->allocator.release(c->face_flags, c->allocator.user);
        if (c->centers)
            c->allocator.release(c->centers, c->allocator.user);
        if (c->radii)
            c->allocator.release(c->radii, c->allocator.user);
        if (c->squared_radii)
            c->allocator.release(c->squared_radii, c->allocator.user);
        if (c->faces)
            c->allocator.release(c->faces, c->allocator.user);
    }
    memset(c, 0, sizeof(*c));
}
int __cdecl bh_collision_cache_prepare(BHCollisionCache *c, const BHBsp *b, const BHSceneAllocator *a)
{
    BHCollisionCache n;
    unsigned long i, j;
    float sum, rx, rz;
    double x, y, z;
    float *center;
    if (!c || !b)
        return 0;
    if (c->centers)
    {
        if (c->bsp == b)
            return 1;
        c->error = BH_BSP_ARGUMENT;
        return 0;
    }
    if (!b->faces || !b->nodes || !b->counts[3] || !b->counts[4] || b->counts[3] > 65535 || b->counts[4] > 65535)
    {
        c->error = BH_BSP_ARGUMENT;
        return 0;
    }
    memset(&n, 0, sizeof(n));
    n.bsp = b;
    n.allocator = a ? *a : b->allocator;
    n.face_count = b->counts[3];
    n.node_count = b->counts[4];
    if (!n.allocator.allocate || !n.allocator.release)
    {
        c->error = BH_BSP_ARGUMENT;
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
        n.faces[i] = b->faces[i];
        n.faces[i].runtime = 0;
        n.face_flags[i] = 0;
    }
    for (i = 0; i < n.node_count; ++i)
    {
        center = &n.centers[i].x;
        for (j = 0; j < 3; ++j)
        {
            bh_round_float(&sum, (double)b->nodes[i].minimum[j] + b->nodes[i].maximum[j]);
            bh_round_float(center + j, (double)sum * 0.5);
        }
        x = (double)b->nodes[i].maximum[0] - center[0];
        y = (double)b->nodes[i].maximum[1] - center[1];
        z = (double)b->nodes[i].maximum[2] - center[2];
        bh_round_float(&rx, x);
        bh_round_float(&rz, z);
        bh_round_float(n.squared_radii + i, (x * rx + y * y) + z * rz);
        bh_round_float(n.radii + i, sqrt((double)n.squared_radii[i]));
    }
    *c = n;
    return 1;
memory:
    bh_collision_cache_dispose(&n);
    c->error = BH_BSP_MEMORY;
    return 0;
}
