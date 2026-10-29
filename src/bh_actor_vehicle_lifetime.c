#include "bh_actor_vehicle_lifetime.h"
#include <string.h>
static __inline BHEntity *link_at(BHEntity *e, unsigned long offset)
{
    BHEntity *p;
    memcpy(&p, (char *)e + offset, 4);
    return p;
}
static __inline void set_link(BHEntity *e, unsigned long offset, BHEntity *p)
{
    memcpy((char *)e + offset, &p, 4);
}
static BHPickupRoom *vehicle_release_room(BHActorVehicleLifetimeContext *c, BHEntity *actor)
{
    BHWorldGeometry *current;
    unsigned long i;
    memcpy(&current, (char *)actor + 0x1a4, 4);
    for (i = 0; i < c->route->room_count; ++i)
        if (c->route->rooms[i].world == current)
            return c->route->rooms + i;
    return NULL;
}
static BHPickupRoom *vehicle_release_notice_room(BHActorVehicleLifetimeContext *c, BHEntity *actor, void *packet)
{
    bh_entities_query(c->pool, 0, 0, 0, 0, packet);
    return vehicle_release_room(c, actor);
}
#include "bh_actor_vehicle_release_core.inc"
int __cdecl bh_actor_vehicle_release(BHActorVehicleLifetimeContext *c, BHEntity *e)
{
    BHPickupRoom *room = 0;
    BHEntity *previous, *next;
    if (!c || !c->pool || !c->route || !c->route->sound || !e || !c->count)
        return 0;
    if (!bh_entity_is_active(c->pool, e))
        return 0;
    room = vehicle_release_room(c, e);
    if (!room)
        return 0;
    previous = link_at(e, 0xf8);
    next = link_at(e, 0xf4);
    if ((previous ? link_at(previous, 0xf4) : room->actors) != e || (next && link_at(next, 0xf8) != e))
        return 0;
    previous = link_at(e, 0xec);
    next = link_at(e, 0xf0);
    if ((previous ? link_at(previous, 0xf0) : c->head) != e || (next && link_at(next, 0xec) != e))
        return 0;
    return bh_actor_vehicle_release_core(e, c);
}
