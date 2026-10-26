#ifndef BH_ACTOR_RESPONSE_H
#define BH_ACTOR_RESPONSE_H
#include "bh_actor_damage.h"
#include "bh_actor_attack.h"
#include "bh_actor_vehicle_lifetime.h"
typedef struct BHActorResponseContext {
 BHActorDamageContext *damage;
 BHActorAttackContext *attack;
 BHActorVehicleLifetimeContext *vehicles;
 BHScriptLifetimeContext *mission;
 BHEntity *player;
 unsigned long mission_resource;
 int error;
} BHActorResponseContext;
/* Whole special-vehicle query0043a1d0 and recoil/death tick00439910.
   Contexts and both intrusive lists remain stable during synchronous queries.
   Curve sampling uses phase/duration here, rather than the ordinary actor's
   signed network-tick ratio. Context payload+588 is source-private storage.
   Death angles remain the explicitly bound legacy input in damage context.
   Finite PC53 state, nonzero positive route duration/speed are required. */
int __cdecl bh_actor_response_bind(BHActorResponseContext *,BHEntity *);
void __cdecl bh_actor_response_query(BHEntity *,void *);
void __cdecl bh_actor_response_tick(BHEntity *);
#endif
