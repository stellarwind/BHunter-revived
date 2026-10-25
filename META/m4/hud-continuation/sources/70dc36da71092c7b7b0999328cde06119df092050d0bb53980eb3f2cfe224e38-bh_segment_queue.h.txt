#ifndef BH_SEGMENT_QUEUE_H
#define BH_SEGMENT_QUEUE_H
#include "bh_draw_submit.h"

typedef enum BHSegmentQueueError {
    BH_SEGMENT_QUEUE_OK,BH_SEGMENT_QUEUE_ARGUMENT,BH_SEGMENT_QUEUE_CAPACITY,
    BH_SEGMENT_QUEUE_NUMERICAL,BH_SEGMENT_QUEUE_IMAGE,BH_SEGMENT_QUEUE_DRAW
} BHSegmentQueueError;
typedef struct BHSegmentQueueContext {
    const BHMainCamera *camera;
    BHSpriteOrientedContext *draw;
    BHDrawImageLookup lookup;void *lookup_user;
    unsigned long accepted;
    BHSegmentQueueError error;
} BHSegmentQueueContext;
/* Complete00455e40. Appends to the actual shared ribbon pool, links the previous
   slot, transforms by the current projected camera, and sets longitudinal UV
   to the signed stored used-count minus one. Existing corner scratch survives.
   Capacity and finite guards expose original out-of-domain inputs. Borrowed
   point/camera/pool must not overlap. PC53 nearest/masked finite profile. */
int __cdecl bh_segment_queue_append(BHSegmentQueueContext *,const BHVec3 *,
    float width,unsigned long flags,float modulation);
/* Complete004777c0, the actual consumer installed by00478390 bh_draw_segment.
   Both endpoint appends precede image lookup/draw. Real004569b0 owns near-plane
   clipping and successful pool reset. Earlier appends remain on source failure.
   Reads the command's current next link AFTER synchronous draw/lookup, also on
   failure. Queue/image/camera/pool survive consumption; next cannot alias them.
   No original executable callback is used or installed. */
int __cdecl bh_segment_queue_execute(BHSegmentQueueContext *,
    const BHDrawCommand *,unsigned long *next);
#endif
