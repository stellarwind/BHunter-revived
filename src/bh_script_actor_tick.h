#ifndef BH_SCRIPT_ACTOR_TICK_H
#define BH_SCRIPT_ACTOR_TICK_H
#include "bh_script_actor_route.h"
/* Whole004374b0/004375b0. Explicit borrowed sound/clock globals; pulse resource
   is original004d441c loaded resource. No sound backend or resource fabricated. */
int __cdecl bh_script_actor_clock_tick(BHActorRouteContext *, BHEntity *);
int __cdecl bh_script_actor_phase_tick(BHActorRouteContext *, BHEntity *, unsigned long pulse_resource);
#endif
