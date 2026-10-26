#ifndef BH_EFFECT_PUFF_H
#define BH_EFFECT_PUFF_H
#include "bh_draw_queue.h"
#include "bh_entities.h"
typedef struct BHEffectPuffContext {BHDrawQueue *queue;unsigned long attempted,queued;int error;} BHEffectPuffContext;
/* Count-zero binding. Private borrowed context at payload+624 is independent
   of delayed-expiry's payload+608. Original entity fields remain authoritative.
   Both draw owners leave the entity seed and all original fields unchanged. */
int __cdecl bh_effect_puff_bind(BHEffectPuffContext *,BHEntity *,int ring);
void __cdecl bh_effect_puff_random_draw(BHEntity *,void *); /*00402830*/
void __cdecl bh_effect_puff_ring_draw(BHEntity *,void *); /*004029d0*/
#endif
