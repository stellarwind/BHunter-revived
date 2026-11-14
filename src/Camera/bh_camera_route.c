#include "bh_camera_route.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <math.h>

static float stored(double value)
{
    float out;
    bh_round_float(&out, value);
    return out;
}
static int fail(BHCameraRouteContext *context, BHCameraRouteError error)
{
    if (context)
        context->error = error;
    return 0;
}
static int finite_vector(const BHVec3 *vector)
{
    return _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static long entry_port(const BHWorldGeometry *room, const BHWorldGeometry *previous)
{
    unsigned long i;
    long found = -1;
    for (i = 0; i < room->navigation.count; ++i)
        if (room->navigation.portals[i].neighbor == &previous->navigation)
            found = (long)i;
    return found;
}
static int point(const BHWorldGeometry *room, long slot, BHVec3 *center, BHVec3 *normal)
{
    return slot >= 0 && (unsigned long)slot < room->navigation.count &&
           bh_portal_center(room->navigation.portals + slot, center) &&
           bh_portal_normal(room->navigation.portals + slot, normal);
}
static void project(const BHVec3 *from, const BHVec3 *toward, const BHVec3 *normal, BHVec3 *out, int terminal)
{
    double dx = (double)toward->x - from->x, dy = (double)toward->y - from->y, dz = (double)toward->z - from->z, dot;
    if (terminal)
        dz = stored(dz);
    dot = (dx * normal->x + dy * normal->y) + dz * normal->z;
    out->x = stored((double)from->x + dot * normal->x);
    out->y = stored(dot * normal->y + from->y);
    out->z = stored(dot * normal->z + from->z);
}
static double length(const BHVec3 *first, const BHVec3 *second, int terminal_last)
{
    double x = (double)first->x - second->x, y = (double)first->y - second->y, z = (double)first->z - second->z;
    if (terminal_last)
        z = stored(z);
    return sqrt((x * stored(x) + y * stored(y)) + z * stored(z));
}
static void detour(const BHWorldGeometry *room, BHVec3 *first_point, BHVec3 *second_point)
{
    BHVec3 center;
    center.x = stored((double)stored((double)room->minimum.x * .5) - (double)room->maximum.x * (-.5));
    center.y = stored((double)stored((double)room->minimum.y * .5) - (double)room->maximum.y * (-.5));
    center.z = stored((double)stored((double)room->minimum.z * .5) - (double)room->maximum.z * (-.5));
    *first_point = *second_point = center;
    first_point->x = stored((double)first_point->x - 10);
    first_point->z = stored((double)first_point->z - 10);
    second_point->x = stored((double)second_point->x - (-10));
    second_point->z = stored((double)second_point->z - (-10));
}
int bh_camera_route_advance(BHCameraRouteContext *context, BHCameraRouteState *state)
{
    BHCameraRouteState next;
    BHWorldGeometry *room, *destination, *previous;
    BHVec3 incoming_normal, outgoing_normal;
    long current_index, destination_index, incoming, outgoing = -1, whole;
    unsigned long i;
    double sum, rate, time;
    float rounded_time;
    int branch;
    if (!context || !state || !context->navigation || !context->navigation->nodes || !context->first ||
        !context->scene_count || context->scene_count > 65535 ||
        !context->waypoints || !context->waypoint_count || !_finite(state->phase) || state->phase < -2147483648.0 ||
        state->phase >= 2147483648.0 || !_finite(state->parameter_rate) || state->parameter_rate <= 0)
        return fail(context, BH_CAMERA_ROUTE_ARGUMENT);
    if (context->selected_waypoint >= context->waypoint_count ||
        !finite_vector(context->waypoints + context->selected_waypoint))
        return fail(context, BH_CAMERA_ROUTE_WAYPOINT);
    room = state->next;
    previous = state->current;
    current_index = bh_navigation_find(context->navigation, room);
    if (current_index < 0 || bh_navigation_find(context->navigation, previous) < 0 || room->navigation.count > 6)
        return fail(context, BH_CAMERA_ROUTE_TOPOLOGY);
    incoming = entry_port(room, previous);
    if (incoming < 0)
        return fail(context, BH_CAMERA_ROUTE_PORTAL);
    next = *state;
    next.current = room;
    if ((unsigned long)current_index >= context->navigation->junction_count)
    {
        branch = 0;
        for (i = 0; i < room->navigation.count; ++i)
            if (room->navigation.portals[i].neighbor && room->navigation.portals[i].neighbor != &previous->navigation)
                outgoing = (long)i;
        if (outgoing < 0)
            return fail(context, BH_CAMERA_ROUTE_PORTAL);
        next.next = (BHWorldGeometry *)room->navigation.portals[outgoing].neighbor;
    }
    else
    {
        destination = bh_world_locate(context->first, context->scene_count, NULL,
                                      context->waypoints + context->selected_waypoint, 0);
        if (!destination)
            return fail(context, BH_CAMERA_ROUTE_WAYPOINT);
        if (destination == room)
        {
            branch = 2;
            next.waypoint_index = context->selected_waypoint;
            if (!point(room, incoming, next.curve, &incoming_normal))
                return fail(context, BH_CAMERA_ROUTE_PORTAL);
            next.curve[2] = next.curve[0];
            next.curve[3] = context->waypoints[next.waypoint_index];
            project(next.curve, next.curve + 3, &incoming_normal, next.curve + 1, 1);
        }
        else
        {
            branch = 1;
            destination_index = bh_navigation_find(context->navigation, destination);
            if (destination_index < 0 || (unsigned long)destination_index >= context->navigation->junction_count ||
                context->navigation->routes.node_count != context->navigation->junction_count ||
                !context->navigation->routes.cells)
                return fail(context, BH_CAMERA_ROUTE_TOPOLOGY);
            outgoing = context->navigation->routes
                           .cells[(unsigned long)destination_index * context->navigation->junction_count + current_index]
                           .exit_slot;
            if (outgoing < 0 || (unsigned long)outgoing >= room->navigation.count ||
                !room->navigation.portals[outgoing].neighbor)
                return fail(context, BH_CAMERA_ROUTE_PORTAL);
            next.next = (BHWorldGeometry *)room->navigation.portals[outgoing].neighbor;
        }
    }
    if (branch != 2)
    {
        if (bh_navigation_find(context->navigation, next.next) < 0 ||
            !point(room, incoming, next.curve, &incoming_normal) ||
            !point(room, outgoing, next.curve + 3, &outgoing_normal))
            return fail(context, BH_CAMERA_ROUTE_PORTAL);
        project(next.curve, next.curve + 3, &incoming_normal, next.curve + 1, 0);
        project(next.curve + 3, next.curve, &outgoing_normal, next.curve + 2, 0);
        if (branch == 1 && incoming == outgoing)
        {
            if (!finite_vector(&room->minimum) || !finite_vector(&room->maximum))
                return fail(context, BH_CAMERA_ROUTE_TOPOLOGY);
            detour(room, next.curve + 1, next.curve + 2);
        }
    }
    whole = (long)state->phase;
    time = ((double)state->phase - whole) / state->parameter_rate;
    sum = length(next.curve, next.curve + 1, 0) + length(next.curve + 3, next.curve + 2, branch == 2);
    if (branch == 0 || branch == 2)
        sum = stored(sum);
    rate = 4.666666666666667 / sum;
    next.parameter_rate = stored(rate);
    if (branch == 2)
        next.phase = stored(time * next.parameter_rate - (-2.0));
    else
    {
        rounded_time = stored(time);
        next.phase = stored((double)rounded_time * (branch == 0 ? rate : next.parameter_rate) - (-1.0));
    }
    *state = next;
    context->error = BH_CAMERA_ROUTE_OK;
    return 1;
}
