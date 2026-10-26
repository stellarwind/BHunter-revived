#ifndef BH_EFFECT_ACCELERATED_H
#define BH_EFFECT_ACCELERATED_H
#include "bh_effect_projectile.h"
#include "bh_effect_puff.h"
#include "bh_effect_network.h"
typedef struct BHEffectAcceleratedContext {
 BHEffectPuffContext *puffs;unsigned long ring_image,second_ring_image,random_image;
 unsigned long interval_sound,expiry_sound;unsigned long collisions,liquid_hits,puff_entities;int error;
} BHEffectAcceleratedContext;
/* Whole00405270. Borrowed source pool/world/response/particle/sound bindings.
   Original fields and actual source callback owners are authoritative. */
void __cdecl bh_effect_accelerated_tick(BHEntity *);
/* Shared zero-count source call-site helper, exact query/tick detonation tail. */
void __cdecl bh_effect_detonate(BHEffectProjectileContext *,BHEntity *);
#endif
