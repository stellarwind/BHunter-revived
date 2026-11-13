#ifndef BH_FLYBY_FIELDS_H
#define BH_FLYBY_FIELDS_H
#include "bh_camera_state.h"

#pragma pack(push, 4)
typedef struct BHFlybyFields
{
    unsigned char reserved_before_camera[4];
    BHCameraState camera;
    unsigned char reserved_before_phase[0x1dc - 0x184];
    unsigned long route_phase;
    unsigned char reserved_before_time[0x1f8 - 0x1e0];
    double orbit_time;
    unsigned char reserved_before_history[0x23c - 0x200];
    BHVec3 previous_position, older_position;
    unsigned char reserved_before_waypoint[0x284 - 0x254];
    long waypoint_index;
} BHFlybyFields;
#pragma pack(pop)
#endif
