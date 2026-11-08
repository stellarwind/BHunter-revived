#include "bh_actor_vehicle_lifetime.h"
#include <string.h>
static __inline BHEntity *link_at(BHEntity *entity, unsigned long offset)
{
    BHEntity *p;
    memcpy(&p, (char *)entity + offset, 4);
    return p;
}
static __inline void set_link(BHEntity *entity, unsigned long offset, BHEntity *p)
{
    memcpy((char *)entity + offset, &p, 4);
}
static BHPickupRoom *vehicle_release_room(BHActorVehicleLifetimeContext *ctx, BHEntity *actor)
{
    BHWorldGeometry *current;
    unsigned long i;
    memcpy(&current, (char *)actor + 0x1a4, 4);
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
    previous = link_at(entity, 0xf8);
    next = link_at(entity, 0xf4);
    if ((previous ? link_at(previous, 0xf4) : room->actors) != entity || (next && link_at(next, 0xf8) != entity))
        return 0;
    previous = link_at(entity, 0xec);
    next = link_at(entity, 0xf0);
    if ((previous ? link_at(previous, 0xf0) : ctx->head) != entity || (next && link_at(next, 0xec) != entity))
        return 0;
    return bh_actor_vehicle_release_core(entity, ctx);
}
