#include "bh_world_query.h"
#include "../Math/bh_fp.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static_assert(offsetof(BHEntity, collision_radius) == 0xc0, "BHEntity.collision_radius offset");
typedef struct Frame
{
    BHWorldGeometry *world;
    unsigned long slot;
} Frame;
static void *allocate(unsigned long n, void *user)
{
    return malloc(n);
}
static void release(void *p, void *user)
{
    free(p);
}
static int fail(BHWorldQuery *world_query, BHWorldQueryError e)
{
    if (world_query)
        world_query->error = e;
    return 0;
}
static long index_of(const BHWorldQuery *world_query, const BHWorldGeometry *p)
{
    return bh_binding_find(&world_query->member_index, p, NULL);
}
int bh_world_query_dispose(BHWorldQuery *world_query)
{
    BHSceneAllocator a;
    if (!world_query || world_query->busy)
        return fail(world_query, BH_QUERY_BUSY);
    a = world_query->allocator;
    bh_binding_dispose(&world_query->member_index, &a);
    if (world_query->members)
        a.release(world_query->members, a.user);
    if (world_query->visited)
        a.release(world_query->visited, a.user);
    if (world_query->frames)
        a.release(world_query->frames, a.user);
    if (world_query->hits)
        a.release(world_query->hits, a.user);
    if (world_query->work)
        a.release(world_query->work, a.user);
    memset(world_query, 0, sizeof(*world_query));
    world_query->allocator = a;
    return 1;
}
int bh_world_query_init(BHWorldQuery *q, BHWorldGeometry *first, unsigned long count, BHEntity *const *actors,
                        unsigned long actor_count, const BHSceneAllocator *allocator)
{
    BHWorldQuery next;
    BHWorldGeometry *world;
    unsigned long i, j;
    if (!q || count > 65535 || actor_count > 65535 || (!!first != !!count) || (actor_count && !actors) ||
        (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(&next, 0, sizeof(next));
    next.first = first;
    next.scene_count = count;
    next.actors = actors;
    next.actor_count = actor_count;
    if (allocator)
        next.allocator = *allocator;
    else
    {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    next.capacity = count + actor_count + 1;
    if (count)
    {
        next.members = next.allocator.allocate(count * sizeof(BHWorldGeometry *), next.allocator.user);
        next.visited = next.allocator.allocate(count, next.allocator.user);
        next.frames = next.allocator.allocate(count * sizeof(Frame), next.allocator.user);
        if (!next.members || !next.visited || !next.frames ||
            !bh_binding_init(&next.member_index, count, &next.allocator))
            goto rejected;
        world = first;
        for (i = 0; i < count; ++i)
        {
            if (!world || (i && world == first) || world->navigation.count > 6)
                goto rejected;
            ((BHWorldGeometry **)next.members)[i] = world;
            if (!bh_binding_insert(&next.member_index, world, NULL, i))
                goto rejected;
            world = (BHWorldGeometry *)world->navigation.next;
        }
        if (world != first)
            goto rejected;
        for (i = 0; i < count; ++i)
        {
            world = ((BHWorldGeometry **)next.members)[i];
            for (j = 0; j < world->navigation.count; ++j)
                if (world->navigation.portals[j].neighbor &&
                    index_of(&next, (BHWorldGeometry *)world->navigation.portals[j].neighbor) < 0)
                    goto rejected;
        }
    }
    for (i = 0; i < actor_count; ++i)
        if (!actors[i])
            goto rejected;
    next.hits = (BHWorldHit *)next.allocator.allocate(next.capacity * sizeof(BHWorldHit), next.allocator.user);
    next.work = (BHWorldHit *)next.allocator.allocate(next.capacity * sizeof(BHWorldHit), next.allocator.user);
    if (!next.hits || !next.work)
        goto rejected;
    next.hits[0].kind = BH_WORLD_HIT_END;
    next.hits[0].object.geometry = NULL;
    *q = next;
    return 1;
rejected:
    bh_world_query_dispose(&next);
    return 0;
}
static int append(BHWorldQuery *world_query, unsigned long *count, BHWorldGeometry *world, BHEntity *actor)
{
    if (*count >= world_query->capacity - 1)
        return fail(world_query, BH_QUERY_CAPACITY);
    world_query->work[*count].kind = world ? BH_WORLD_HIT_GEOMETRY : BH_WORLD_HIT_ACTOR;
    if (world)
        world_query->work[*count].object.geometry = world;
    else
        world_query->work[*count].object.actor = actor;
    ++*count;
    return 1;
}
static int box(const BHWorldGeometry *world, const BHVec3 *point, float r)
{
    return world->minimum.x <= (double)point->x + r && (double)point->x - r <= world->maximum.x && world->minimum.y <= (double)point->y + r &&
           (double)point->y - r <= world->maximum.y && world->minimum.z <= (double)point->z + r && (double)point->z - r <= world->maximum.z;
}
static int portal_box(const BHPortal *portal, const float *lower, const float *upper)
{
    unsigned long axis, u, v;
    if (portal->direction > 6 || portal->direction == 3)
        return 0;
    axis = portal->direction & 3;
    u = axis == 0 ? 1 : 0;
    v = axis == 2 ? 1 : 2;
    return lower[axis] <= (double)portal->plane && (double)portal->plane <= upper[axis] && lower[u] <= portal->aperture[1] &&
           portal->aperture[0] <= upper[u] && lower[v] <= portal->aperture[3] && portal->aperture[2] <= upper[v];
}
static int enter(BHWorldQuery *world_query, BHWorldGeometry *world, const BHVec3 *point, float r, unsigned long *count,
                 Frame *stack, unsigned long *depth)
{
    long index = index_of(world_query, world);
    BHWorldGeometry *child;
    unsigned long steps = 0;
    if (index < 0 || world->navigation.count > 6)
        return fail(world_query, BH_QUERY_TOPOLOGY);
    if (((unsigned char *)world_query->visited)[index])
        return 1;
    ((unsigned char *)world_query->visited)[index] = 1;
    if (!append(world_query, count, world, NULL))
        return 0;
    child = (BHWorldGeometry *)world->navigation.next;
    while (child->owner)
    {
        if (++steps > world_query->scene_count)
            return fail(world_query, BH_QUERY_TOPOLOGY);
        if ((child->query_flags & 2) && box(child, point, r) && !append(world_query, count, child, NULL))
            return 0;
        child = (BHWorldGeometry *)child->navigation.next;
    }
    if (*depth >= world_query->scene_count)
        return fail(world_query, BH_QUERY_TOPOLOGY);
    stack[*depth].world = world;
    stack[*depth].slot = 0;
    ++*depth;
    return 1;
}
static int collect(BHWorldQuery *world_query, BHWorldGeometry *start, const BHVec3 *p, float r, unsigned long *count)
{
    Frame *stack = (Frame *)world_query->frames;
    unsigned long depth = 0, i;
    float lower[3], upper[3];
    const float *point = (const float *)p;
    for (i = 0; i < 3; ++i)
    {
        bh_round_float(lower + i, (double)point[i] - r);
        bh_round_float(upper + i, (double)point[i] + r);
    }
    if (!enter(world_query, start, p, r, count, stack, &depth))
        return 0;
    while (depth)
    {
        Frame *frame = stack + depth - 1;
        BHPortal *portal;
        if (frame->slot >= frame->world->navigation.count)
        {
            --depth;
            continue;
        }
        portal = frame->world->navigation.portals + frame->slot++;
        if (portal->neighbor && portal_box(portal, lower, upper) &&
            !enter(world_query, (BHWorldGeometry *)portal->neighbor, p, r, count, stack, &depth))
            return 0;
    }
    return 1;
}
static int actors(BHWorldQuery *world_query, BHEntity *self, const BHVec3 *position, float r, unsigned long *count)
{
    unsigned long i;
    BHEntity *actor;
    for (i = 0; i < world_query->actor_count; ++i)
    {
        actor = world_query->actors[i];
        if (actor == self)
            continue;
        if ((double)actor->position.x - actor->collision_radius < (double)position->x + r &&
            (double)position->x - r < (double)actor->position.x + actor->collision_radius &&
            (double)actor->position.y - actor->collision_radius < (double)position->y + r &&
            (double)position->y - r < (double)actor->position.y + actor->collision_radius &&
            (double)actor->position.z - actor->collision_radius < (double)position->z + r &&
            (double)position->z - r < (double)actor->position.z + actor->collision_radius && !append(world_query, count, NULL, actor))
            return 0;
    }
    return 1;
}
int bh_world_query_run(BHWorldQuery *world_query, BHEntity *self, BHWorldGeometry *previous, const BHVec3 *point, float margin,
                       BHWorldGeometry **owner)
{
    BHWorldGeometry *selected = previous;
    BHWorldHit *swap;
    unsigned long count = 0, i;
    if (!world_query || !world_query->hits || !world_query->work || !owner || !point || world_query->busy || !_finite(point->x) || !_finite(point->y) ||
        !_finite(point->z) || !_finite(margin) || !bh_math_environment_supported())
        return fail(world_query, BH_QUERY_ARGUMENT);
    if (previous && index_of(world_query, previous) < 0)
        return fail(world_query, BH_QUERY_TOPOLOGY);
    for (i = 0; i < world_query->actor_count; ++i)
        if (!world_query->actors[i] || !_finite(world_query->actors[i]->position.x) || !_finite(world_query->actors[i]->position.y) ||
            !_finite(world_query->actors[i]->position.z) || !_finite(world_query->actors[i]->collision_radius))
            return fail(world_query, BH_QUERY_ARGUMENT);
    world_query->busy = 1;
    if (world_query->visited)
        memset(world_query->visited, 0, world_query->scene_count);
    if (selected && !box(selected, point, 0))
        selected = NULL;
    if (!selected)
        selected = bh_world_locate(world_query->first, world_query->scene_count, previous, point, 0);
    if (!selected)
        selected = bh_world_locate(world_query->first, world_query->scene_count, previous, point, margin);
    if (!selected)
        selected = previous;
    if (selected && !collect(world_query, selected, point, margin, &count))
        goto rejected;
    if (!actors(world_query, self, point, margin, &count))
        goto rejected;
    world_query->work[count].kind = BH_WORLD_HIT_END;
    world_query->work[count].object.geometry = NULL;
    swap = world_query->hits;
    world_query->hits = world_query->work;
    world_query->work = swap;
    world_query->hit_count = count;
    *owner = selected;
    if (world_query->visited)
        memset(world_query->visited, 0, world_query->scene_count);
    world_query->busy = 0;
    world_query->error = BH_QUERY_OK;
    return 1;
rejected:
    if (world_query->visited)
        memset(world_query->visited, 0, world_query->scene_count);
    world_query->busy = 0;
    return 0;
}
