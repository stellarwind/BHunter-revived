#include "bh_actor_exit_image.h"

typedef struct BHActorRouteAdvanceBinding
{
    BHActorRouteContext *context;
    BHActorExitRoomImage *room;
    void *current_identity;
} BHActorRouteAdvanceBinding;
typedef char advance_binding_room[offsetof(BHActorRouteAdvanceBinding, room) == 4 ? 1 : -1];
typedef char advance_binding_current[offsetof(BHActorRouteAdvanceBinding, current_identity) == 8 ? 1 : -1];
typedef char advance_context_sound[offsetof(BHActorRouteContext, sound) == 12 ? 1 : -1];
typedef char advance_room_count[offsetof(BHActorExitRoomImage, count_record) == 0xf8 ? 1 : -1];
typedef char advance_room_portals[offsetof(BHActorExitRoomImage, portals) == 0x1cc ? 1 : -1];

int __cdecl bh_actor_guided_exit_core(const BHEntity *, void *, int);

#define ADVANCE_UINT(a, o) (*(unsigned long *)((char *)(a) + (o)))
#define ADVANCE_FLOAT(a, o) (*(float *)((char *)(a) + (o)))

/* Also used on source-only validation failures, preserving the old adapter's
   sound expiration before its room/selector rejection. No fabricated voice. */
static __inline void route_advance_expire_sound(BHActorRouteContext *context, BHEntity *actor)
{
    if (ADVANCE_UINT(actor, 0x108))
    {
        --ADVANCE_UINT(actor, 0x108);
        if (!ADVANCE_UINT(actor, 0x108) && ADVANCE_UINT(actor, 0x104) != BH_SOUND_INVALID)
        {
            bh_sound_stop(context->sound, ADVANCE_UINT(actor, 0x104));
            ADVANCE_UINT(actor, 0x104) = BH_SOUND_INVALID;
        }
    }
}

/* Complete 004355c0 behavior. The explicit binding replaces only the original
   room/model/global-context representation; actor storage remains the real
   BHEntity. It supplies no replacement RNG or source-owned game subsystem. */
int __cdecl bh_actor_route_advance_core(BHEntity *actor, const BHActorRouteAdvanceBinding *binding)
{
    BHActorExitRoomImage *room;
    const BHActorExitModelRecord *model;
    int incoming = -1, outgoing, i;
    unsigned long first, second;
    double factor;
    route_advance_expire_sound(binding->context, actor);
    room = binding->room;
    model = bh_actor_exit_counts + room->count_record;
    for (i = 0; i < model->portal_count; ++i)
        if (room->portals[i].neighbor == binding->current_identity || incoming < 0)
            incoming = i;
    if (ADVANCE_UINT(room, 0x114) & 8)
        outgoing = bh_actor_guided_exit_core(actor, room, incoming);
    else
        outgoing = bh_actor_choose_exit_core(actor, room, NULL, incoming);
    first = random_next(ADVANCE_UINT(actor, 0x1e8));
    second = random_next(first);
    factor = 1.0 - (double)ADVANCE_FLOAT(actor, 0x1b0);
    *(volatile float *)((char *)actor + 0x18c) = ADVANCE_FLOAT(actor, 0x18c) * factor -
        ((double)(first & 0x7fff) * ADVANCE_FLOAT(actor, 0x1b0)) * (-1.0 / 32768.0);
    *(volatile unsigned long *)((char *)actor + 0x1e8) = second;
    *(volatile float *)((char *)actor + 0x190) = ADVANCE_FLOAT(actor, 0x190) * factor -
        ((double)(second & 0x7fff) * ADVANCE_FLOAT(actor, 0x1b0)) * (-1.0 / 32768.0);
    return bh_actor_route_promote(binding->context, actor, incoming, outgoing);
}

#undef ADVANCE_UINT
#undef ADVANCE_FLOAT
