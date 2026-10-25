#include "bh_world_query.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char radius_offset[offsetof(BHEntity, collision_radius) == 0xc0 ? 1 : -1];
typedef struct Frame {
    BHWorldGeometry *world;
    unsigned long slot;
} Frame;
static void *__cdecl allocate(unsigned long n, void *user) {
    return malloc(n);
}
static void __cdecl release(void *p, void *user) {
    free(p);
}
static int fail(BHWorldQuery *q, BHWorldQueryError e) {
    if (q)
        q->error = e;
    return 0;
}
static long index_of(const BHWorldQuery *q, const BHWorldGeometry *p) {
    BHWorldGeometry **members = (BHWorldGeometry **)q->members;
    unsigned long i;
    for (i = 0; i < q->scene_count; ++i)
        if (members[i] == p)
            return (long)i;
    return -1;
}
int __cdecl bh_world_query_dispose(BHWorldQuery *q) {
    BHSceneAllocator a;
    if (!q || q->busy)
        return fail(q, BH_QUERY_BUSY);
    a = q->allocator;
    if (q->members)
        a.release(q->members, a.user);
    if (q->visited)
        a.release(q->visited, a.user);
    if (q->frames)
        a.release(q->frames, a.user);
    if (q->hits)
        a.release(q->hits, a.user);
    if (q->work)
        a.release(q->work, a.user);
    memset(q, 0, sizeof(*q));
    q->allocator = a;
    return 1;
}
int __cdecl bh_world_query_init(BHWorldQuery *q, BHWorldGeometry *first, unsigned long count,
                                BHEntity *const *actors, unsigned long actor_count,
                                const BHSceneAllocator *allocator) {
    BHWorldQuery next;
    BHWorldGeometry *p;
    unsigned long i, j;
    if (!q || count > 65535 || actor_count > 65535 || (!!first != !!count) ||
        (actor_count && !actors) || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(&next, 0, sizeof(next));
    next.first = first;
    next.scene_count = count;
    next.actors = actors;
    next.actor_count = actor_count;
    if (allocator)
        next.allocator = *allocator;
    else {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    next.capacity = count + actor_count + 1;
    if (count) {
        next.members =
            next.allocator.allocate(count * sizeof(BHWorldGeometry *), next.allocator.user);
        next.visited = next.allocator.allocate(count, next.allocator.user);
        next.frames = next.allocator.allocate(count * sizeof(Frame), next.allocator.user);
        if (!next.members || !next.visited || !next.frames)
            goto rejected;
        p = first;
        for (i = 0; i < count; ++i) {
            if (!p || (i && p == first) || p->navigation.count > 6)
                goto rejected;
            ((BHWorldGeometry **)next.members)[i] = p;
            p = (BHWorldGeometry *)p->navigation.next;
        }
        if (p != first)
            goto rejected;
        for (i = 0; i < count; ++i) {
            p = ((BHWorldGeometry **)next.members)[i];
            for (j = 0; j < p->navigation.count; ++j)
                if (p->navigation.portals[j].neighbor &&
                    index_of(&next, (BHWorldGeometry *)p->navigation.portals[j].neighbor) < 0)
                    goto rejected;
        }
    }
    for (i = 0; i < actor_count; ++i)
        if (!actors[i])
            goto rejected;
    next.hits = (BHWorldHit *)next.allocator.allocate(next.capacity * sizeof(BHWorldHit),
                                                      next.allocator.user);
    next.work = (BHWorldHit *)next.allocator.allocate(next.capacity * sizeof(BHWorldHit),
                                                      next.allocator.user);
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
static int append(BHWorldQuery *q, unsigned long *count, BHWorldGeometry *world, BHEntity *actor) {
    if (*count >= q->capacity - 1)
        return fail(q, BH_QUERY_CAPACITY);
    q->work[*count].kind = world ? BH_WORLD_HIT_GEOMETRY : BH_WORLD_HIT_ACTOR;
    if (world)
        q->work[*count].object.geometry = world;
    else
        q->work[*count].object.actor = actor;
    ++*count;
    return 1;
}
static int box(const BHWorldGeometry *w, const BHVec3 *p, float r) {
    return w->minimum.x <= (double)p->x + r && (double)p->x - r <= w->maximum.x &&
           w->minimum.y <= (double)p->y + r && (double)p->y - r <= w->maximum.y &&
           w->minimum.z <= (double)p->z + r && (double)p->z - r <= w->maximum.z;
}
static int portal_box(const BHPortal *p, const float *lower, const float *upper) {
    unsigned long axis, u, v;
    if (p->direction > 6 || p->direction == 3)
        return 0;
    axis = p->direction & 3;
    u = axis == 0 ? 1 : 0;
    v = axis == 2 ? 1 : 2;
    return lower[axis] <= (double)p->plane && (double)p->plane <= upper[axis] &&
           lower[u] <= p->aperture[1] && p->aperture[0] <= upper[u] && lower[v] <= p->aperture[3] &&
           p->aperture[2] <= upper[v];
}
static int enter(BHWorldQuery *q, BHWorldGeometry *world, const BHVec3 *point, float r,
                 unsigned long *count, Frame *stack, unsigned long *depth) {
    long index = index_of(q, world);
    BHWorldGeometry *child;
    unsigned long steps = 0;
    if (index < 0)
        return fail(q, BH_QUERY_TOPOLOGY);
    if (((unsigned char *)q->visited)[index])
        return 1;
    ((unsigned char *)q->visited)[index] = 1;
    if (!append(q, count, world, NULL))
        return 0;
    child = (BHWorldGeometry *)world->navigation.next;
    while (child->owner) {
        if (++steps > q->scene_count)
            return fail(q, BH_QUERY_TOPOLOGY);
        if ((child->query_flags & 2) && box(child, point, r) && !append(q, count, child, NULL))
            return 0;
        child = (BHWorldGeometry *)child->navigation.next;
    }
    if (*depth >= q->scene_count)
        return fail(q, BH_QUERY_TOPOLOGY);
    stack[*depth].world = world;
    stack[*depth].slot = 0;
    ++*depth;
    return 1;
}
static int collect(BHWorldQuery *q, BHWorldGeometry *start, const BHVec3 *p, float r,
                   unsigned long *count) {
    Frame *stack = (Frame *)q->frames;
    unsigned long depth = 0, i;
    float lower[3], upper[3];
    const float *point = (const float *)p;
    for (i = 0; i < 3; ++i) {
        bh_round_float(lower + i, (double)point[i] - r);
        bh_round_float(upper + i, (double)point[i] + r);
    }
    if (!enter(q, start, p, r, count, stack, &depth))
        return 0;
    while (depth) {
        Frame *frame = stack + depth - 1;
        BHPortal *portal;
        if (frame->slot >= frame->world->navigation.count) {
            --depth;
            continue;
        }
        portal = frame->world->navigation.portals + frame->slot++;
        if (portal->neighbor && portal_box(portal, lower, upper) &&
            !enter(q, (BHWorldGeometry *)portal->neighbor, p, r, count, stack, &depth))
            return 0;
    }
    return 1;
}
static int actors(BHWorldQuery *q, BHEntity *self, const BHVec3 *p, float r, unsigned long *count) {
    unsigned long i;
    BHEntity *a;
    for (i = 0; i < q->actor_count; ++i) {
        a = q->actors[i];
        if (a == self)
            continue;
        if ((double)a->position.x - a->collision_radius < (double)p->x + r &&
            (double)p->x - r < (double)a->position.x + a->collision_radius &&
            (double)a->position.y - a->collision_radius < (double)p->y + r &&
            (double)p->y - r < (double)a->position.y + a->collision_radius &&
            (double)a->position.z - a->collision_radius < (double)p->z + r &&
            (double)p->z - r < (double)a->position.z + a->collision_radius &&
            !append(q, count, NULL, a))
            return 0;
    }
    return 1;
}
int __cdecl bh_world_query_run(BHWorldQuery *q, BHEntity *self, BHWorldGeometry *previous,
                               const BHVec3 *point, float margin, BHWorldGeometry **owner) {
    BHWorldGeometry *selected = previous;
    BHWorldHit *swap;
    unsigned long count = 0, i;
    if (!q || !q->hits || !q->work || !owner || !point || q->busy || !_finite(point->x) ||
        !_finite(point->y) || !_finite(point->z) || !_finite(margin) ||
        !bh_math_environment_supported())
        return fail(q, BH_QUERY_ARGUMENT);
    if (previous && index_of(q, previous) < 0)
        return fail(q, BH_QUERY_TOPOLOGY);
    for (i = 0; i < q->scene_count; ++i) {
        BHWorldGeometry *w = ((BHWorldGeometry **)q->members)[i];
        if (w->navigation.count > 6 ||
            w->navigation.next !=
                &((BHWorldGeometry **)q->members)[(i + 1) % q->scene_count]->navigation)
            return fail(q, BH_QUERY_TOPOLOGY);
    }
    for (i = 0; i < q->actor_count; ++i)
        if (!q->actors[i] || !_finite(q->actors[i]->position.x) ||
            !_finite(q->actors[i]->position.y) || !_finite(q->actors[i]->position.z) ||
            !_finite(q->actors[i]->collision_radius))
            return fail(q, BH_QUERY_ARGUMENT);
    q->busy = 1;
    if (q->visited)
        memset(q->visited, 0, q->scene_count);
    if (selected && !box(selected, point, 0))
        selected = NULL;
    if (!selected)
        selected = bh_world_locate(q->first, q->scene_count, previous, point, 0);
    if (!selected)
        selected = bh_world_locate(q->first, q->scene_count, previous, point, margin);
    if (!selected)
        selected = previous;
    if (selected && !collect(q, selected, point, margin, &count))
        goto rejected;
    if (!actors(q, self, point, margin, &count))
        goto rejected;
    q->work[count].kind = BH_WORLD_HIT_END;
    q->work[count].object.geometry = NULL;
    swap = q->hits;
    q->hits = q->work;
    q->work = swap;
    q->hit_count = count;
    *owner = selected;
    if (q->visited)
        memset(q->visited, 0, q->scene_count);
    q->busy = 0;
    q->error = BH_QUERY_OK;
    return 1;
rejected:
    if (q->visited)
        memset(q->visited, 0, q->scene_count);
    q->busy = 0;
    return 0;
}
