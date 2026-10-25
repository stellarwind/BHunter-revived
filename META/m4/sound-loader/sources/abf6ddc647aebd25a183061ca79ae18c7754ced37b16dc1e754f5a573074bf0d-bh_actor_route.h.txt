#ifndef BH_ACTOR_ROUTE_H
#define BH_ACTOR_ROUTE_H
#include "bh_pickup.h"

/* Named view of verified fields, not a competing full entity layout. Storage
   remains in the real BHEntity. current/next borrow source world definitions;
   the context resolves their actual existing BHPickupRoom.actors list owners. */
typedef struct BHActorRouteState {
    BHWorldGeometry *current,*next;
    BHVec3 controls[4],portal_center;
    float phase,duration,speed,u,v,speed_scale,jitter;
    unsigned long sound,sound_frames,random;
    unsigned short tick,duration_ticks;
} BHActorRouteState;
typedef enum BHActorRouteError {
    BH_ACTOR_ROUTE_OK,BH_ACTOR_ROUTE_ARGUMENT,BH_ACTOR_ROUTE_TOPOLOGY,BH_ACTOR_ROUTE_NUMERICAL
} BHActorRouteError;
typedef struct BHActorRouteContext {
    BHPickupRoom *rooms;unsigned long room_count,actor_limit;
    BHSoundQueue *sound;
    unsigned short tick; /* explicit original004dfac4 binding */
    BHActorRouteError error;
} BHActorRouteContext;
/* Explicit source field-view adapters, not original function admissions. They
   copy only the verified fields. Bind worlds/actors before the owning update;
   these do not create actors or install callbacks. */
int __cdecl bh_actor_route_read(const BHEntity *,BHActorRouteState *);
int __cdecl bh_actor_route_write(BHEntity *,const BHActorRouteState *);
/* Complete00434f40. Integer incoming/outgoing portal indices (the decompiler's
   provisional float outgoing type is contradicted by raw EAX/index arithmetic).
   Promote next world, preserve actual f4/f8 room-list links, build four route
   controls and portal center, reset phase, duration/30Hz tick words. Context
   room heads and all actor/world/model/neighbor lifetimes must remain stable.
   PC53 nearest/masked, finite moderate state and positive speed factors required.
   Source validation errors precede this owner's mutation. */
int __cdecl bh_actor_route_promote(BHActorRouteContext *,BHEntity *,long incoming,long outgoing);
/* Complete004355c0. Original sound countdown/stop, last matching incoming
   neighbor (first slot fallback), actual random/guided selector, two exact RNG
   transitions for aperture bias, then real promotion owner. Earlier sound,
   RNG and bias effects remain observable if a later source guard fails. */
int __cdecl bh_actor_route_advance(BHActorRouteContext *,BHEntity *);
#endif
