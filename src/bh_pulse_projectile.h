#ifndef BH_PULSE_PROJECTILE_H
#define BH_PULSE_PROJECTILE_H
#include "bh_pulse_visual.h"
#include "bh_homing_projectile.h"
#include "bh_script_explosion.h"
typedef struct BHPulseProjectileContext
{
    BHHomingProjectileContext *effects;
    BHProjectileLifetime *lifetime;
    BHPulseVisualContext *draw;
    BHScriptDebrisContext *debris;
    BHScriptExplosionContext *explosion;
    unsigned long *identity;
    unsigned long loop_sound, bounce_sound;
    unsigned long explosions, bounces, liquid_hits;
    int error;
} BHPulseProjectileContext;
BHEntity *__cdecl bh_pulse_projectile_create(BHPulseProjectileContext *, BHWorldGeometry *, BHEntity *, const BHVec3 *,
                                             const BHVec3 *, const BHVec3 *); /*00440ad0*/
void __cdecl bh_pulse_projectile_tick(BHEntity *);                            /*00440050*/
#endif
