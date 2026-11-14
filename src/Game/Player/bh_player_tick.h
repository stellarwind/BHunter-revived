#ifndef BH_PLAYER_TICK_H
#define BH_PLAYER_TICK_H
#include "bh_player_force_control.h"
#include "../Weapons/bh_target_construct.h"
#include "bh_player_aim.h"
#include "bh_player_scene.h"
#include "bh_player_reset.h"
#include "bh_player_query.h"
#include "../Weapons/bh_weapon_dispatch.h"
#include "../Actors/bh_remote_messages.h"
#include "../../World/bh_world_reset.h"
#include "../UI/bh_menu_runtime.h"
#include "../UI/bh_message_panel.h"
#include "../Effects/bh_impact_ring.h"
typedef int (*BHPlayerForceMethod)(BHPlayerForceControl *, float, int);
typedef struct BHPlayerTickState
{
    long paused, mode, world_mode, confirmation, ending, death_frames, god_mode;
    long damage_warning, hull_warning, repair_warning, send_delay;
    unsigned long tick, peers, saved_model;
    float saved_cd_volume, saved_wave_volume;
    BHPlayerForceMethod force;
} BHPlayerTickState;
typedef struct BHPlayerTickResources
{
    unsigned long menu_cue, ui_cue, no_weapon, damage_warning, hull_warning;
    unsigned long collision_side, electro, target_cue, lock_cue, ground_loop;
    unsigned long death_loop, death_cue, death_model, cheat_model;
    unsigned long boost_start, boost_loop, boost_end;
    unsigned long smoke_image, second_smoke, water_cloud, water_ring;
} BHPlayerTickResources;
typedef struct BHPlayerTick
{
    BHEntity *player;
    BHEntityPool *pool;
    BHPlayerTickState *state;
    BHPhysicsStepContext *physics;
    BHPhysicsStepBody *body;
    BHTargetConstructContext *targets; /* optional owner of dynamic bodies in physics */
    BHPlayerForceControl *force;
    BHPlayerInputList *input;
    BHPlayerInputState *decoded;
    BHInputSnapshot *snapshot;
    BHWeaponDispatch *weapons;
    BHPlayerCameraContext *camera;
    BHPlayerReset *reset;
    BHPlayerScene *scene;
    BHMessagePanel *panel;
    BHWorldReset *world_reset;
    BHMenuRuntimeContext *menu;
    BHMediaControl *media;
    BHSoundQueue *sound;
    BHWorldSegmentContext *world;
    BHPickupRoom *rooms;
    unsigned long room_count;
    const BHBindingIndex *room_index; /* optional immutable scene identities */
    BHScriptLifetimeContext *lifetime;
    BHScriptParticleContext *particles;
    BHContactEffects *contacts;
    BHFlashFlareContext *flash;
    BHScriptExplosionContext *explosions;
    BHNetworkGame *game;
    BHRemoteMessages *network;
    BHNetworkScores *scores;
    const BHNavigation *navigation;
    float *latency;
    const float *camera_yaw;
    BHEntity **damage_source;
    BHPlayerTickResources resources;
    unsigned char packet[64];
    BHPacketWriter writer;
    unsigned long packet_bytes;
    unsigned long generation, calls, packets, near_contacts, ground_particles;
    int error;
    const char *menu_return_stage;
} BHPlayerTick;
/* Whole0041e850. All original pause/network/physics/force/recoil/contact,
   surface particles, menu/death/respawn, weapon input and camera, cheats,
   damage/protection/audio/timer branches use the actual typed source owners.
   This is a single callback over supplied data; source fixtures never poll
   devices, inject OS input or drive a game loop. Only first36 decoded input
   words belong to player+18c..21b; bounded external state mirrors that window
   without inventing a1024-byte entity overlay. Transition counters retain
   original branch order. Source guards bound malformed cycles/selection loops,
   stale ownership and nonfinite conversions after prior effects. PC53 profile.
   One explicit live-player callback registry holds the source context; no
   pointer is stored inside original live player physics/plume/input fields.
   Unbind before retiring the pool/context. Installed refresh invokes actual scene. */
int bh_player_tick(BHPlayerTick *);
int bh_player_leave_network(BHPlayerTick *);
int bh_player_tick_bind(BHPlayerTick *, BHEntity *);
int bh_player_tick_unbind(BHPlayerTick *);
BHPlayerTick *bh_player_tick_context(BHEntity *);
void bh_player_step(BHEntity *);
#endif
