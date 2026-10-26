#ifndef BH_WEAPON_ARC_FIRE_H
#define BH_WEAPON_ARC_FIRE_H
#include "bh_weapon_fire.h"
#include "bh_weapon_path.h"
typedef struct BHWeaponArcFireContext {BHWeaponFireContext *fire;BHScriptParticleContext *particles;BHFlashFlareContext *flash;unsigned long first_image,second_image,sound,shots,queries;}BHWeaponArcFireContext;
int __cdecl bh_weapon_arc_fire(BHWeaponArcFireContext *,BHEntity *,BHEntity *); /*004327a0*/
#endif
