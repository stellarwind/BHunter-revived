#include "bh_navigation.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

static void *__cdecl allocate(unsigned long n, void *user)
{
    return malloc(n);
}
static void __cdecl release(void *p, void *user)
{
    free(p);
}
static int fail(BHNavigation *n, BHNavigationError error)
{
    if (n)
        n->error = error;
    return 0;
}
int __cdecl bh_navigation_init(BHNavigation *n, const BHSceneAllocator *allocator)
{
    BHSceneAllocator memory;
    if (!n || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    if (allocator)
        memory = *allocator;
    else
    {
        memory.allocate = allocate;
        memory.release = release;
        memory.user = NULL;
    }
    memset(n, 0, sizeof(*n));
    n->allocator = memory;
    bh_scene_routes_init(&n->routes, &memory);
    return 1;
}
void __cdecl bh_navigation_dispose(BHNavigation *n)
{
    BHSceneAllocator memory;
    if (!n)
        return;
    memory = n->allocator;
    if (n->nodes)
        memory.release(n->nodes, memory.user);
    bh_scene_routes_dispose(&n->routes);
    bh_navigation_init(n, &memory);
}
long __cdecl bh_navigation_find(const BHNavigation *n, const BHWorldGeometry *p)
{
    unsigned long i;
    if (!n || !p)
        return -1;
    for (i = 0; i < n->node_count; ++i)
        if (n->nodes[i].placement == p)
            return (long)i;
    return -1;
}
static unsigned long degree(const BHWorldGeometry *p)
{
    unsigned long i, count = 0;
    if (!p->owner)
        for (i = 0; i < 6; ++i)
            if (p->navigation.portals[i].neighbor)
                ++count;
    return count;
}
static int scene_counts(BHWorldGeometry *first, unsigned long count, unsigned long *total, unsigned long *junctions)
{
    BHWorldGeometry *p = first;
    unsigned long i, j, d;
    const float *bounds;
    *total = *junctions = 0;
    if (!p || !count || count > 65535)
        return 0;
    for (i = 0; i < count; ++i)
    {
        if (!p || (i && p == first))
            return 0;
        bounds = (const float *)&p->minimum;
        for (j = 0; j < 6; ++j)
            if (!_finite(bounds[j]))
                return 0;
        for (j = 0; j < 3; ++j)
            if (bounds[j] > bounds[j + 3])
                return 0;
        d = degree(p);
        if (d)
        {
            ++*total;
            if (d != 2)
                ++*junctions;
        }
        p = (BHWorldGeometry *)p->navigation.next;
    }
    return p == first;
}
/* Assembly 00407d9f..00407e98 keeps current centers unrounded, but spills
   previous centers and one operand of each squared delta to binary32. */
static int edge_distance(const BHWorldGeometry *previous, const BHWorldGeometry *current, float *out)
{
    const float *a = (const float *)&previous->minimum, *b = (const float *)&current->minimum;
    double d[3];
    float center, rounded[3];
    unsigned long i;
    for (i = 0; i < 3; ++i)
    {
        bh_round_float(&center, ((double)a[i] + a[i + 3]) * 0.5);
        d[i] = ((double)b[i] + b[i + 3]) * 0.5 - center;
        bh_round_float(rounded + i, d[i]);
    }
    bh_round_float(out, sqrt((d[0] * rounded[0] + d[1] * rounded[1]) + d[2] * rounded[2]));
    return _finite(*out) && *out >= 0;
}
static int corridor(BHNavigation *n, long origin, long previous, long current, float *distance, long *destination,
                    float *stack)
{
    unsigned long depth = 0, i, seen;
    long next;
    BHWorldGeometry *p;
    for (;;)
    {
        if (depth >= n->node_count)
            return fail(n, BH_NAV_CYCLE);
        if (!edge_distance(n->nodes[previous].placement, n->nodes[current].placement, stack + depth))
            return fail(n, BH_NAV_NUMERICAL);
        if ((unsigned long)current < n->junction_count)
        {
            *destination = current;
            *distance = stack[depth];
            break;
        }
        p = n->nodes[current].placement;
        seen = 0;
        next = -1;
        for (i = 0; i < 6; ++i)
            if (p->navigation.portals[i].neighbor)
            {
                long candidate = bh_navigation_find(n, (BHWorldGeometry *)p->navigation.portals[i].neighbor);
                if (candidate < 0)
                    return fail(n, BH_NAV_TOPOLOGY);
                if (candidate != previous)
                {
                    next = candidate;
                    if (seen)
                    {
                        n->nodes[current].junction_first_port = origin;
                        n->nodes[current].endpoints_valid |= 1;
                    }
                    else
                    {
                        n->nodes[current].junction_second_port = origin;
                        n->nodes[current].endpoints_valid |= 2;
                    }
                    break;
                }
                ++seen;
            }
        if (next < 0)
        {
            *destination = origin;
            *distance = 0;
            break;
        }
        previous = current;
        current = next;
        ++depth;
    }
    while (depth)
    {
        --depth;
        bh_round_float(distance, (double)*distance + stack[depth]);
    }
    return 1;
}
static void relax(BHNavigation *n)
{
    unsigned long k, j, i, count = n->junction_count;
    BHSceneRoute *cells = n->routes.cells;
    for (k = 0; k < count; ++k)
        for (j = 0; j < count; ++j)
            for (i = 0; i < count; ++i)
            {
                double candidate = (double)cells[i * count + k].distance + cells[k * count + j].distance;
                if (candidate < cells[i * count + j].distance)
                {
                    float rounded;
                    bh_round_float(&rounded, candidate);
                    cells[i * count + j].distance = cells[j * count + i].distance = rounded;
                    cells[i * count + j].exit_slot = cells[k * count + j].exit_slot;
                    cells[j * count + i].exit_slot = cells[k * count + i].exit_slot;
                }
            }
}
int __cdecl bh_navigation_build(BHNavigation *n, BHWorldGeometry *first, unsigned long scene_count, BHVFile *cache)
{
    BHNavigation next;
    BHWorldGeometry *p;
    unsigned long total, junctions, i, j, at, d, cell_count, word;
    float *stack = NULL, distance;
    long target, start = -1;
    BHNavigationError error = BH_NAV_OK;
    if (!n || !n->allocator.allocate || !n->allocator.release)
        return fail(n, BH_NAV_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(n, BH_NAV_NUMERICAL);
    if (!scene_counts(first, scene_count, &total, &junctions))
        return fail(n, BH_NAV_TOPOLOGY);
    if (junctions > 2048)
        return fail(n, BH_NAV_LIMIT);
    bh_navigation_init(&next, &n->allocator);
    next.node_count = total;
    next.junction_count = junctions;
    if (total)
    {
        next.nodes = (BHNavigationNode *)next.allocator.allocate(total * sizeof(BHNavigationNode), next.allocator.user);
        if (!next.nodes)
        {
            error = BH_NAV_MEMORY;
            goto rejected;
        }
        memset(next.nodes, 0, total * sizeof(BHNavigationNode));
        stack = (float *)next.allocator.allocate(total * 4, next.allocator.user);
        if (!stack)
        {
            error = BH_NAV_MEMORY;
            goto rejected;
        }
    }
    at = 0;
    p = first;
    for (i = 0; i < scene_count; ++i)
    {
        d = degree(p);
        if (d && d != 2)
            next.nodes[at++].placement = p;
        p = (BHWorldGeometry *)p->navigation.next;
    }
    p = first;
    for (i = 0; i < scene_count; ++i)
    {
        if (degree(p) == 2)
            next.nodes[at++].placement = p;
        p = (BHWorldGeometry *)p->navigation.next;
    }
    for (i = 0; i < total; ++i)
        for (j = 0; j < 6; ++j)
            if (next.nodes[i].placement->navigation.portals[j].neighbor &&
                bh_navigation_find(&next, (BHWorldGeometry *)next.nodes[i].placement->navigation.portals[j].neighbor) <
                    0)
            {
                error = BH_NAV_TOPOLOGY;
                goto rejected;
            }
    cell_count = junctions * junctions;
    if (cell_count)
    {
        next.routes.cells = (BHSceneRoute *)next.allocator.allocate(cell_count * 8, next.allocator.user);
        if (!next.routes.cells)
        {
            error = BH_NAV_MEMORY;
            goto rejected;
        }
        next.routes.node_count = junctions;
        word = 0x7e876abdUL;
        for (i = 0; i < cell_count; ++i)
        {
            next.routes.cells[i].exit_slot = -1;
            memcpy(&next.routes.cells[i].distance, &word, 4);
        }
        for (i = 0; i < junctions; ++i)
            next.routes.cells[i * junctions + i].distance = 0;
    }
    for (i = 0; i < junctions; ++i)
        for (j = 0; j < 6; ++j)
            if (next.nodes[i].placement->navigation.portals[j].neighbor)
            {
                target = bh_navigation_find(&next,
                                            (BHWorldGeometry *)next.nodes[i].placement->navigation.portals[j].neighbor);
                if (!corridor(&next, (long)i, (long)i, target, &distance, &target, stack))
                {
                    error = next.error;
                    goto rejected;
                }
                next.routes.cells[target * junctions + i].exit_slot = (long)j;
                next.routes.cells[target * junctions + i].distance = distance;
            }
    if (cache)
    {
        if (bh_vfile_seek(cache, 0, SEEK_CUR) || (start = bh_vfile_tell(cache)) < 0 ||
            bh_vfile_read(&word, 4, 1, cache) != 1)
        {
            error = BH_NAV_CACHE;
            goto rejected;
        }
        if (word == junctions)
        {
            if (bh_vfile_seek(cache, start, SEEK_SET) || !bh_scene_routes_read(&next.routes, cache, junctions))
            {
                error = BH_NAV_CACHE;
                goto rejected;
            }
            next.cache_used = 1;
        }
    }
    if (!next.cache_used)
        relax(&next);
    if (stack)
        next.allocator.release(stack, next.allocator.user);
    bh_navigation_dispose(n);
    *n = next;
    return 1;
rejected:
    if (cache && start >= 0)
        bh_vfile_seek(cache, start, SEEK_SET);
    if (stack)
        next.allocator.release(stack, next.allocator.user);
    bh_navigation_dispose(&next);
    return fail(n, error);
}
