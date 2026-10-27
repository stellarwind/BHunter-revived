#ifndef BH_WEAPON_PATH_H
#define BH_WEAPON_PATH_H
#include "bh_weapon_common.h"
/* Whole00432370. Four input vectors captured before output mutation, allowing
   any output/input alias. PC53 nearest, finite moderate coordinates. */
void __cdecl bh_weapon_path_blend(BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *, float);
/* Whole004325d0. Actual weapon+108 eligibility and+f8/+fc target/charge,
   player+24 up,+30 side,+280 forward. Target query remains synchronous and
   borrowed; output is untouched when gate rejects. Returns1 iff evaluated. */
int __cdecl bh_weapon_path_point(BHEntity *, BHEntity *, BHVec3 *, float);
#endif
