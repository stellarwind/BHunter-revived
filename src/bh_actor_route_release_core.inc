#include <stddef.h>

typedef struct BHActorRouteReleaseBinding
{
    BHSoundQueue *sound;
    BHEntityPool *pool;
    BHEntity **room_head;
} BHActorRouteReleaseBinding;
typedef char release_binding_pool[offsetof(BHActorRouteReleaseBinding, pool) == 4 ? 1 : -1];
typedef char release_binding_head[offsetof(BHActorRouteReleaseBinding, room_head) == 8 ? 1 : -1];

#define RELEASE_LINK(a, o) (*(BHEntity **)((char *)(a) + (o)))
#define RELEASE_UINT(a, o) (*(unsigned long *)((char *)(a) + (o)))

/* Complete 00435720. Source bindings name the real queue, pool and room head;
   the original payload/list stores remain on the real entity. No retained
   payload, sound handle or actor route field is cleared during retirement. */
int __cdecl bh_actor_route_release_core(BHEntity *actor, const BHActorRouteReleaseBinding *binding)
{
    BHEntity *previous, *next;
    if (RELEASE_UINT(actor, 0x104) != BH_SOUND_INVALID)
        bh_sound_stop(binding->sound, RELEASE_UINT(actor, 0x104));
    previous = RELEASE_LINK(actor, 0xf8);
    if (previous)
        RELEASE_LINK(previous, 0xf4) = RELEASE_LINK(actor, 0xf4);
    else
        *binding->room_head = RELEASE_LINK(actor, 0xf4);
    next = RELEASE_LINK(actor, 0xf4);
    if (next)
        RELEASE_LINK(next, 0xf8) = RELEASE_LINK(actor, 0xf8);
    return bh_entity_release(binding->pool, actor);
}

#undef RELEASE_LINK
#undef RELEASE_UINT
