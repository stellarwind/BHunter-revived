#ifndef BH_HOMING_PROJECTILE_H
#define BH_HOMING_PROJECTILE_H
#include "bh_projectile_lifetime.h"
#include "bh_world_segment.h"
#include "bh_hit_marker.h"
#include "bh_script_particles.h"
#include "bh_contact_effects.h"
#include "bh_flash_flare.h"
typedef struct BHHomingWorldResponse
{
    BHWorldGeometry *world;
    BHEntity *receiver;
} BHHomingWorldResponse;
typedef struct BHHomingProjectileContext
{
    BHEntityPool *pool;
    BHWorldSegmentContext *segment;
    BHSoundQueue *sound;
    BHScriptParticleContext *particles;
    BHContactEffects *contacts;
    BHFlashFlareContext *flash;
    BHHitMarkerContext *markers;
    const BHHomingWorldResponse *responses;
    unsigned long response_count;
    unsigned long hit_sound, liquid_sound, first_image, second_image;
    unsigned long guidance, proximity, world_hits, damage_packets;
    int error;
} BHHomingProjectileContext;
/* Count-zero binding over an actual acquired projectile. Original fields e8,
   100..108,11c,124..130 remain in its real entity storage. Lifetime/query owner
   bh_projectile_lifetime_bind may be installed separately and remains intact.
   Context/geometry/room lists/receiver entities and all subsystems remain stable
   through callbacks. Owners must not recycle another live projectile or target
   from callbacks. World-response mappings bind actual query entities; NULL is
   an explicitly absent RET receiver. All hit worlds require one mapping.
   No constructor, gameplay loop, invented vtable or original code is supplied. */
int __cdecl bh_homing_projectile_bind(BHHomingProjectileContext *, BHEntity *);
/* Whole0043f630: predictive steering, source segment/proximity checks,
   liquid/solid contact effects, marker, damage and pooled retirement. */
void __cdecl bh_homing_projectile_tick(BHEntity *);
#endif
