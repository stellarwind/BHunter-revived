#ifndef BH_CAMERA_ROUTE_H
#define BH_CAMERA_ROUTE_H
#include "bh_route_query.h"
typedef struct BHCameraRouteState {
    BHWorldGeometry *current,*next;
    unsigned long waypoint_index;
    BHVec3 curve[4];
    float phase,parameter_rate;
} BHCameraRouteState;
typedef enum BHCameraRouteError {
    BH_CAMERA_ROUTE_OK,BH_CAMERA_ROUTE_ARGUMENT,BH_CAMERA_ROUTE_TOPOLOGY,
    BH_CAMERA_ROUTE_WAYPOINT,BH_CAMERA_ROUTE_PORTAL
} BHCameraRouteError;
/* Real source navigation, world ring, and selected waypoint positions are
   borrowed. Position array projects the original 32-byte camera-table rows;
   its construction/selection owner remains separate. No pathfinding callback,
   original pointer, invented waypoint or fallback route is supplied here. */
typedef struct BHCameraRouteContext {
    const BHNavigation *navigation;
    BHWorldGeometry *first;
    unsigned long scene_count;
    const BHVec3 *waypoints;
    unsigned long waypoint_count,selected_waypoint;
    BHCameraRouteError error;
} BHCameraRouteContext;
/* Complete004296a0: promote next room, choose corridor continuation or actual
   destination-major route exit, build four interpolation controls, detour when
   entering/leaving one portal, and preserve fractional phase across rate change.
   Finite phase within signed32 range and positive finite old rate required.
   Invalid original uninitialized-slot/foreign-world cases reject transactionally.
   Zero control length retains original masked-x87 infinity/NaN output behavior.
   Requires PC53/nearest; storage/context must not overlap. */
int __cdecl bh_camera_route_advance(BHCameraRouteContext *,BHCameraRouteState *);
#endif
