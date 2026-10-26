#ifndef BH_PLAYER_AIM_H
#define BH_PLAYER_AIM_H
#include "bh_physics_step.h"
/* Whole00417c50. Read the real player's two aim scalars+1f0/+1f4, project
   through the SAME authoritative physics pose basis and publish direction at
   +280 and local direction at+28c. Zero controls copy forward bits unchanged.
   Original threefold input scale and seventh-power soft clamp retain singular
   masked NaN at scaled +/-1; no repaired clamp is substituted. PC53 finite
   moderate controls/basis; CRTpow/sin/cos reuse, no original code execution.
   Body/entity/physics must be live, disjoint and remain owned by caller. */
int __cdecl bh_player_aim_update(BHPhysicsStepBody *);
#endif
