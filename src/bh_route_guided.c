#include "bh_route_guided.h"
#include "bh_actor_guided_exit_core.inc"

int __cdecl bh_route_guided_exit(const BHEntity *actor, const BHWorldGeometry *room, long incoming, long *selected)
{
    BHActorExitRoomImage image;
    BHActorExitNeighborImage neighbors[6];
    const BHWorldGeometry *neighbor;
    unsigned long i, count;
    if (!actor || !room || !room->model || !selected)
        return 0;
    count = room->model->portal_count;
    if (count > 6 || room->navigation.count != count || incoming < 0 || (unsigned long)incoming >= count ||
        room->navigation.portals[incoming].direction > 6 || room->navigation.portals[incoming].direction == 3)
        return 0;
    image.count_record = (int)count;
    for (i = 0; i < count; ++i)
    {
        image.portals[i] = room->navigation.portals[i];
        neighbor = (const BHWorldGeometry *)image.portals[i].neighbor;
        if (neighbor)
        {
            neighbors[i].flags = neighbor->query_flags;
            image.portals[i].neighbor = (BHPortalNode *)(neighbors + i);
        }
    }
    *selected = bh_actor_guided_exit_core(actor, &image, (int)incoming);
    return 1;
}
