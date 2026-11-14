#include "bh_route_query.h"
#include "../../Game/Actors/bh_actor_portal_core.inc"
#include "../../Game/Actors/bh_actor_portal_center_core.inc"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <math.h>
#include <float.h>

static int contains(const BHWorldGeometry *room, const float *lower, const float *upper)
{
    if (room->owner)
        return 0;
    return room->minimum.x <= upper[0] && lower[0] <= room->maximum.x && room->minimum.y <= upper[1] &&
           lower[1] <= room->maximum.y && room->minimum.z <= upper[2] && lower[2] <= room->maximum.z;
}
BHWorldGeometry *bh_world_locate(BHWorldGeometry *first, unsigned long count, BHWorldGeometry *hint,
                                 const BHVec3 *point, float margin)
{
    BHWorldGeometry *room, *start;
    unsigned long i;
    float lower[3], upper[3];
    if (!first || !count || count > 65535 || !point || !_finite(point->x) || !_finite(point->y) || !_finite(point->z) ||
        !_finite(margin))
        return NULL;
    if (margin == 0)
    {
        memcpy(lower, point, sizeof(lower));
        memcpy(upper, point, sizeof(upper));
    }
    else
        /* 0040aa68..0040aa7c stores the expanded bounds before either search. */
        for (i = 0; i < 3; ++i)
        {
            bh_round_float(lower + i, (double)((const float *)point)[i] - margin);
            bh_round_float(upper + i, (double)((const float *)point)[i] + margin);
        }
    /* 0040a8c0 trusts the scene's live links. Scene owners validate topology
       when binding it; repeating whole-ring and membership checks here makes
       every projectile/route lookup proportional to the entire level. */
    if (hint)
        for (i = 0; i < 6; ++i)
            if (hint->navigation.portals[i].neighbor)
            {
                room = (BHWorldGeometry *)hint->navigation.portals[i].neighbor;
                if (contains(room, lower, upper))
                    return room;
            }
    start = hint ? hint : first;
    room = start;
    for (i = 0; room && i < count; ++i)
    {
        if (contains(room, lower, upper))
            return room;
        room = (BHWorldGeometry *)room->navigation.next;
        if (room == start)
            break;
    }
    return NULL;
}
int bh_portal_center(const BHPortal *portal, BHVec3 *out)
{
    BHActorPortalImage image;
    if (!portal || !out || portal->direction > 6 || portal->direction == 3)
        return 0;
    memcpy(image.portals, portal, sizeof(*portal));
    bh_actor_portal_center_core(&image, 0, (float *)out);
    return 1;
}
int bh_portal_normal(const BHPortal *portal, BHVec3 *out)
{
    BHActorPortalImage image;
    if (!portal || !out || portal->direction > 6 || portal->direction == 3)
        return 0;
    memcpy(image.portals, portal, sizeof(*portal));
    bh_actor_portal_normal_core(&image, 0, (float *)out);
    return 1;
}
static double squared(double x, double y, double z)
{
    float rx, ry, rz;
    bh_round_float(&rx, x);
    bh_round_float(&ry, y);
    bh_round_float(&rz, z);
    return (x * rx + y * ry) + z * rz;
}
int bh_route_plan(const BHNavigation *n, BHWorldGeometry *first, unsigned long count, BHWorldGeometry *current,
                  const BHVec3 *lookup_goal, const BHVec3 *projection_goal, BHRoutePlan *out)
{
    BHWorldGeometry *destination;
    BHRoutePlan next;
    long source_index, destination_index, slot;
    BHVec3 normal;
    double dx, dot, length_a, length_b;
    float dy, dz, rounded_dot;
    if (!n || !current || !lookup_goal || !projection_goal || !out || !n->routes.cells ||
        n->routes.node_count != n->junction_count || !bh_math_environment_supported())
        return 0;
    destination = bh_world_locate(first, count, NULL, lookup_goal, 0);
    source_index = bh_navigation_find(n, current);
    destination_index = bh_navigation_find(n, destination);
    if (source_index < 0 || destination_index < 0 || (unsigned long)source_index >= n->junction_count ||
        (unsigned long)destination_index >= n->junction_count)
        return 0;
    slot = n->routes.cells[destination_index * n->junction_count + source_index].exit_slot;
    if (slot < 0 || slot >= 6 || !current->navigation.portals[slot].neighbor)
        return 0;
    memset(&next, 0, sizeof(next));
    next.next = (BHWorldGeometry *)current->navigation.portals[slot].neighbor;
    next.exit_slot = slot;
    next.goal = *projection_goal;
    if (!bh_portal_center(current->navigation.portals + slot, &next.portal_center) ||
        !bh_portal_normal(current->navigation.portals + slot, &normal))
        return 0;
    dx = (double)next.goal.x - next.portal_center.x;
    bh_round_float(&dy, (double)next.goal.y - next.portal_center.y);
    bh_round_float(&dz, (double)next.goal.z - next.portal_center.z);
    dot = (normal.x * dx + normal.y * dy) + normal.z * dz;
    bh_round_float(&rounded_dot, dot);
    /* Original x projection uses unrounded dot; y/z reload its float store. */
    bh_round_float(&next.projected.x, dot * normal.x + next.portal_center.x);
    bh_round_float(&next.projected.y, (double)rounded_dot * normal.y + next.portal_center.y);
    bh_round_float(&next.projected.z, (double)rounded_dot * normal.z + next.portal_center.z);
    next.anchor = next.portal_center;
    length_a = sqrt(squared(dx, dy, dz));
    length_b = sqrt(squared((double)next.anchor.x - next.projected.x, (double)next.anchor.y - next.projected.y,
                            (double)next.anchor.z - next.projected.z));
    bh_round_float(&next.parameter_rate, 4.666666666666667 / (length_a + length_b));
    *out = next;
    return 1;
}
