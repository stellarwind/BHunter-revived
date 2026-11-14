#include "bh_actor_route.h"
#include "../../Camera/bh_axis_snap.h"
#include "../../World/Routes/bh_route_choice.h"
#include "../../World/Routes/bh_route_guided.h"
#include "../../Math/bh_fp.h"
#include <stddef.h>
#include <string.h>
#include <float.h>

static_assert(offsetof(BHEntity, payload) == 0xe4 && sizeof(void *) == 4, "route actor links");
static_assert(offsetof(BHEntity, world) == 0xc4, "BHEntity.world offset");
unsigned short bh_actor_route_now(const BHActorRouteContext *ctx)
{
    return (unsigned short)*ctx->live_tick;
}
void bh_actor_route_set_tick(BHActorRouteContext *ctx, unsigned short tick)
{
    *ctx->live_tick = (*ctx->live_tick & 0xffff0000UL) | tick;
}
int bh_actor_route_read(const BHEntity *entity, BHActorRouteState *state)
{
    if (!entity || !state)
        return 0;
    state->current = entity->actor.current;
    state->next = entity->actor.next;
    state->controls[0] = entity->actor.route_start;
    state->controls[1] = entity->actor.route_first;
    state->controls[2] = entity->actor.route_second;
    state->controls[3] = entity->actor.route_end;
    state->portal_center = entity->actor.portal_center;
    state->phase = entity->actor.phase;
    state->duration = entity->actor.duration;
    state->speed = entity->actor.speed;
    state->u = entity->actor.u;
    state->v = entity->actor.v;
    state->speed_scale = entity->actor.speed_scale;
    state->jitter = entity->actor.jitter;
    state->sound = entity->actor.sound;
    state->sound_frames = entity->actor.sound_frames;
    state->random = entity->actor.random;
    state->tick = entity->actor.tick;
    state->duration_ticks = entity->actor.duration_ticks;
    return 1;
}
int bh_actor_route_write(BHEntity *entity, const BHActorRouteState *state)
{
    if (!entity || !state)
        return 0;
    entity->actor.current = state->current;
    entity->actor.next = state->next;
    entity->actor.route_start = state->controls[0];
    entity->actor.route_first = state->controls[1];
    entity->actor.route_second = state->controls[2];
    entity->actor.route_end = state->controls[3];
    entity->actor.portal_center = state->portal_center;
    entity->actor.phase = state->phase;
    entity->actor.duration = state->duration;
    entity->actor.speed = state->speed;
    entity->actor.u = state->u;
    entity->actor.v = state->v;
    entity->actor.speed_scale = state->speed_scale;
    entity->actor.jitter = state->jitter;
    entity->actor.sound = state->sound;
    entity->actor.sound_frames = state->sound_frames;
    entity->actor.random = state->random;
    entity->actor.tick = state->tick;
    entity->actor.duration_ticks = state->duration_ticks;
    return 1;
}
static int fail(BHActorRouteContext *ctx, BHActorRouteError e)
{
    if (ctx)
        ctx->error = e;
    return 0;
}
BHPickupRoom *bh_actor_route_room(BHActorRouteContext *ctx, const BHWorldGeometry *world)
{
    unsigned long i;
    long found = ctx->room_index ? bh_binding_find(ctx->room_index, world, NULL) : -1;
    if (found >= 0 && (unsigned long)found < ctx->room_count && ctx->rooms[found].world == world)
        return ctx->rooms + found;
    for (i = 0; i < ctx->room_count; ++i)
        if (ctx->rooms[i].world == world)
            return ctx->rooms + i;
    return NULL;
}
static int vector_finite(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int portal_valid(const BHPortal *p)
{
    unsigned long i;
    if (p->direction > 6 || p->direction == 3)
        return 0;
    for (i = 0; i < 4; ++i)
        if (!_finite(p->scratch[i]))
            return 0;
    return p->scratch[0] <= p->scratch[1] && p->scratch[2] <= p->scratch[3];
}
static int duration_ticks(double duration, unsigned short *out)
{
    double ticks = duration * 30.0;
    if (!_finite(ticks) || ticks < -9223372036854775808.0 || ticks >= 9223372036854775808.0)
        return 0;
    *out = (unsigned short)(__int64)ticks;
    return 1;
}
#include "bh_actor_route_promote_core.inc"
int bh_actor_route_promote(BHActorRouteContext *ctx, BHEntity *entity, long incoming, long outgoing)
{
    BHActorRouteState state;
    BHPickupRoom *old, *current;
    BHWorldGeometry *world;
    const BHPortal *in, *out;
    BHEntity *next, *previous, *at;
    unsigned long n;
    BHActorRoutePromotionBinding binding;
    BHEntity saved;
    BHEntity *old_head, *new_head, *new_head_previous;
    if (!ctx || !entity || !ctx->live_tick || !ctx->rooms || !ctx->room_count || !ctx->actor_limit)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(entity, &state);
    old = bh_actor_route_room(ctx, state.current);
    current = bh_actor_route_room(ctx, state.next);
    world = state.next;
    if (!old || !current || !world->model || world->model->portal_count > 6 ||
        world->navigation.count != world->model->portal_count || incoming < 0 || outgoing < 0 ||
        (unsigned long)incoming >= world->navigation.count || (unsigned long)outgoing >= world->navigation.count)
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    in = world->navigation.portals + incoming;
    out = world->navigation.portals + outgoing;
    if (!out->neighbor || !bh_actor_route_room(ctx, (BHWorldGeometry *)out->neighbor) || !portal_valid(in) ||
        !portal_valid(out))
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    next = entity->actor.room_next;
    previous = entity->actor.room_previous;
    if ((previous ? previous->actor.room_next != entity : old->actors != entity) ||
        (next && next->actor.room_previous != entity))
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    at = old->actors;
    n = 0;
    while (at && at != entity && n < ctx->actor_limit)
    {
        at = at->actor.room_next;
        ++n;
    }
    if (at != entity || n >= ctx->actor_limit)
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    if (!vector_finite(&state.controls[3]) || !vector_finite(&world->minimum) || !vector_finite(&world->maximum) ||
        !_finite(state.u) || !_finite(state.v) || !_finite(state.speed) || state.speed <= 0 ||
        !_finite(state.speed_scale) || state.speed_scale <= 0)
        return fail(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    /* Invalid numerical input remains transactional in this checked adapter.
       The complete valid core follows the original early list/ownership stores. */
    saved = *entity;
    old_head = old->actors;
    new_head = current->actors;
    new_head_previous = new_head ? new_head->actor.room_previous : NULL;
    binding.context = ctx;
    binding.old_room = old;
    binding.room = current;
    binding.world = world;
    binding.incoming = in;
    binding.outgoing = out;
    if (!bh_actor_route_promote_core(entity, &binding, incoming, outgoing))
    {
        *entity = saved;
        old->actors = old_head;
        current->actors = new_head;
        if (previous)
            previous->actor.room_next = entity;
        if (next)
            next->actor.room_previous = entity;
        if (new_head)
            new_head->actor.room_previous = new_head_previous;
        return fail(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    }
    ctx->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
static __inline unsigned long random_next(unsigned long state)
{
    return ((state * 0xe3570000UL) | ((state * 0xe357UL) >> 16)) ^ state;
}
#include "bh_actor_route_advance_core.inc"

int bh_actor_route_advance(BHActorRouteContext *ctx, BHEntity *entity)
{
    BHActorRouteState state;
    BHActorRouteAdvanceBinding binding;
    BHActorExitRoomImage image;
    BHActorExitNeighborImage neighbors[6];
    const BHWorldGeometry *room, *neighbor;
    unsigned long i, j, count;
    long incoming = -1;
    if (!ctx || !entity || !ctx->live_tick || !ctx->rooms || !ctx->room_count)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(ctx, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(entity, &state);
    if (state.sound_frames == 1 && state.sound != BH_SOUND_INVALID && !ctx->sound)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    room = state.next;
    if (!room || !room->model || room->model->portal_count > 6 || room->navigation.count != room->model->portal_count)
    {
        route_advance_expire_sound(ctx, entity);
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    count = room->navigation.count;
    /* The guided owner has no valid incoming portal when count is zero. */
    if (!count && (room->query_flags & 8))
    {
        route_advance_expire_sound(ctx, entity);
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    for (i = 0; i < count; ++i)
        if ((const BHWorldGeometry *)room->navigation.portals[i].neighbor == state.current || incoming < 0)
            incoming = (long)i;
    if ((room->query_flags & 8) &&
        (room->navigation.portals[incoming].direction > 6 || room->navigation.portals[incoming].direction == 3))
    {
        route_advance_expire_sound(ctx, entity);
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    image.count_record = (int)count;
    image.flags = room->query_flags;
    binding.context = ctx;
    binding.room = &image;
    binding.current_identity = state.current ? (void *)&image : NULL;
    for (i = 0; i < count; ++i)
    {
        image.portals[i] = room->navigation.portals[i];
        neighbor = (const BHWorldGeometry *)image.portals[i].neighbor;
        if (neighbor)
        {
            for (j = 0; j < i; ++j)
                if (room->navigation.portals[j].neighbor == room->navigation.portals[i].neighbor)
                    break;
            if (j == i)
            {
                neighbors[i].flags = neighbor->query_flags;
                image.portals[i].neighbor = (BHPortalNode *)(neighbors + i);
            }
            else
                image.portals[i].neighbor = image.portals[j].neighbor;
            if (neighbor == state.current)
                binding.current_identity = image.portals[i].neighbor;
        }
    }
    return bh_actor_route_advance_core(entity, &binding);
}
