#ifndef BH_WEAPON_REMOVAL_H
#define BH_WEAPON_REMOVAL_H
#include "bh_weapon_common.h"
typedef struct BHWeaponRemovedPacket {long event;BHEntity *entity;} BHWeaponRemovedPacket;
/* Complete00433e50: event17 matching the current target clears it before
   stopping its optional actual sound handle, then writes handle-1. Charge,
   cache and unrelated fields survive. Stop's return is ignored as original.
   Borrowed packet/weapon/sound survive the synchronous call; no query method
   or constructor is installed by this explicit source binding. */
int __cdecl bh_weapon_target_removed(BHSoundQueue *,BHEntity *,const BHWeaponRemovedPacket *);
#endif
