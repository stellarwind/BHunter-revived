#include "bh_world_segment.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <float.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

static_assert(offsetof(BHEntity, kind) == 8, "BHEntity.kind offset");
static_assert(offsetof(BHEntity, opaque0) == 0x18, "BHEntity.opaque0 offset");
static_assert(offsetof(BHEntity, collision_radius) == 0xc0, "BHEntity.collision_radius offset");
static_assert(offsetof(BHEntity, query) == 0xd0, "BHEntity.query offset");
static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");

BHWorldSegmentRoom *bh_world_segment_room(BHWorldSegmentContext *segment_context, const BHWorldGeometry *w)
{
    unsigned long i;
    if (segment_context->room_index)
    {
        i = ((unsigned long)w >> 4) & segment_context->room_index_mask;
        while (segment_context->room_index[i])
        {
            if (segment_context->room_index[i]->world == w)
                return segment_context->room_index[i];
            i = (i + 1) & segment_context->room_index_mask;
        }
        return NULL;
    }
    for (i = 0; i < segment_context->room_count; ++i)
        if (segment_context->rooms[i].world == w)
            return segment_context->rooms + i;
    return NULL;
}
int bh_world_segment_index_rooms(BHWorldSegmentContext *segment_context, BHWorldSegmentRoom **workspace, unsigned long capacity)
{
    unsigned long i, slot;
    if (!segment_context || segment_context->busy)
        return 0;
    segment_context->room_index = NULL;
    segment_context->room_index_mask = 0;
    if (!segment_context->rooms || !segment_context->room_count || segment_context->room_count > 65535 || !workspace || capacity < segment_context->room_count * 2 ||
        capacity > 131072 || (capacity & (capacity - 1)))
        return 0;
    memset(workspace, 0, capacity * sizeof(*workspace));
    for (i = 0; i < segment_context->room_count; ++i)
    {
        if (!segment_context->rooms[i].world)
            return 0;
        slot = ((unsigned long)segment_context->rooms[i].world >> 4) & (capacity - 1);
        while (workspace[slot])
        {
            if (workspace[slot]->world == segment_context->rooms[i].world)
                return 0;
            slot = (slot + 1) & (capacity - 1);
        }
        workspace[slot] = segment_context->rooms + i;
    }
    segment_context->room_index = workspace;
    segment_context->room_index_mask = capacity - 1;
    return 1;
}
static int fail(BHWorldSegmentContext *segment_context, BHWorldSegmentError e)
{
    segment_context->error = e;
    return -1;
}
static int finite_vec(const BHVec3 *vector)
{
    return vector && _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static int contains(const BHWorldGeometry *world, const BHVec3 *point)
{
    return world->minimum.x <= point->x && point->x <= world->maximum.x && world->minimum.y <= point->y && point->y <= world->maximum.y &&
           world->minimum.z <= point->z && point->z <= world->maximum.z;
}
static int excluded(const BHWorldHit *self, BHWorldGeometry *w, BHEntity *e)
{
    return self && ((w && self->kind == BH_WORLD_HIT_GEOMETRY && self->object.geometry == w) ||
                    (e && self->kind == BH_WORLD_HIT_ACTOR && self->object.actor == e));
}
int bh_world_segment_init(BHWorldSegmentContext *q, BHWorldGeometry *first, unsigned long count,
                          BHWorldSegmentRoom *rooms, unsigned long room_count, const BHWorldSegmentActor *actors,
                          unsigned long actor_count, unsigned long max_room_actors)
{
    BHWorldSegmentContext next;
    BHWorldGeometry *world;
    unsigned long i, j;
    if (!q || !first || !count || count > 65535 || !rooms || room_count != count || actor_count > 65535 ||
        (actor_count && !actors) || !max_room_actors || max_room_actors > 65535)
        return 0;
    memset(&next, 0, sizeof(next));
    next.first = first;
    next.world_count = count;
    next.rooms = rooms;
    next.room_count = room_count;
    next.actors = actors;
    next.actor_count = actor_count;
    next.max_room_actors = max_room_actors;
    world = first;
    for (i = 0; i < count; ++i)
    {
        if (!world || (i && world == first) || !bh_world_segment_room(&next, world) || world->navigation.count > 6)
            return 0;
        for (j = 0; j < i; ++j)
            if (rooms[j].world == rooms[i].world)
                return 0;
        world = (BHWorldGeometry *)world->navigation.next;
    }
    if (world != first)
        return 0;
    for (i = 0; i < actor_count; ++i)
        if (!actors[i].entity || actors[i].entity->kind)
            return 0;
    for (i = 0; i < count; ++i)
    {
        world = rooms[i].world;
        if (world->owner && !bh_world_segment_room(&next, world->owner))
            return 0;
        for (j = 0; j < world->navigation.count; ++j)
            if (world->navigation.portals[j].neighbor &&
                !bh_world_segment_room(&next, (BHWorldGeometry *)world->navigation.portals[j].neighbor))
                return 0;
    }
    *q = next;
    return 1;
}
static double projection(const BHVec3 *point, const BHVec3 *start, const BHVec3 *direction)
{
    return (((double)point->x - start->x) * direction->x + ((double)point->z - start->z) * direction->z) +
           ((double)point->y - start->y) * direction->y;
}
static double distance_squared(double x, double y, double z)
{
    float sx, sy;
    bh_round_float(&sx, x);
    bh_round_float(&sy, y);
    return (x * sx + y * sy) + z * z;
}
static long material(unsigned long bits)
{
    long signed_bits;
    float stored;
    memcpy(&signed_bits, &bits, 4);
    bh_round_float(&stored, (double)signed_bits);
    /* Original CRT __ftol produces64 bits; only EAX is stored. The sole
       positive overflow of this signed32 input roundtrip is exactly2^31. */
    if (stored >= 2147483648.0)
        return (long)0x80000000u;
    return (long)stored;
}
/* First strict crossing in slot order. Original 0040ba40 compares the matched
   opening at portal+24 (+1e4 in the world object), not the model aperture. */
static BHWorldGeometry *crossing(BHWorldGeometry *world, const BHVec3 *a, const BHVec3 *b, BHVec3 *point)
{
    unsigned long i, axis, u, v;
    const float *start = (const float *)a, *end = (const float *)b;
    BHPortal *portal;
    double plane, t, s, du, dv, upper_u;
    float tt, ss, plane_float, stored_u, stored_v;
    BHVec3 at;
    for (i = 0; i < world->navigation.count; ++i)
    {
        portal = world->navigation.portals + i;
        if (!portal->neighbor || portal->direction > 6 || portal->direction == 3)
            continue;
        axis = portal->direction & 3;
        bh_round_float(&plane_float, (double)portal->plane);
        plane = plane_float;
        if (portal->direction < 4)
        {
            if (!(end[axis] < plane && plane < start[axis]))
                continue;
        }
        else if (!(start[axis] < plane && plane < end[axis]))
            continue;
        t = (plane - start[axis]) / ((double)end[axis] - start[axis]);
        s = ((double)end[axis] - plane) / ((double)end[axis] - start[axis]);
        bh_round_float(&tt, t);
        bh_round_float(&ss, s);
        u = axis == 0 ? 1 : 0;
        v = axis == 2 ? 1 : 2;
        /* X crossings reload both binary32 ratios. Y/Z retain both ratios
           for X, then reload them for their other aperture coordinate. */
        du = axis == 0 ? (double)end[u] * tt + (double)start[u] * ss : (double)end[u] * t + (double)start[u] * s;
        dv = (double)end[v] * tt + (double)start[v] * ss;
        bh_round_float(&stored_u, du);
        bh_round_float(&stored_v, dv);
        upper_u = axis == 0 ? stored_u : du;
        if (axis == 2)
            dv = stored_v; /* Z crossing stores Y before both tests. */
        if (portal->overlap[0] < du && upper_u < portal->overlap[1] && portal->overlap[2] < dv && dv < portal->overlap[3])
        {
            ((float *)&at)[axis] = plane_float;
            ((float *)&at)[u] = stored_u;
            ((float *)&at)[v] = stored_v;
            *point = at;
            return (BHWorldGeometry *)portal->neighbor;
        }
    }
    return NULL;
}
static int geometry(BHWorldSegmentContext *q, BHWorldGeometry *world, const BHVec3 *a, BHVec3 *end, BHWorldSegmentHit *out)
{
    BHWorldSegmentRoom *binding = bh_world_segment_room(q, world);
    BHSegmentHit hit;
    int result;
    if (!binding)
        return fail(q, BH_WORLD_SEGMENT_TOPOLOGY);
    if (!binding->query)
        return 0;
    result = bh_segment_query_run(binding->query, a, end, &world->transform, &world->inverse, &hit);
    if (result < 0)
        return fail(q, BH_WORLD_SEGMENT_DEPENDENCY);
    if (result)
    {
        *end = hit.point;
        out->point = hit.point;
        out->normal = hit.normal;
        out->object.kind = BH_WORLD_HIT_GEOMETRY;
        out->object.object.geometry = world;
        out->material_flags = material(hit.flags);
    }
    return result;
}
static BHEntity *actor_next(BHEntity *actor)
{
    BHEntity *next;
    memcpy(&next, &actor->actor.room_next, sizeof(next));
    return next;
}
static int bh_world_segment_room_trace(BHWorldSegmentContext *segment_context, const BHWorldHit *self, BHWorldGeometry *world,
                                       const BHVec3 *start, const BHVec3 *end, float radius, const BHVec3 *direction,
                                       BHWorldSegmentHit *out)
{
    BHVec3 a = *start, limit, hit_point, normal;
    BHWorldGeometry *neighbor, *child;
    BHWorldSegmentRoom *binding;
    BHEntity *actor;
    BHEntityPositionPacket packet;
    float length, along, rr;
    double dot, dx, dy, dz, ds, hit_length, px, py, pz;
    unsigned long steps, rooms = 0;
    int result, found;
    while (world)
    {
        if (++rooms > segment_context->world_count + 1)
            return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
        binding = bh_world_segment_room(segment_context, world);
        if (!binding)
            return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
        limit = *end;
        neighbor = crossing(world, &a, end, &limit);
        hit_point = limit;
        found = 0;
        bh_round_float(&length,
                       sqrt(distance_squared((double)limit.x - a.x, (double)limit.y - a.y, (double)limit.z - a.z)));
        if (radius > 0)
        {
            actor = binding->actors_owner ? *binding->actors_owner : NULL;
            steps = 0;
            packet.event = 3;
            while (actor)
            {
                if (++steps > segment_context->max_room_actors)
                    return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
                if (actor->kind)
                    return fail(segment_context, BH_WORLD_SEGMENT_ARGUMENT);
                if (!excluded(self, NULL, actor))
                {
                    double actor_radius = (double)radius * bh_room_actor_collision_scale(actor);
                    if (!actor->query)
                        return fail(segment_context, BH_WORLD_SEGMENT_DEPENDENCY);
                    actor->query(actor, &packet);
                    if (!finite_vec(&packet.position))
                        return fail(segment_context, BH_WORLD_SEGMENT_DEPENDENCY);
                    dot = projection(&packet.position, &a, direction);
                    bh_round_float(&along, dot);
                    if (dot >= 0 && along <= length)
                    {
                        dx = ((double)along * direction->x + a.x) - packet.position.x;
                        dy = ((double)along * direction->y + a.y) - packet.position.y;
                        dz = ((double)along * direction->z + a.z) - packet.position.z;
                        ds = distance_squared(dx, dy, dz);
                        bh_round_float(&rr, actor_radius * actor_radius);
                        if (ds < rr)
                        {
                            hit_length = (double)along - sqrt((double)rr - ds);
                            bh_round_float(&length, hit_length);
                            px = hit_length * direction->x + a.x;
                            py = hit_length * direction->y + a.y;
                            pz = hit_length * direction->z + a.z;
                            bh_round_float(&hit_point.x, px);
                            bh_round_float(&hit_point.y, py);
                            bh_round_float(&hit_point.z, pz);
                            bh_round_float(&normal.x, px - packet.position.x);
                            bh_round_float(&normal.y, py - packet.position.y);
                            bh_round_float(&normal.z, pz - packet.position.z);
                            bh_vec3_normalize(&normal);
                            out->point = hit_point;
                            out->normal = normal;
                            out->object.kind = BH_WORLD_HIT_ACTOR;
                            out->object.object.actor = actor;
                            out->material_flags = 0;
                            found = 1;
                        }
                    }
                }
                actor = actor_next(actor);
            }
        }
        if ((world->query_flags & 2) && !excluded(self, world, NULL))
        {
            result = geometry(segment_context, world, &a, &hit_point, out);
            if (result < 0)
                return -1;
            if (result)
                found = 1;
        }
        child = (BHWorldGeometry *)world->navigation.next;
        steps = 0;
        while (child && child->owner)
        {
            if (++steps > segment_context->world_count || !bh_world_segment_room(segment_context, child))
                return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
            if ((child->query_flags & 2) && !excluded(self, child, NULL) &&
                (child->minimum.x <= a.x || child->minimum.x <= hit_point.x) &&
                (a.x <= child->maximum.x || hit_point.x <= child->maximum.x) &&
                (child->minimum.y <= a.y || child->minimum.y <= hit_point.y) &&
                (a.y <= child->maximum.y || hit_point.y <= child->maximum.y) &&
                (child->minimum.z <= a.z || child->minimum.z <= hit_point.z) &&
                (a.z <= child->maximum.z || hit_point.z <= child->maximum.z))
            {
                result = geometry(segment_context, child, &a, &hit_point, out);
                if (result < 0)
                    return -1;
                if (result)
                    found = 1;
            }
            child = (BHWorldGeometry *)child->navigation.next;
        }
        if (!child)
            return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
        if (found)
            return 1;
        if (!neighbor)
            return 0;
        a = limit;
        world = neighbor;
    }
    return 0;
}
static void actor_pose(const BHEntity *entity, BHMat34 *pose)
{
    unsigned long i, j;
    float value;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
        {
            memcpy(&value, entity->opaque0 + (i * 3 + j) * 4, 4);
            pose->m[j][i] = value;
        }
    pose->m[0][3] = entity->position.x;
    pose->m[1][3] = entity->position.y;
    pose->m[2][3] = entity->position.z;
}
static int bh_world_segment_actor_trace(BHWorldSegmentContext *segment_context, const BHWorldHit *self, const BHVec3 *vector,
                                        const BHVec3 *end, const BHVec3 *direction, BHWorldSegmentHit *out)
{
    unsigned long i;
    BHEntity *entity;
    BHMat34 pose, inverse;
    BHVec3 limit = *end;
    BHSegmentHit hit;
    float length, along, stored_y, stored_z;
    double dot, dx, dy, dz;
    int found = 0, result;
    bh_round_float(&length, projection(end, vector, direction));
    for (i = 0; i < segment_context->actor_count; ++i)
    {
        entity = segment_context->actors[i].entity;
        if (!entity)
            return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
        if (entity->kind)
            return fail(segment_context, BH_WORLD_SEGMENT_ARGUMENT);
        if (excluded(self, NULL, entity))
            continue;
        dot = projection(&entity->position, vector, direction);
        bh_round_float(&along, dot);
        if (!(-(double)entity->collision_radius < dot && along < (double)length + entity->collision_radius))
            continue;
        dx = ((double)along * direction->x + vector->x) - entity->position.x;
        dy = ((double)along * direction->y + vector->y) - entity->position.y;
        dz = ((double)along * direction->z + vector->z) - entity->position.z;
        bh_round_float(&stored_y, dy);
        bh_round_float(&stored_z, dz);
        if (!((dx * dx + dy * stored_y) + dz * stored_z < (double)entity->collision_radius * entity->collision_radius))
            continue;
        if (!segment_context->actors[i].query)
            continue;
        actor_pose(entity, &pose);
        bh_mat34_inverse_rigid(&pose, &inverse);
        result = bh_segment_query_run(segment_context->actors[i].query, vector, &limit, &pose, &inverse, &hit);
        if (result < 0)
            return fail(segment_context, BH_WORLD_SEGMENT_DEPENDENCY);
        if (result)
        {
            dot = (((double)hit.point.y - vector->y) * direction->y + ((double)hit.point.x - vector->x) * direction->x) +
                  ((double)hit.point.z - vector->z) * direction->z;
            bh_round_float(&along, dot);
            if (dot < length)
            {
                length = along;
                limit = hit.point;
                out->point = hit.point;
                out->normal = hit.normal;
                out->object.kind = BH_WORLD_HIT_ACTOR;
                out->object.object.actor = entity;
                found = 1;
                /* Original global actor pass does not overwrite output+2c. */
            }
        }
    }
    return found;
}
int bh_world_segment_run(BHWorldSegmentContext *segment_context, const BHWorldHit *self, BHWorldGeometry **cached,
                         const BHVec3 *start, const BHVec3 *end, float radius, BHWorldSegmentHit *out)
{
    BHWorldGeometry *owner, *old;
    BHWorldSegmentHit hit, original;
    BHVec3 direction, limit;
    int a, b;
    double denominator, numerator;
    if (!segment_context)
        return -1;
    if (!cached || !out)
        return fail(segment_context, BH_WORLD_SEGMENT_ARGUMENT);
    if (segment_context->busy)
        return fail(segment_context, BH_WORLD_SEGMENT_BUSY);
    if (!finite_vec(start) || !finite_vec(end) || !_finite(radius) || !bh_math_environment_supported())
        return fail(segment_context, BH_WORLD_SEGMENT_ARGUMENT);
    if (segment_context->refresh_actors && !segment_context->refresh_actors(segment_context, segment_context->refresh_user))
        return fail(segment_context, BH_WORLD_SEGMENT_DEPENDENCY);
    old = *cached;
    if (old && !bh_world_segment_room(segment_context, old))
        return fail(segment_context, BH_WORLD_SEGMENT_TOPOLOGY);
    original = *out;
    hit = original;
    hit.material_flags = 0;
    owner = old;
    if (!owner || !contains(owner, start))
    {
        owner = bh_world_locate(segment_context->first, segment_context->world_count, old, start, 0);
        if (!owner)
            owner = old;
    }
    if (!owner)
    {
        out->material_flags = 0;
        segment_context->error = BH_WORLD_SEGMENT_OK;
        return 0;
    }
    bh_round_float(&direction.x, (double)end->x - start->x);
    bh_round_float(&direction.y, (double)end->y - start->y);
    bh_round_float(&direction.z, (double)end->z - start->z);
    bh_vec3_normalize(&direction);
    *cached = owner;
    out->material_flags = 0;
    segment_context->busy = 1;
    segment_context->error = BH_WORLD_SEGMENT_OK;
    a = bh_world_segment_room_trace(segment_context, self, owner, start, end, radius, &direction, &hit);
    if (a < 0)
    {
        segment_context->busy = 0;
        *cached = old;
        *out = original;
        return -1;
    }
    limit = a ? hit.point : *end;
    b = bh_world_segment_actor_trace(segment_context, self, start, &limit, &direction, &hit);
    segment_context->busy = 0;
    if (b < 0)
    {
        *cached = old;
        *out = original;
        return -1;
    }
    if (a || b)
    {
        numerator = projection(&hit.point, start, &direction);
        denominator = (((double)end->z - start->z) * direction.z + ((double)end->y - start->y) * direction.y) +
                      ((double)end->x - start->x) * direction.x;
        bh_round_float(&hit.fraction, numerator / denominator);
        *out = hit;
    }
    else
        out->material_flags = 0;
    return a || b;
}
