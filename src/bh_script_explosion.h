#ifndef BH_SCRIPT_EXPLOSION_H
#define BH_SCRIPT_EXPLOSION_H
#include "bh_script_debris.h"
#include "bh_flash_flare.h"
#include "bh_sound_queue.h"

typedef enum BHScriptExplosionError {
    BH_EXPLOSION_OK,BH_EXPLOSION_ARGUMENT,BH_EXPLOSION_POOL,
    BH_EXPLOSION_QUERY,BH_EXPLOSION_MUTATION,BH_EXPLOSION_LIMIT,BH_EXPLOSION_QUEUE
} BHScriptExplosionError;
typedef struct BHScriptExplosionContext {
    BHEntityPool *pool;
    BHDrawQueue *queue;
    BHSoundQueue *sound;
    BHScriptDebrisContext *debris;
    BHFlashFlareContext *flash;
    unsigned long sound_resource,image;
    BHScriptExplosionError error;
} BHScriptExplosionContext;
typedef struct BHScriptExplosionState {
    float age,duration;
    BHVec3 direction[8];
    float angle[8],spin[8];
    float radius,magnitude,probability;
    unsigned long sound_resource,origin_tag,target;
} BHScriptExplosionState;
/* Whole constructors00402e50/004032f0. All contexts stay borrowed while their
   entities live; debris->origin_tag is the SAME attribution word004dfb34.
   Sound requests queue voices without playback. A source-returned main entity
   is observational: original constructors returned void. Sound/flash survive
   main allocation failure. Success uses48 lobe draws, one debris-count draw,
   then nine draws per successful actual debris entity. Finite constructor
   inputs, nonnegative magnitude and PC53 nearest/masked are supported. Debris
   counts outside0..65535 report LIMIT after the main/sound/flash/blast effects;
   they do not suppress a finite secondary explosion or its damage broadcasts.
   Query callbacks may remove entities: released/recycled current slots stop
   the remaining source accesses with MUTATION instead of original stale use. */
BHEntity *__cdecl bh_script_explosion_create(BHScriptExplosionContext *,const BHVec3 *,
    float duration,float magnitude,float probability,unsigned long target);
BHEntity *__cdecl bh_script_explosion_secondary(BHScriptExplosionContext *,const BHVec3 *,
    float duration,float magnitude,float probability,unsigned long sound,unsigned long target);
void __cdecl bh_script_explosion_tick(BHEntity *); /*00402af0*/
void __cdecl bh_script_explosion_refresh(BHEntity *,void *); /*00402710*/
int __cdecl bh_script_explosion_read(const BHEntity *,BHScriptExplosionState *);
int __cdecl bh_script_explosion_write(BHEntity *,const BHScriptExplosionState *);
#endif
