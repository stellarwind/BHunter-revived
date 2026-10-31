#include "bh_actor_exit_image.h"
#include <stddef.h>

typedef char exit_model_stride[sizeof(BHActorExitModelRecord) == 0x174 ? 1 : -1];
typedef char exit_count_offset[offsetof(BHActorExitModelRecord, portal_count) == 0x18 ? 1 : -1];
typedef char exit_room_descriptor[offsetof(BHActorExitRoomImage, count_record) == 0xf8 ? 1 : -1];
typedef char exit_room_portals[offsetof(BHActorExitRoomImage, portals) == 0x1cc ? 1 : -1];
typedef char exit_portal_neighbor[offsetof(BHPortal, neighbor) == 0x38 ? 1 : -1];
typedef char exit_neighbor_flags[offsetof(BHActorExitNeighborImage, flags) == 0x114 ? 1 : -1];

#define EXIT_INT(p, o) (*(int *)((unsigned char *)(p) + (o)))
#define EXIT_UINT(p, o) (*(unsigned int *)((unsigned char *)(p) + (o)))
#define EXIT_POINTER(p, o) (*(void **)((unsigned char *)(p) + (o)))

int __cdecl bh_actor_choose_exit_core(void *actor, void *room_image, void *preferred_image, int excluded)
{
    int eligible = 0, allowed[10], i, count;
    const BHActorExitModelRecord *model = bh_actor_exit_counts + EXIT_INT(room_image, 0xf8);
    if (preferred_image)
    {
        for (i = 0; i < model->portal_count; ++i)
        {
            void *neighbor = EXIT_POINTER(room_image, 0x204 + i * 60);
            if (neighbor == preferred_image && neighbor &&
                (!(EXIT_UINT(neighbor, 0x114) & 0x10) || EXIT_INT(actor, 0xfc) <= 0))
            {
                if (!(EXIT_UINT(neighbor, 0x114) & 0x20))
                    return i;
                if (EXIT_INT(actor, 0xfc) != 0 && EXIT_INT(actor, 0xfc) != 6)
                    return i;
            }
        }
    }
    count = model->portal_count;
    for (i = 0; i < count; ++i)
    {
        void *neighbor = EXIT_POINTER(room_image, 0x204 + i * 60);
        if (!neighbor || ((EXIT_UINT(neighbor, 0x114) & 0x10) && EXIT_INT(actor, 0xfc) > 0) ||
            ((EXIT_UINT(neighbor, 0x114) & 0x20) && (EXIT_INT(actor, 0xfc) == 0 || EXIT_INT(actor, 0xfc) == 6)) ||
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
        EXIT_UINT(actor, 0x1e8) =
            ((EXIT_UINT(actor, 0x1e8) * -0x1ca90000) | ((EXIT_UINT(actor, 0x1e8) * 0xe357) >> 16)) ^
            EXIT_UINT(actor, 0x1e8);
        state = EXIT_UINT(actor, 0x1e8);
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

#undef EXIT_INT
#undef EXIT_UINT
#undef EXIT_POINTER
