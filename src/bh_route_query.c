#include "bh_route_query.h"
#include "bh_actor_portal_core.inc"
#include "bh_actor_portal_center_core.inc"
#include "bh_fp.h"
#include "bh_round.h"
#include <string.h>
#include <math.h>
#include <float.h>

static int member(BHWorldGeometry *first, unsigned long count, const BHWorldGeometry *target)
{
    BHWorldGeometry *p = first;
    unsigned long i;
    for (i = 0; i < count; ++i)
    {
        if (p == target)
            return 1;
        p = (BHWorldGeometry *)p->navigation.next;
    }
    return 0;
}
static int contains(const BHWorldGeometry *p, const BHVec3 *point, float margin)
{
    if (p->owner)
        return 0;
    return p->minimum.x <= (double)point->x + margin && (double)point->x - margin <= p->maximum.x &&
           p->minimum.y <= (double)point->y + margin && (double)point->y - margin <= p->maximum.y &&
           p->minimum.z <= (double)point->z + margin && (double)point->z - margin <= p->maximum.z;
}
BHWorldGeometry *__cdecl bh_world_locate(BHWorldGeometry *first, unsigned long count, BHWorldGeometry *hint,
                                         const BHVec3 *point, float margin)
{
    BHWorldGeometry *p, *start;
    unsigned long i;
    if (!first || !count || count > 65535 || !point || !_finite(point->x) || !_finite(point->y) || !_finite(point->z) ||
        !_finite(margin))
        return NULL;
    p = first;
    for (i = 0; i < count; ++i)
    {
        if (!p || (i && p == first))
            return NULL;
        p = (BHWorldGeometry *)p->navigation.next;
    }
    if (p != first || (hint && !member(first, count, hint)))
        return NULL;
    if (hint)
        for (i = 0; i < 6; ++i)
            if (hint->navigation.portals[i].neighbor)
            {
                p = (BHWorldGeometry *)hint->navigation.portals[i].neighbor;
                if (!member(first, count, p))
                    return NULL;
                if (contains(p, point, margin))
                    return p;
            }
    start = hint ? hint : first;
    p = start;
    do
    {
        if (contains(p, point, margin))
            return p;
        p = (BHWorldGeometry *)p->navigation.next;
    } while (p != start);
    return NULL;
}
int __cdecl bh_portal_center(const BHPortal *p, BHVec3 *out)
{
    BHActorPortalImage image;
    if (!p || !out || p->direction > 6 || p->direction == 3)
        return 0;
    memcpy(image.portals, p, sizeof(*p));
    bh_actor_portal_center_core(&image, 0, (float *)out);
    return 1;
}
int __cdecl bh_portal_normal(const BHPortal *p, BHVec3 *out)
{
    BHActorPortalImage image;
    if (!p || !out || p->direction > 6 || p->direction == 3)
        return 0;
    memcpy(image.portals, p, sizeof(*p));
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
int __cdecl bh_route_plan(const BHNavigation *n, BHWorldGeometry *first, unsigned long count, BHWorldGeometry *current,
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
