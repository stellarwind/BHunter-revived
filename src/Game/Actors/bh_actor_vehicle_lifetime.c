#include "bh_actor_vehicle_lifetime.h"
#include <string.h>
static BHPickupRoom *vehicle_release_room(BHActorVehicleLifetimeContext *ctx, BHEntity *actor)
{
    BHWorldGeometry *current;
    unsigned long i;
    memcpy(&current, (char *)&actor->actor.current, 4);
    return bh_actor_route_room(ctx->route, current);
}
static BHPickupRoom *vehicle_release_notice_room(BHActorVehicleLifetimeContext *ctx, BHEntity *actor, void *packet)
{
    bh_entities_query(ctx->pool, 0, 0, 0, 0, packet);
    return vehicle_release_room(ctx, actor);
}
#include "bh_actor_vehicle_release_core.inc"
int bh_actor_vehicle_release(BHActorVehicleLifetimeContext *ctx, BHEntity *entity)
{
    BHPickupRoom *room = 0;
    BHEntity *previous, *next;
    if (!ctx || !ctx->pool || !ctx->route || !ctx->route->sound || !entity || !ctx->count)
        return 0;
    if (!bh_entity_is_active(ctx->pool, entity))
        return 0;
    room = vehicle_release_room(ctx, entity);
    if (!room)
        return 0;
    previous = entity->actor.room_previous;
    next = entity->actor.room_next;
    if ((previous ? previous->actor.room_next : room->actors) != entity ||
        (next && next->actor.room_previous != entity))
        return 0;
    previous = entity->actor.global_previous;
    next = entity->actor.global_next;
    if ((previous ? previous->actor.global_next : ctx->head) != entity ||
        (next && next->actor.global_previous != entity))
        return 0;
    return bh_actor_vehicle_release_core(entity, ctx);
}
