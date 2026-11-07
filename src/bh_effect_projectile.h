#ifndef BH_EFFECT_PROJECTILE_H
#define BH_EFFECT_PROJECTILE_H
#include "bh_homing_projectile.h"
#include "bh_effect_cleanup.h"
#include "bh_effect_draw.h"
#include "bh_trail.h"
#include "bh_script_explosion.h"
struct BHEffectOrbitTickContext;
struct BHEffectAcceleratedContext;
struct BHEffectSplitContext;
struct BHEffectNetworkContext;
typedef union BHEffectPacket {
    long words[10];
    float floats[10];
} BHEffectPacket;
typedef struct BHEffectProjectileContext
{
    struct BHEffectOrbitTickContext *orbit;
    struct BHEffectAcceleratedContext *accelerated;
    BHHomingProjectileContext *effects;
    BHEffectCleanupContext cleanup;
    BHScriptDebrisContext *debris;
    BHScriptExplosionContext *explosion;
    BHEffectDrawContext *draw;
    BHTrailContext *trails;
    unsigned long *pending_identity;
    struct BHEffectSplitContext *split;
    struct BHEffectNetworkContext *network;
    unsigned long blast_sound, impact_sound, ricochet_sound, loop_sound, second_loop_sound;
    unsigned long smoke_image, trail_image;
    unsigned long packets, flashes, particles, random_calls;
    int error;
} BHEffectProjectileContext;
/* Explicit borrowed actual source owners. Private context+608 and generation
   +612 are separate from cleanup's+604 and the original entity fields. The
   real cleanup and query definitions are installed, never original addresses.
   All referenced entities/owners remain stable during synchronous emissions.
   Finite moderate PC53/nearest; original PC24 and extreme trig unvalidated. */
int __cdecl bh_effect_projectile_bind(BHEffectProjectileContext *, BHEntity *, int orbit);
void __cdecl bh_effect_orbit_query(BHEntity *, void *);  /*405ef0*/
void __cdecl bh_effect_linear_query(BHEntity *, void *); /*406300*/
/* Complete source constructor APIs: ordinary borrowed world/entity bindings.
   All installed tick/draw/query callbacks have actual source definitions. */
BHEntity *__cdecl bh_effect_orbit_create(BHEffectProjectileContext *, BHWorldGeometry *, BHEntity *, const BHVec3 *,
                                         const BHVec3 *, const BHVec3 *, const BHVec3 *, const BHVec3 *, float,
                                         BHEntity *);
BHEntity *__cdecl bh_effect_split_create(BHEffectProjectileContext *, BHWorldGeometry *, BHEntity *, const BHVec3 *,
                                         const BHVec3 *, float, int, unsigned long, float);
BHEntity *__cdecl bh_effect_shard_create(BHEffectProjectileContext *, BHWorldGeometry *, BHEntity *, const BHVec3 *,
                                         const BHVec3 *, float, int, unsigned long, float);
BHEntity *__cdecl bh_effect_accelerated_create(BHEffectProjectileContext *, BHWorldGeometry *, BHEntity *,
                                               const BHVec3 *, const BHVec3 *, const BHVec3 *);
#endif
