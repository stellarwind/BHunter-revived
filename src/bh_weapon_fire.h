#ifndef BH_WEAPON_FIRE_H
#define BH_WEAPON_FIRE_H
#include "bh_weapon_common.h"
#include "bh_projectile_create.h"
#include "bh_area_projectile.h"
#include "bh_messages.h"
typedef struct BHWeaponFireContext
{
    BHProjectileCreateContext *projectiles;
    BHAreaProjectileContext *area;
    BHSoundQueue *sound;
    BHMessageState *messages;
    unsigned long *empty_warning;
    unsigned long ballistic_sound, homing_sound, empty_sound, area_sound;
    unsigned long shots, bursts, empty_cues;
    int error;
} BHWeaponFireContext;
int __cdecl bh_weapon_fire_empty(BHWeaponFireContext *, const BHEntity *);
/* Whole actual original table methods. Initialized borrowed contexts retain
   live projectile factories and sound owners. */
int __cdecl bh_weapon_fire_ballistic(BHWeaponFireContext *, BHEntity *, BHEntity *); /*004305f0*/
int __cdecl bh_weapon_fire_homing(BHWeaponFireContext *, BHEntity *, BHEntity *);    /*00430a30*/
int __cdecl bh_weapon_area_start(BHWeaponFireContext *, BHEntity *, BHEntity *);     /*00431db0*/
int __cdecl bh_weapon_area_tick(BHWeaponFireContext *, BHEntity *, BHEntity *);      /*00431f00*/
#endif
