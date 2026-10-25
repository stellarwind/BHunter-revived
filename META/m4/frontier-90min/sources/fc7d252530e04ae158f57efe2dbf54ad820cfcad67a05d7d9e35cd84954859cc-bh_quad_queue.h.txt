#ifndef BH_QUAD_QUEUE_H
#define BH_QUAD_QUEUE_H
#include "bh_draw_submit.h"
#include "bh_sprite_quad.h"
/* Complete 00478730. Raw binary32 words are copied without normalizing them.
   The explicit source bucket API admits0..8; invalid queue/bindings return-1,
   exhaustion0, queued1. Handle is resolved at execution, borrowed meanwhile.
   The original maps other bucket values to header0; source bounds guards do
   not claim its malformed selector behavior. */
int __cdecl bh_quad_queue(BHDrawQueue *,unsigned long bucket,unsigned long image,
    const BHVec3 *center,const BHVec3 *up,const BHVec3 *normal,
    float width,float height,float angle,unsigned long flags,float modulation,
    float u,float u_extent,float v,float v_extent);
typedef struct BHQuadQueueContext {
    BHSpriteQuadContext *draw;
    BHDrawImageLookup lookup;
    void *lookup_user;
} BHQuadQueueContext;
/* Complete 00477760. Truncates flags to its low byte, invokes the real quad
   owner, then publishes the current next link. No command pointer is retained.
   Source lookup failure also publishes next and reports failure. Lookup must
   not mutate the command. next must not alias command/context storage. */
int __cdecl bh_quad_queue_execute(BHQuadQueueContext *,const BHDrawCommand *,unsigned long *next);
#endif
