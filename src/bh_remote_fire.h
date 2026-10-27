#ifndef BH_REMOTE_FIRE_H
#define BH_REMOTE_FIRE_H
#include "bh_remote_render.h"
#include "bh_weapon_pulse.h"
#include "bh_weapon_effect_fire.h"
#include "bh_weapon_pulse_launch.h"
#include "bh_weapon_hud_arc.h"
typedef struct BHRemoteFire
{
    BHWeaponPulseContext *pulse;
    BHProjectileCreateContext *projectiles;
    BHEffectProjectileContext *effects;
    BHAreaProjectileContext *area;
    BHPulseProjectileContext *pulse_projectile;
    BHArcContext *arc;
    BHEntity *local;
    unsigned long *tick, *sync_identity;
    unsigned long ballistic_sound, homing_sound, target_sound, area_sound, arc_sound;
    unsigned long special_sound, arc_image, arc_flare, split_model;
    unsigned long calls, shots, queries;
    int error;
} BHRemoteFire;
/* Whole00424160, all nine types. Temporarily replaces only low16 global tick,
   predicts real remote pose then restores previous low16 after all source
   branches. Upper16 changes during callbacks survive. Original shot counters,
   burst state, target queries, shared random order, traces/effects and actual
   projectile constructors remain authoritative. Caller owns explicit native
   source contexts and actor/world lifetimes. No OS input or network gameplay.
   Finite moderate PC53. Missing owners/methods and malformed states reject
   after earlier effects; no guessed globals/vtables or original execution. */
int __cdecl bh_remote_fire(BHRemoteFire *, BHEntity *, short shot_tick, unsigned long type, BHEntity *target);
#endif
