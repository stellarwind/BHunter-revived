#ifndef BH_WEAPON_TIMER_H
#define BH_WEAPON_TIMER_H
#include "bh_weapon_common.h"
/* Complete identical index8 00433820 and index7 00433b00 method+12c.
   The source owner is shared; each table identity remains distinct. Strictly
   positive binary32 timer +f0 subtracts the original double 1/30 and stores
   binary32. Other state and nonpositive/unordered raw bits are retained.
   Caller borrows a live weapon and masked PC53/nearest FP environment. */
int __cdecl bh_weapon_single_timer_tick(BHEntity *weapon);
#endif
