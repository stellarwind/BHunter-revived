#include <stddef.h>
typedef char vehicle_release_head[offsetof(BHActorVehicleLifetimeContext, head) == 8 ? 1 : -1];
typedef char vehicle_release_count[offsetof(BHActorVehicleLifetimeContext, count) == 12 ? 1 : -1];

/* Whole 004397e0 through real source queue/query/pool owners. Notification
   binding resolves the publisher's room AFTER callbacks, as the original
   +1a4 read does; all room/global links and the count are then reloaded. */
int __cdecl bh_actor_vehicle_release_core(BHEntity *actor, BHActorVehicleLifetimeContext *context)
{
    BHPickupRoom *room;
    BHEntity *previous, *next;
    unsigned long sound;
    struct
    {
        long event;
        BHEntity *actor;
    } packet;
    memcpy(&sound, (char *)actor + 0x104, 4);
    if (sound != BH_SOUND_INVALID)
        bh_sound_stop(context->route->sound, sound);
    packet.event = 17;
    packet.actor = actor;
    room = vehicle_release_notice_room(context, actor, &packet);
    if (!room)
        return 0;
    previous = link_at(actor, 0xf8);
    if (previous)
        set_link(previous, 0xf4, link_at(actor, 0xf4));
    else
        room->actors = link_at(actor, 0xf4);
    next = link_at(actor, 0xf4);
    if (next)
        set_link(next, 0xf8, link_at(actor, 0xf8));
    previous = link_at(actor, 0xec);
    if (previous)
        set_link(previous, 0xf0, link_at(actor, 0xf0));
    else
        context->head = link_at(actor, 0xf0);
    next = link_at(actor, 0xf0);
    if (next)
        set_link(next, 0xec, link_at(actor, 0xec));
    --context->count;
    return bh_entity_release(context->pool, actor);
}
