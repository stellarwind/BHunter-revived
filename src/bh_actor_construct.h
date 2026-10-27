#ifndef BH_ACTOR_CONSTRUCT_H
#define BH_ACTOR_CONSTRUCT_H
#include "bh_actor_response.h"
#include "bh_actor_vehicle_draw.h"
typedef struct BHActorConstructRoom
{
    BHPickupRoom *room;
    unsigned long bound_resource;
} BHActorConstructRoom;
typedef struct BHActorConstructContext
{
    BHActorResponseContext *response;
    BHActorVehicleDrawContext *draw;
    BHWorldGeometry *first;
    unsigned long world_count;
    const BHActorConstructRoom *rooms;
    unsigned long room_count;
    const unsigned long *models, *sounds;
    unsigned long resource_count;
    const unsigned long *cycle_models;
    unsigned long cycle_count, cycle;
    unsigned long special_model;
    int error;
} BHActorConstructContext;
/* Whole0043a920 /0043b2d0. The positional constructor locates a room twice
   (margins0 then1) and starts its curve at that room's center too. Explicit
   loaded resources replace original runtime tables; no image/global overlay.
   Draw/route/query callbacks call actual source owners. All bindings borrowed.
   NULL/missing room is a bounded error instead of original fatal exit.
   Recycled fields not initialized by the originals remain retained. */
BHEntity *__cdecl bh_actor_construct_room(BHActorConstructContext *, BHWorldGeometry *, unsigned long type, long token,
                                          float speed, unsigned long death_tag, unsigned long seed);
BHEntity *__cdecl bh_actor_construct_position(BHActorConstructContext *, const BHVec3 *, unsigned long type, long token,
                                              float speed, unsigned long death_tag, unsigned long seed);
#endif
