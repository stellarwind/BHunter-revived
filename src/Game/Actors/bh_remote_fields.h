#ifndef BH_REMOTE_FIELDS_H
#define BH_REMOTE_FIELDS_H
#include "../../Math/bh_math.h"
#include "../Player/bh_player_propulsion.h"

struct BHWorldGeometry;
struct BHEntity;

/* Nine original 12-byte firing schedules. Weapon-specific counters reuse
   these same words; the packet count is retained independently of remaining. */
typedef struct BHRemoteFireSchedule
{
    unsigned short remaining, spacing, countdown, packet_count;
    struct BHEntity *target;
} BHRemoteFireSchedule;

typedef struct BHRemotePose
{
    unsigned short tick;
    unsigned char reserved[2];
    BHVec3 position, angles;
    struct BHWorldGeometry *world;
} BHRemotePose;

typedef struct BHRemoteFields
{
    unsigned char reserved_before_sound[0x104 - 0xe4];
    unsigned long engine_sound, sound_frames, engine_resource;
    /* Initialized to the invalid sound handle; no further consumer recovered. */
    unsigned long reserved_sound;
    short slot, roster_slot;
    unsigned short muzzle_phase;
    unsigned short pose_flags;
    short score;
    char name[20];
    unsigned char reserved_before_cached_world[2];
    struct BHWorldGeometry *cached_world;
    unsigned long player_id;
    BHRemotePose oldest, middle, newest;
    BHRemoteFireSchedule firing[9];
    unsigned char reserved_before_propulsion[0x2c0 - 0x208];
    unsigned char propulsion[BH_PROPULSION_CHANNELS];
} BHRemoteFields;
#endif
