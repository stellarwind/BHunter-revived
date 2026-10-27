#ifndef BH_ENTITY_RESOURCE_DRAW_H
#define BH_ENTITY_RESOURCE_DRAW_H

#include "bh_entities.h"
#include "bh_draw_submit.h"

/* Borrowed bindings for the type-9, variant-0 entity draw method at 004413b0.
   Its original callback pointer lives at entity +e8, inside the payload; this
   module does not install a generic entity refresh callback. */
typedef struct BHEntityResourceDrawContext
{
    BHDrawQueue *queue;
    unsigned long glow_image;
} BHEntityResourceDrawContext;

/* Queue two model resources followed by a billboard, in original call order.
   Returns 1 when all three append, 0 on partial/full queue, -1 for an invalid
   source binding. The queue retains successful appends after saturation.
   Original floating-point inputs and resource handles are passed through. */
int __cdecl bh_entity_resource_draw_special(BHEntityResourceDrawContext *context, BHEntity *entity);

#endif
