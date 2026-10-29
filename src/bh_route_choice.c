#include "bh_route_choice.h"
#include "bh_actor_exit_image.h"
#include "bh_actor_exit_core.inc"
#include <stddef.h>
#include <string.h>
typedef char route_actor_payload[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char route_word_size[sizeof(unsigned long) == 4 && sizeof(long) == 4 ? 1 : -1];

int __cdecl bh_route_choose_exit(BHEntity *actor, const BHWorldGeometry *room, const BHWorldGeometry *preferred,
                                 long excluded, long *selected)
{
    unsigned long count, i;
    const BHWorldGeometry *neighbor;
    BHActorExitRoomImage image;
    BHActorExitNeighborImage neighbors[6];
    void *preferred_image;
    if (!actor || !room || !room->model || !selected)
        return 0;
    count = room->model->portal_count;
    if (count > 6 || room->navigation.count != count)
        return 0;
    image.count_record = (int)count;
    preferred_image = preferred ? (void *)&image : NULL;
    for (i = 0; i < count; ++i)
    {
        neighbor = (const BHWorldGeometry *)room->navigation.portals[i].neighbor;
        image.portals[i] = room->navigation.portals[i];
        if (neighbor)
        {
            neighbors[i].flags = neighbor->query_flags;
            image.portals[i].neighbor = (BHPortalNode *)(neighbors + i);
            if (neighbor == preferred && preferred_image == &image)
                preferred_image = neighbors + i;
        }
    }
    *selected = bh_actor_choose_exit_core(actor, &image, preferred_image, (int)excluded);
    return 1;
}
