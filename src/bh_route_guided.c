#include "bh_route_guided.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>
int __cdecl bh_route_guided_exit(const BHEntity *actor, const BHWorldGeometry *room, long incoming, long *selected)
{
    static const BHVec3 up = {0, 1, 0};
    BHVec3 anchor, normal, side, point, stored;
    const BHWorldGeometry *neighbor;
    unsigned long count, i, flags;
    long kind, choice = incoming;
    volatile float best = 9.0e37f;
    double x, y, z, vertical, lateral, score;
    float stored_vertical;
    if (!actor || !room || !room->model || !selected)
        return 0;
    count = room->model->portal_count;
    if (count > 6 || room->navigation.count != count || incoming < 0 || (unsigned long)incoming >= count)
        return 0;
    if (!bh_portal_center(room->navigation.portals + incoming, &anchor) ||
        !bh_portal_normal(room->navigation.portals + incoming, &normal))
        return 0;
    bh_vec3_cross(&up, &normal, &side);
    memcpy(&kind, actor->payload + 24, 4);
    for (i = 0; i < count; ++i)
    {
        if ((long)i == incoming)
            continue;
        neighbor = (const BHWorldGeometry *)room->navigation.portals[i].neighbor;
        if (!neighbor)
            continue;
        flags = neighbor->query_flags;
        if ((flags & 0x10) && kind > 0)
            continue;
        if ((flags & 0x20) && (kind == 0 || kind == 6))
            continue;
        if (room->navigation.portals[i].direction != (room->navigation.portals[incoming].direction ^ 4UL))
            continue;
        if (!bh_portal_center(room->navigation.portals + i, &point))
            return 0;
        x = (double)point.x - anchor.x;
        y = (double)point.y - anchor.y;
        z = (double)point.z - anchor.z;
        bh_round_float(&stored.x, x);
        bh_round_float(&stored.y, y);
        bh_round_float(&stored.z, z);
        vertical = (x * up.x + y * up.y) + (double)stored.z * up.z;
        lateral = ((double)stored.x * side.x + (double)stored.y * side.y) + (double)stored.z * side.z;
        bh_round_float(&stored_vertical, vertical);
        score = vertical * stored_vertical + lateral * lateral;
        if (_isnan(score) || _isnan((double)best) || score < best)
        {
            best = (float)score;
            choice = (long)i;
        }
    }
    *selected = choice;
    return 1;
}
