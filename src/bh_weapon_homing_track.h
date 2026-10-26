#ifndef BH_WEAPON_HOMING_TRACK_H
#define BH_WEAPON_HOMING_TRACK_H
#include "bh_weapon_target_lifecycle.h"
/*Whole00430d80. Original shared target actor+f0 chain, cached world+104,
  signed charge and independent cooldowns. Borrowed owners survive queries.*/
int __cdecl bh_weapon_homing_track(BHWeaponTargetContext *,BHEntity *,const BHEntity *);
#endif
