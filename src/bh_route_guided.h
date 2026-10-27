#ifndef BH_ROUTE_GUIDED_H
#define BH_ROUTE_GUIDED_H
#include "bh_route_choice.h"
/* Complete00434650. Prefer the eligible portal opposite the incoming slot's
   axis (direction XOR4), minimizing lateral/up displacement between actual
   scratch-aperture centers. Uses existing center/normal/cross owners and same
   actor-class/neighbor-flag eligibility as bh_route_choose_exit. No RNG change.
   Retains mixed float stores and comparison-before-score-store (unordered
   selects as original FCOM C0); exact ordered score ties retain earlier slot.
   No match leaves selected=incoming. Pointers/models/portals must stay live;
   outputs must not alias inputs. Requires caller PC53 nearest/masked profile.
   Missing bindings/count mismatch/>6/invalid incoming direction or index return
   0 before output publication; this guards original uninitialized/OOB inputs. */
int __cdecl bh_route_guided_exit(const BHEntity *actor, const BHWorldGeometry *room, long incoming, long *selected);
#endif
