#ifndef BH_EFFECT_PUFF_H
#define BH_EFFECT_PUFF_H
#include "../../Graphics/bh_draw_queue.h"
#include "../Entities/bh_entities.h"
typedef struct BHEffectPuffContext
{
    BHDrawQueue *queue;
    unsigned long attempted, queued;
    int error;
} BHEffectPuffContext;
/* Count-zero binding. Private borrowed context at payload+624 is independent
   of delayed-expiry's payload+608. Original entity fields remain authoritative.
   Both draw owners leave the entity seed and all original fields unchanged. */
int bh_effect_puff_bind(BHEffectPuffContext *, BHEntity *, int ring);
void bh_effect_puff_random_draw(BHEntity *, void *); /*00402830*/
void bh_effect_puff_ring_draw(BHEntity *, void *);   /*004029d0*/
#endif
