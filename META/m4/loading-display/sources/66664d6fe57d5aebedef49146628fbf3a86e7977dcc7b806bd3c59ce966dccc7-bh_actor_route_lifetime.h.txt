#ifndef BH_ACTOR_ROUTE_LIFETIME_H
#define BH_ACTOR_ROUTE_LIFETIME_H
#include "bh_actor_route.h"
/* Complete00435790: signed16 wrap test of tick-duration_ticks-start_tick.
   Negative wrapped delta does nothing. Otherwise advance ONCE through real
   004355c0 owner and write reloaded tick+original_delta modulo65536 as stamp,
   even though promotion wrote a new duration/stamp. No repeated catch-up loop.
   Context/live actor/profile requirements are those of the real route owner. */
int __cdecl bh_actor_route_due(BHActorRouteContext *,BHEntity *);
/* Complete00435720. Stop non-invalid actual sound voice, unlink verified game
   f4/f8 list from existing BHPickupRoom.actors, release through real entity pool.
   Do not clear retained payload or force the old sound handle invalid: original
   cleanup leaves those bytes for slot reuse. Borrowed owners must remain live.
   Source binding/list/pool guards reject before effects; no cleanup callback,
   native buffer or original global is fabricated. */
int __cdecl bh_actor_route_release(BHActorRouteContext *,BHEntityPool *,BHEntity *);
#endif
