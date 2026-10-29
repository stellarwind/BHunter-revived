#ifndef BH_ACTOR_VEHICLE_STEP_H
#define BH_ACTOR_VEHICLE_STEP_H
#include "bh_entities.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Original 004398c0 ABI, called by the checked vehicle-tick adapter. The two
   dependency adapters dispatch the real source attack/pursuit owners; their
   own matching evidence is tracked separately. */
void __cdecl bh_actor_vehicle_step(BHEntity *entity);
void __cdecl bh_actor_attack_bound(BHEntity *entity);
void __cdecl bh_actor_pursuit_bound(BHEntity *entity);
#ifdef __cplusplus
}
#endif
#endif
