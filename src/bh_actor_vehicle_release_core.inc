#include <stddef.h>
static_assert(offsetof(BHActorVehicleLifetimeContext, head) == 8, "BHActorVehicleLifetimeContext.head offset");
static_assert(offsetof(BHActorVehicleLifetimeContext, count) == 12, "BHActorVehicleLifetimeContext.count offset");

/* Whole 004397e0 through real source queue/query/pool owners. Notification
   binding resolves the publisher's room AFTER callbacks, as the original
   +1a4 read does; all room/global links and the count are then reloaded. */
int bh_actor_vehicle_release_core(BHEntity *actor, BHActorVehicleLifetimeContext *context)
{
    BHPickupRoom *room;
    BHEntity *previous, *next;
    unsigned long sound;
    struct
    {
        long event;
        BHEntity *actor;
    } packet;
    memcpy(&sound, (char *)&actor->actor.sound, 4);
    if (sound != BH_SOUND_INVALID)
        bh_sound_stop(context->route->sound, sound);
    packet.event = 17;
    packet.actor = actor;
    room = vehicle_release_notice_room(context, actor, &packet);
    if (!room)
        return 0;
    previous = actor->actor.room_previous;
    if (previous)
        (previous->actor.room_next = actor->actor.room_next);
    else
        room->actors = actor->actor.room_next;
    next = actor->actor.room_next;
    if (next)
        (next->actor.room_previous = actor->actor.room_previous);
    previous = actor->actor.global_previous;
    if (previous)
        (previous->actor.global_next = actor->actor.global_next);
    else
        context->head = actor->actor.global_next;
    next = actor->actor.global_next;
    if (next)
        (next->actor.global_previous = actor->actor.global_previous);
    --context->count;
    return bh_entity_release(context->pool, actor);
}
