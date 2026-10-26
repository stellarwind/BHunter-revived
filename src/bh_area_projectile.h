#ifndef BH_AREA_PROJECTILE_H
#define BH_AREA_PROJECTILE_H
#include "bh_projectile_create.h"
/* Borrowed initialized owners, response table and ordered global actor registry
   remain valid through synchronous callbacks. No native playback is invoked. */
typedef struct BHAreaProjectileContext {
 BHHomingProjectileContext *effects;
 BHProjectileDrawContext *draw;
 unsigned long *identity;
 unsigned long loop_sound,hit_sound;
 unsigned long collisions,room_hits,global_hits;
 int error;
} BHAreaProjectileContext;
BHEntity *__cdecl bh_area_projectile_create(BHAreaProjectileContext *,BHWorldGeometry *,const BHVec3 *,const BHVec3 *,BHEntity *source,float strength); /*00442360*/
void __cdecl bh_area_projectile_tick(BHEntity *); /*00441d80*/
#endif
