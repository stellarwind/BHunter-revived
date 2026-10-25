#ifndef BH_WEAPON_HUD_STATE_H
#define BH_WEAPON_HUD_STATE_H
#include "bh_entities.h"
/* Observed borrowed bindings of selected weapon +f8 target and +fc signed
   charge. This is not an assertion of a compact original weapon allocation.
   The target/its real installed query callback survive the synchronous call;
   the query may mutate charge or replace target. Callers preserve that state
   lifetime through the post-query reads. No weapon callbacks are invented. */
typedef struct BHWeaponHudState {
    BHEntity *target;
    long charge;
} BHWeaponHudState;
#endif
