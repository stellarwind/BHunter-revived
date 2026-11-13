#include "bh_actor_exit_image.h"

typedef struct BHActorRouteAdvanceBinding
{
    BHActorRouteContext *context;
    BHActorExitRoomImage *room;
    void *current_identity;
} BHActorRouteAdvanceBinding;
static_assert(offsetof(BHActorRouteAdvanceBinding, room) == 4, "BHActorRouteAdvanceBinding.room offset");
static_assert(offsetof(BHActorRouteAdvanceBinding, current_identity) == 8,
              "BHActorRouteAdvanceBinding.current_identity offset");
static_assert(offsetof(BHActorRouteContext, sound) == 12, "BHActorRouteContext.sound offset");
static_assert(offsetof(BHActorExitRoomImage, count_record) == 0xf8, "BHActorExitRoomImage.count_record offset");
static_assert(offsetof(BHActorExitRoomImage, portals) == 0x1cc, "BHActorExitRoomImage.portals offset");

int bh_actor_guided_exit_core(const BHEntity *, void *, int);

/* Also used on source-only validation failures, preserving the old adapter's
   sound expiration before its room/selector rejection. No fabricated voice. */
static __inline void route_advance_expire_sound(BHActorRouteContext *context, BHEntity *actor)
{
    if (actor->actor.sound_frames)
    {
        --actor->actor.sound_frames;
        if (!actor->actor.sound_frames && actor->actor.sound != BH_SOUND_INVALID)
        {
            bh_sound_stop(context->sound, actor->actor.sound);
            actor->actor.sound = BH_SOUND_INVALID;
        }
    }
}

/* Complete 004355c0 behavior. The explicit binding replaces only the original
   room/model/global-context representation; actor storage remains the real
   BHEntity. It supplies no replacement RNG or source-owned game subsystem. */
int bh_actor_route_advance_core(BHEntity *actor, const BHActorRouteAdvanceBinding *binding)
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
    if (room->flags & 8)
        outgoing = bh_actor_guided_exit_core(actor, room, incoming);
    else
        outgoing = bh_actor_choose_exit_core(actor, room, NULL, incoming);
    first = random_next(actor->actor.random);
    second = random_next(first);
    factor = 1.0 - (double)actor->actor.jitter;
    *(volatile float *)&actor->actor.u =
        actor->actor.u * factor - ((double)(first & 0x7fff) * actor->actor.jitter) * (-1.0 / 32768.0);
    *(volatile unsigned long *)&actor->actor.random = second;
    *(volatile float *)&actor->actor.v =
        actor->actor.v * factor - ((double)(second & 0x7fff) * actor->actor.jitter) * (-1.0 / 32768.0);
    return bh_actor_route_promote(binding->context, actor, incoming, outgoing);
}
