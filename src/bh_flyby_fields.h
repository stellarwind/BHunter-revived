#ifndef BH_FLYBY_FIELDS_H
#define BH_FLYBY_FIELDS_H
#include "bh_camera_state.h"

typedef struct BHFlybyFields
{
    unsigned char reserved_before_camera[4];
    BHCameraState camera;
    unsigned char reserved_before_history[0x23c - 0x184];
    BHVec3 previous_position, older_position;
    unsigned char reserved_before_waypoint[0x284 - 0x254];
    long waypoint_index;
} BHFlybyFields;
#endif
