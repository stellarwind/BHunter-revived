#ifndef BH_ACTOR_ATTACK_H
#define BH_ACTOR_ATTACK_H
#include "bh_actor_pursuit_route.h"
#include "bh_effect_projectile.h"
#include "bh_area_projectile.h"
typedef struct BHActorAttackContext {
 BHActorPursuitRouteContext *pursuit;
 BHEffectProjectileContext *effects;
 BHProjectileCreateContext *projectiles;BHAreaProjectileContext *area;
 unsigned long orbit_cue,area_sound,ballistic_sound,homing_sound,impact_sound,liquid_sound;
 unsigned long split_model,smoke_image,impact_image,flash_image,beam_image;
 /* Beam contact leaves the original packet severity stack word unwritten.
    Explicit source scratch binding; no original value is asserted. */
 float legacy_contact_severity;
 unsigned long attacks,specials,beams;int error;
} BHActorAttackContext;
/* Whole00437ae0. Stable real owners and synchronous target query motion are
   borrowed. Source guards stop unsupported owner/lifetime mutation. */
int __cdecl bh_actor_attack(BHActorAttackContext *,BHEntity *);
/* Count-zero binding for complete004398c0 tick; payload+596 private context. */
int __cdecl bh_actor_vehicle_tick_bind(BHActorAttackContext *,BHEntity *);
void __cdecl bh_actor_vehicle_tick(BHEntity *);
#endif
