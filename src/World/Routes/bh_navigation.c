#include "bh_navigation.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

static void *allocate(unsigned long n, void *user)
{
    return malloc(n);
}
static void release(void *p, void *user)
{
    free(p);
}
static int fail(BHNavigation *navigation, BHNavigationError error)
{
    if (navigation)
        navigation->error = error;
    return 0;
}
int bh_navigation_init(BHNavigation *navigation, const BHSceneAllocator *allocator)
{
    BHSceneAllocator memory;
    if (!navigation || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    if (allocator)
        memory = *allocator;
    else
    {
        memory.allocate = allocate;
        memory.release = release;
        memory.user = NULL;
    }
    memset(navigation, 0, sizeof(*navigation));
    navigation->allocator = memory;
    bh_scene_routes_init(&navigation->routes, &memory);
    return 1;
}
void bh_navigation_dispose(BHNavigation *navigation)
{
    BHSceneAllocator memory;
    if (!navigation)
        return;
    memory = navigation->allocator;
    if (navigation->nodes)
        memory.release(navigation->nodes, memory.user);
    bh_scene_routes_dispose(&navigation->routes);
    bh_navigation_init(navigation, &memory);
}
long bh_navigation_find(const BHNavigation *navigation, const BHWorldGeometry *room)
{
    unsigned long i;
    if (!navigation || !room)
        return -1;
    for (i = 0; i < navigation->node_count; ++i)
        if (navigation->nodes[i].placement == room)
            return (long)i;
    return -1;
}
static unsigned long degree(const BHWorldGeometry *room)
{
    unsigned long i, count = 0;
    if (!room->owner)
        for (i = 0; i < 6; ++i)
            if (room->navigation.portals[i].neighbor)
                ++count;
    return count;
}
static int scene_counts(BHWorldGeometry *first, unsigned long count, unsigned long *total, unsigned long *junctions)
{
    BHWorldGeometry *room = first;
    unsigned long i, j, portal_count;
    const float *bounds;
    *total = *junctions = 0;
    if (!room || !count || count > 65535)
        return 0;
    for (i = 0; i < count; ++i)
    {
        if (!room || (i && room == first))
            return 0;
        bounds = (const float *)&room->minimum;
        for (j = 0; j < 6; ++j)
            if (!_finite(bounds[j]))
                return 0;
        for (j = 0; j < 3; ++j)
            if (bounds[j] > bounds[j + 3])
                return 0;
        portal_count = degree(room);
        if (portal_count)
        {
            ++*total;
            if (portal_count != 2)
                ++*junctions;
        }
        room = (BHWorldGeometry *)room->navigation.next;
    }
    return room == first;
}
/* Assembly 00407d9f..00407e98 keeps current centers unrounded, but spills
   previous centers and one operand of each squared delta to binary32. */
static int edge_distance(const BHWorldGeometry *previous, const BHWorldGeometry *current, float *out)
{
    const float *previous_bounds = (const float *)&previous->minimum,
                *current_bounds = (const float *)&current->minimum;
    double delta[3];
    float center, rounded_delta[3];
    unsigned long i;
    for (i = 0; i < 3; ++i)
    {
        bh_round_float(&center, ((double)previous_bounds[i] + previous_bounds[i + 3]) * 0.5);
        delta[i] = ((double)current_bounds[i] + current_bounds[i + 3]) * 0.5 - center;
        bh_round_float(rounded_delta + i, delta[i]);
    }
    bh_round_float(out,
                   sqrt((delta[0] * rounded_delta[0] + delta[1] * rounded_delta[1]) + delta[2] * rounded_delta[2]));
    return _finite(*out) && *out >= 0;
}
static int corridor(BHNavigation *navigation, long origin, long previous, long current, float *distance,
                    long *destination, float *stack)
{
    unsigned long depth = 0, i, seen;
    long next;
    BHWorldGeometry *room;
    for (;;)
    {
        if (depth >= navigation->node_count)
            return fail(navigation, BH_NAV_CYCLE);
        if (!edge_distance(navigation->nodes[previous].placement, navigation->nodes[current].placement, stack + depth))
            return fail(navigation, BH_NAV_NUMERICAL);
        if ((unsigned long)current < navigation->junction_count)
        {
            *destination = current;
            *distance = stack[depth];
            break;
        }
        room = navigation->nodes[current].placement;
        seen = 0;
        next = -1;
        for (i = 0; i < 6; ++i)
            if (room->navigation.portals[i].neighbor)
            {
                long candidate =
                    bh_navigation_find(navigation, (BHWorldGeometry *)room->navigation.portals[i].neighbor);
                if (candidate < 0)
                    return fail(navigation, BH_NAV_TOPOLOGY);
                if (candidate != previous)
                {
                    next = candidate;
                    if (seen)
                    {
                        navigation->nodes[current].junction_first_port = origin;
                        navigation->nodes[current].endpoints_valid |= 1;
                    }
                    else
                    {
                        navigation->nodes[current].junction_second_port = origin;
                        navigation->nodes[current].endpoints_valid |= 2;
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
static void relax(BHNavigation *navigation)
{
    unsigned long via, destination, source, count = navigation->junction_count;
    BHSceneRoute *cells = navigation->routes.cells;
    for (via = 0; via < count; ++via)
        for (destination = 0; destination < count; ++destination)
            for (source = 0; source < count; ++source)
            {
                double candidate =
                    (double)cells[source * count + via].distance + cells[via * count + destination].distance;
                if (candidate < cells[source * count + destination].distance)
                {
                    float rounded;
                    bh_round_float(&rounded, candidate);
                    cells[source * count + destination].distance = cells[destination * count + source].distance =
                        rounded;
                    cells[source * count + destination].exit_slot = cells[via * count + destination].exit_slot;
                    cells[destination * count + source].exit_slot = cells[via * count + source].exit_slot;
                }
            }
}
int bh_navigation_build(BHNavigation *navigation, BHWorldGeometry *first, unsigned long scene_count, BHVFile *cache)
{
    BHNavigation next;
    BHWorldGeometry *room;
    unsigned long total, junctions, i, j, at, portal_count, cell_count, word;
    float *stack = NULL, distance;
    long target, start = -1;
    BHNavigationError error = BH_NAV_OK;
    if (!navigation || !navigation->allocator.allocate || !navigation->allocator.release)
        return fail(navigation, BH_NAV_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(navigation, BH_NAV_NUMERICAL);
    if (!scene_counts(first, scene_count, &total, &junctions))
        return fail(navigation, BH_NAV_TOPOLOGY);
    if (junctions > 2048)
        return fail(navigation, BH_NAV_LIMIT);
    bh_navigation_init(&next, &navigation->allocator);
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
    room = first;
    for (i = 0; i < scene_count; ++i)
    {
        portal_count = degree(room);
        if (portal_count && portal_count != 2)
            next.nodes[at++].placement = room;
        room = (BHWorldGeometry *)room->navigation.next;
    }
    room = first;
    for (i = 0; i < scene_count; ++i)
    {
        if (degree(room) == 2)
            next.nodes[at++].placement = room;
        room = (BHWorldGeometry *)room->navigation.next;
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
    bh_navigation_dispose(navigation);
    *navigation = next;
    return 1;
rejected:
    if (cache && start >= 0)
        bh_vfile_seek(cache, start, SEEK_SET);
    if (stack)
        next.allocator.release(stack, next.allocator.user);
    bh_navigation_dispose(&next);
    return fail(navigation, error);
}
