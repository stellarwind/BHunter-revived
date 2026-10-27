#ifndef BH_HORIZON_QUEUE_H
#define BH_HORIZON_QUEUE_H
#include "bh_draw_queue.h"
#include "bh_horizon.h"
/* Complete004784c0. Header-zero insertion precedes every numbered bucket and
   preserves ordering with camera commands. Texture is a borrowed source handle;
   four binary32 parameters and the full color word are copied without numeric
   validation. Unused command words retain their previous storage contents.
   Return1 queued,0 full,-1 invalid source binding/topology.
   Uses source semantic selector instead of original executable callback. */
int __cdecl bh_horizon_queue(BHDrawQueue *, unsigned long texture, float texture_scale, float height,
                             float outer_radius, float inner_radius, unsigned long color);
typedef BHTextureImage *(__cdecl *BHHorizonImageLookup)(unsigned long, void *);
typedef struct BHHorizonQueueContext
{
    BHHorizonContext *draw;
    BHHorizonImageLookup lookup;
    void *lookup_user;
} BHHorizonQueueContext;
/* Complete00477aa0: actual mode0 barrier, full horizon owner, then next load.
   Explicit source handle resolver returns borrowed real texture storage.
   No ring/command/context overlap; lookup may not mutate command storage.
   next is published even on source barrier/draw/lookup failure. */
int __cdecl bh_horizon_queue_execute(BHHorizonQueueContext *, const BHDrawQueue *, const BHDrawCommand *,
                                     unsigned long *next);
#endif
