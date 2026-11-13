#include <stddef.h>

typedef struct BHActorRouteReleaseBinding
{
    BHSoundQueue *sound;
    BHEntityPool *pool;
    BHEntity **room_head;
} BHActorRouteReleaseBinding;
static_assert(offsetof(BHActorRouteReleaseBinding, pool) == 4, "BHActorRouteReleaseBinding.pool offset");
static_assert(offsetof(BHActorRouteReleaseBinding, room_head) == 8, "BHActorRouteReleaseBinding.room_head offset");

/* Complete 00435720. Source bindings name the real queue, pool and room head;
   the original payload/list stores remain on the real entity. No retained
   payload, sound handle or actor route field is cleared during retirement. */
int bh_actor_route_release_core(BHEntity *actor, const BHActorRouteReleaseBinding *binding)
{
    BHEntity *previous, *next;
    if (actor->actor.sound != BH_SOUND_INVALID)
        bh_sound_stop(binding->sound, actor->actor.sound);
    previous = actor->actor.room_previous;
    if (previous)
        previous->actor.room_next = actor->actor.room_next;
    else
        *binding->room_head = actor->actor.room_next;
    next = actor->actor.room_next;
    if (next)
        next->actor.room_previous = actor->actor.room_previous;
    return bh_entity_release(binding->pool, actor);
}
