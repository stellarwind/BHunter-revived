#ifndef BH_EFFECT_ORBIT_TICK_H
#define BH_EFFECT_ORBIT_TICK_H
#include "bh_effect_projectile.h"
#include "bh_effect_network.h"
typedef struct BHEffectOrbitTickContext
{
    BHEffectProjectileContext *core;
    unsigned long ticks, expired, proximity, world_hits, liquid_hits;
    int error;
} BHEffectOrbitTickContext;
/* Count-zero complete source callback binding. core->orbit borrows this stable
   owner; core608/generation612/cleanup604 keep their established ABI. */
int __cdecl bh_effect_orbit_tick_bind(BHEffectOrbitTickContext *, BHEntity *);
void __cdecl bh_effect_orbit_tick(BHEntity *); /*whole00403870*/
#endif
