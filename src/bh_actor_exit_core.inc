#include "bh_actor_exit_image.h"
#include "bh_entities.h"
#include <stddef.h>

static_assert(sizeof(BHActorExitModelRecord) == 0x174, "BHActorExitModelRecord size");
static_assert(offsetof(BHActorExitModelRecord, portal_count) == 0x18, "BHActorExitModelRecord.portal_count offset");
static_assert(offsetof(BHActorExitRoomImage, count_record) == 0xf8, "BHActorExitRoomImage.count_record offset");
static_assert(offsetof(BHActorExitRoomImage, flags) == 0x114, "BHActorExitRoomImage.flags offset");
static_assert(sizeof(BHActorExitRoomImage) == 0x334, "BHActorExitRoomImage size");
static_assert(offsetof(BHActorExitRoomImage, portals) == 0x1cc, "BHActorExitRoomImage.portals offset");
static_assert(offsetof(BHPortal, neighbor) == 0x38, "BHPortal.neighbor offset");
static_assert(offsetof(BHActorExitNeighborImage, flags) == 0x114, "BHActorExitNeighborImage.flags offset");

int bh_actor_choose_exit_core(void *actor, void *room_image, void *preferred_image, int excluded)
{
    BHEntity *entity = (BHEntity *)actor;
    BHActorExitRoomImage *room = (BHActorExitRoomImage *)room_image;
    int eligible = 0, allowed[10], i, count;
    const BHActorExitModelRecord *model = bh_actor_exit_counts + room->count_record;
    if (preferred_image)
    {
        for (i = 0; i < model->portal_count; ++i)
        {
            void *neighbor = room->portals[i].neighbor;
            if (neighbor == preferred_image && neighbor &&
                (!(((BHActorExitNeighborImage *)neighbor)->flags & 0x10) || (*(int *)&entity->actor.kind) <= 0))
            {
                if (!(((BHActorExitNeighborImage *)neighbor)->flags & 0x20))
                    return i;
                if ((*(int *)&entity->actor.kind) != 0 && (*(int *)&entity->actor.kind) != 6)
                    return i;
            }
        }
    }
    count = model->portal_count;
    for (i = 0; i < count; ++i)
    {
        void *neighbor = room->portals[i].neighbor;
        if (!neighbor ||
            ((((BHActorExitNeighborImage *)neighbor)->flags & 0x10) && (*(int *)&entity->actor.kind) > 0) ||
            ((((BHActorExitNeighborImage *)neighbor)->flags & 0x20) &&
             ((*(int *)&entity->actor.kind) == 0 || (*(int *)&entity->actor.kind) == 6)) ||
            i == excluded)
            allowed[i] = 0;
        else
        {
            allowed[i] = 1;
            ++eligible;
        }
    }
    if (eligible)
    {
        unsigned int state, rank;
        entity->actor.random =
            ((entity->actor.random * -0x1ca90000) | ((entity->actor.random * 0xe357) >> 16)) ^ entity->actor.random;
        state = entity->actor.random;
        rank = ((state & 0x7fff) * eligible) >> 15;
        i = -1;
        do
        {
            ++i;
            while (!allowed[i])
                ++i;
        } while (rank--);
        return i;
    }
    return excluded;
}

const BHActorExitModelRecord bh_actor_exit_counts[7] = {{{0}, 0, {0}}, {{0}, 1, {0}}, {{0}, 2, {0}}, {{0}, 3, {0}},
                                                        {{0}, 4, {0}}, {{0}, 5, {0}}, {{0}, 6, {0}}};
