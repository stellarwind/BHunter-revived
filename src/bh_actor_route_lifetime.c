#include "bh_actor_route_lifetime.h"
#include <string.h>
static int fail(BHActorRouteContext *ctx, BHActorRouteError e)
{
    if (ctx)
        ctx->error = e;
    return 0;
}
static BHEntity *link_value(const BHEntity *entity, unsigned long offset)
{
    BHEntity *p;
    memcpy(&p, (const unsigned char *)entity + offset, 4);
    return p;
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
    next = link_value(entity, 0xf4);
    previous = link_value(entity, 0xf8);
    if ((previous ? link_value(previous, 0xf4) != entity : room->actors != entity) ||
        (next && link_value(next, 0xf8) != entity))
        return fail(ctx, BH_ACTOR_ROUTE_TOPOLOGY);
    at = room->actors;
    n = 0;
    while (at && at != entity && n < ctx->actor_limit)
    {
        at = link_value(at, 0xf4);
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
