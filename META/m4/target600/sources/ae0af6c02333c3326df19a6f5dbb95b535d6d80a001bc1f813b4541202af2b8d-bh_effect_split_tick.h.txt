#ifndef BH_EFFECT_SPLIT_TICK_H
#define BH_EFFECT_SPLIT_TICK_H
#include "bh_effect_projectile.h"
#include "bh_effect_network.h"
typedef struct BHEffectSplitContext {unsigned long split_sound,cascades,collisions,jitters,liquid_hits;int error;}BHEffectSplitContext;
/* Count-zero actual borrowed family binding. Caller initializes c->split and
   c->network and actual effect dependencies before constructors. */
void __cdecl bh_effect_split_tick(BHEntity *); /*004047c0*/
int __cdecl bh_effect_split_cascade(BHEntity *,const BHVec3 *); /*004044c0*/
/* Parent-owned actual constructor dependency; definition is required at link. */
BHEntity *__cdecl bh_effect_shard_create(BHEffectProjectileContext *,BHWorldGeometry *,BHEntity *,const BHVec3 *,const BHVec3 *,float,int,unsigned long,float);
#endif
