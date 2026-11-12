#ifndef BH_REMOTE_FIELDS_H
#define BH_REMOTE_FIELDS_H
#include "bh_math.h"
#include "bh_player_propulsion.h"

struct BHWorldGeometry;

typedef struct BHRemotePose
{
    unsigned short tick;
    unsigned char reserved[2];
    BHVec3 position, angles;
    struct BHWorldGeometry *world;
} BHRemotePose;

typedef struct BHRemoteFields
{
    unsigned char reserved_before_flags[0x11a - 0xe4];
    unsigned short pose_flags;
    unsigned char reserved_before_history[0x13c - 0x11c];
    BHRemotePose oldest, middle, newest;
    unsigned char reserved_before_propulsion[0x2c0 - 0x19c];
    unsigned char propulsion[BH_PROPULSION_CHANNELS];
} BHRemoteFields;
#endif
