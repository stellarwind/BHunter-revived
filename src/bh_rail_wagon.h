#ifndef BH_RAIL_WAGON_H
#define BH_RAIL_WAGON_H
#include "bh_rail_paths.h"
#include "bh_pickup.h"
#include "bh_draw_submit.h"
typedef struct BHRailWagonPose
{
    BHVec3 right, up, forward, angles;
} BHRailWagonPose;
typedef struct BHRailWagonState
{
    BHEntityStep draw;
    unsigned long retained[2];
    BHEntity *actor_next, *actor_previous;
    long kind, subtype;
    unsigned long sound_instance;
    long sound_cooldown;
    unsigned long sound_resource, secondary_instance;
    float elapsed, duration, speed, spacing;
    unsigned long reserved;
    BHRailSegment *segment;
    BHEntity *leader;
    long direction;
    BHEntity *rider;
    BHRailSegment *checkpoint;
} BHRailWagonState;
typedef enum BHRailWagonError
{
    BH_WAGON_OK,
    BH_WAGON_ARGUMENT,
    BH_WAGON_TOPOLOGY,
    BH_WAGON_LIFETIME,
    BH_WAGON_NUMERICAL,
    BH_WAGON_WORLD,
    BH_WAGON_POOL,
    BH_WAGON_DRAW,
    BH_WAGON_SOUND
} BHRailWagonError;
typedef struct BHRailWagonContext
{
    BHRailPaths *rails;
    BHEntityPool *pool;
    BHWorldGeometry *first_world;
    unsigned long world_count;
    BHPickupRoom *rooms;
    unsigned long room_count;
    BHDrawQueue *queue;
    unsigned long model;
    BHSoundQueue *sound;
    unsigned long sound_resource;
    unsigned long spawned, draws, steps, follows, queries, transitions, migrations;
    unsigned long couples, removed, failures;
    BHRailWagonError error;
} BHRailWagonContext;
/* Real source callbacks for the five complete owners. All borrowed contexts,
   entity pool, room heads, rail allocation, model and sound identities outlive
   their wagons. Do not reorder/relink/dispose rails while a wagon is alive.
   PC53/nearest/masked finite moderate geometry is required. Source guards
   reject foreign/dead pointers, open rail walks and invalid divisors; no
   original game/global ABI, physical playback or game startup is supplied. */
BHEntity *__cdecl bh_rail_wagon_spawn(BHRailWagonContext *, const BHVec3 *, const BHVec3 *); /*004470c0*/
void __cdecl bh_rail_wagon_draw(BHEntity *);                                                 /*00446160*/
void __cdecl bh_rail_wagon_tick(BHEntity *);                                                 /*004462f0*/
void __cdecl bh_rail_wagon_follow(BHEntity *);                                               /*00446b10*/
void __cdecl bh_rail_wagon_query(BHEntity *, void *);                                        /*00446e00*/
/* Source component access/binding adapters. Pose maps actual opaque0(+18..47),
   state maps payload+4; shared actor next/previous/class offsets are retained.
   Private persistent owner metadata is outside original wagon/rider fields. */
BHRailWagonState *__cdecl bh_rail_wagon_state(BHEntity *);
BHRailWagonPose *__cdecl bh_rail_wagon_pose(BHEntity *);
int __cdecl bh_rail_wagon_set_rider(BHEntity *, BHEntity *, BHRailSegment *);
#endif
