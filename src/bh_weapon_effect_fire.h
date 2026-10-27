#ifndef BH_WEAPON_EFFECT_FIRE_H
#define BH_WEAPON_EFFECT_FIRE_H
#include "bh_weapon_fire.h"
#include "bh_effect_projectile.h"
typedef struct BHWeaponEffectFireContext
{
    BHWeaponFireContext *fire;
    BHEffectProjectileContext *projectile;
    unsigned long split_model, shot_sound;
} BHWeaponEffectFireContext;
int __cdecl bh_weapon_effect_orbit_fire(BHWeaponEffectFireContext *, BHEntity *, BHEntity *);       /*00431810*/
int __cdecl bh_weapon_effect_split_fire(BHWeaponEffectFireContext *, BHEntity *, BHEntity *);       /*004331c0*/
int __cdecl bh_weapon_effect_accelerated_fire(BHWeaponEffectFireContext *, BHEntity *, BHEntity *); /*00433850*/
#endif
