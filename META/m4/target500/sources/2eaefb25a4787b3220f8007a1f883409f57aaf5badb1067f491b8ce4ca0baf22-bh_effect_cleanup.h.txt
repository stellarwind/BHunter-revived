#ifndef BH_EFFECT_CLEANUP_H
#define BH_EFFECT_CLEANUP_H
#include "bh_entities.h"
#include "bh_sound_queue.h"
typedef struct BHEffectCleanupContext {
  BHEntityPool *pool;
  BHSoundQueue *sound;
} BHEffectCleanupContext;
/* Count-zero lifetime binding for actual original sound word+128. The source
   context at private payload+604 must outlive the entity. Installs the complete
   cleanup owner; it does not claim restoration of constructor00403870. */
int __cdecl bh_effect_cleanup_bind(BHEffectCleanupContext *, BHEntity *,
                                   unsigned long handle);
void __cdecl bh_effect_sound_cleanup(BHEntity *); /*whole00403850*/
#endif
