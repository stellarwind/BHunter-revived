#ifndef BH_PLAYER_FIELDS_H
#define BH_PLAYER_FIELDS_H
#include "../../Camera/bh_camera_state.h"
#include "bh_player_propulsion.h"

struct BHEntity;
struct BHPlayerInputList;
struct BHPlayerCreate;

enum
{
    BH_PLAYER_INPUT_WORDS = 36,
    BH_PLAYER_INPUT_AIM_X = 25,
    BH_PLAYER_INPUT_AIM_Y = 26,
    BH_PLAYER_INPUT_ESCAPE = 33
};

typedef struct BHPlayerNozzleTrail
{
    BHVec3 from, to;
} BHPlayerNozzleTrail;

/* Player view of the entity payload. Other entity types reuse this storage. */
typedef struct BHPlayerFields
{
    unsigned char reserved_before_camera[4];
    BHCameraState camera;
    unsigned char reserved_after_camera[4];
    struct BHPlayerInputList *input_list;
    union
    {
        unsigned long words[BH_PLAYER_INPUT_WORDS];
        float axes[BH_PLAYER_INPUT_WORDS];
    } input;
    unsigned long model_resource;
    unsigned long engine_sound, death_sound, ground_sound, shield_sound;
    long cash, fine, bounty_pending;
    union
    {
        float damage;
        float spawn_delay;
    };
    float shield_time, shield_fade, shield_pitch_time;
    float wear, instability, damage_flash;
    struct BHEntity *weapons[10];
    BHVec3 aim_direction, aim_offset, recoil_position, recoil_velocity;
    short camera_mode, camera_countdown, disabled_frames;
    unsigned char selected_weapon[2];
    unsigned char frame_phase, cash_flash, fine_flash, vehicle;
    union
    {
        float channels[15];
        struct
        {
            float center_channel;
            BHVec3 nozzles[BH_PLAYER_NOZZLE_COUNT];
        } thrust;
        struct
        {
            float reserved[11];
            struct BHPlayerCreate *creation;
        } delayed;
    } propulsion;
    float engine_output;
    BHPlayerNozzleTrail nozzle_trail[BH_PLAYER_NOZZLE_COUNT];
} BHPlayerFields;
#endif
