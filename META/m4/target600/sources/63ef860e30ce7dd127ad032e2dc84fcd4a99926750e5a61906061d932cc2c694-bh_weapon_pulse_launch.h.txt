#ifndef BH_WEAPON_PULSE_LAUNCH_H
#define BH_WEAPON_PULSE_LAUNCH_H
#include "bh_weapon_fire.h"
#include "bh_pulse_projectile.h"
typedef struct BHWeaponPulseLaunchContext {BHWeaponFireContext *fire;BHPulseProjectileContext *projectile;unsigned long sound;}BHWeaponPulseLaunchContext;
int __cdecl bh_weapon_pulse_launch(BHWeaponPulseLaunchContext *,BHEntity *,BHEntity *); /*00433b30*/
#endif
