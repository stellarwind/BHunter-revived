#include "bh_actor_route_lifetime.h"
#include <string.h>
static int fail(BHActorRouteContext *ctx, BHActorRouteError e)
{
    if (ctx)
        ctx->error = e;
    return 0;
}
#include "bh_actor_route_release_core.inc"
#include "bh_actor_route_due_core.inc"
int bh_actor_route_due(BHActorRouteContext *ctx, BHEntity *entity)
{
    const unsigned short *clock;
    if (!ctx || !entity || !ctx->live_tick)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    clock = (const unsigned short *)ctx->live_tick;
    if (!bh_actor_route_due_core(entity, ctx, clock))
        return 0;
    ctx->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
int bh_actor_route_release(BHActorRouteContext *ctx, BHEntityPool *pool, BHEntity *entity)
{
    BHActorRouteState state;
    BHPickupRoom *room = NULL;
    BHEntity *next, *previous, *at;
    BHActorRouteReleaseBinding binding;
    unsigned long i, n;
    if (!ctx || !pool || !entity || !ctx->rooms || !ctx->room_count || !ctx->actor_limit)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_entity_is_active(pool, entity))
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    bh_actor_route_read(entity, &state);
    room = bh_actor_route_room(ctx, state.current);
    if (!room)
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    next = entity->actor.room_next;
    previous = entity->actor.room_previous;
    if ((previous ? previous->actor.room_next != entity : room->actors != entity) ||
        (next && next->actor.room_previous != entity))
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    at = room->actors;
    n = 0;
    while (at && at != entity && n < ctx->actor_limit)
    {
        at = at->actor.room_next;
        ++n;
    }
    if (at != entity || n >= ctx->actor_limit)
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    if (state.sound != BH_SOUND_INVALID && !ctx->sound)
        return fail(ctx, BH_ACTOR_ROUTE_ARGUMENT);
    binding.sound = ctx->sound;
    binding.pool = pool;
    binding.room_head = &room->actors;
    if (!bh_actor_route_release_core(entity, &binding))
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    ctx->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
