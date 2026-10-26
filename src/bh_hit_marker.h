#ifndef BH_HIT_MARKER_H
#define BH_HIT_MARKER_H
#include "bh_entities.h"
#include "bh_quad_queue.h"
typedef enum BHHitMarkerError {BH_HIT_MARKER_OK,BH_HIT_MARKER_ARGUMENT,BH_HIT_MARKER_POOL,BH_HIT_MARKER_QUEUE,BH_HIT_MARKER_TOPOLOGY} BHHitMarkerError;
typedef struct BHHitMarkerContext {
    BHEntityPool *pool;
    BHDrawQueue *queue;
    unsigned long image,count;
    BHEntity *oldest,*newest;
    BHHitMarkerError error;
} BHHitMarkerContext;
typedef void (__cdecl *BHHitMarkerDraw)(BHEntity *);
/* Whole 00407b40 / 00407a20. Context must start zeroed, bind the real pool,
   queue and original 004f0084 texture identity. All owner entities and this
   context outlive their markers; dispose before releasing owners or pool.
   Original owner+12c and marker+ec..fc are intrusive lists, not pool links.
   Query20 hides size without unlinking; eviction remains global FIFO at200.
   Draw is installed at marker+e8; refresh/step are verified RET sentinels.
   The private borrowed source context occupies payload+624. PC53/masked FP.
   No original code, model stub or backend implementation is supplied here. */
BHEntity *__cdecl bh_hit_marker_create(BHHitMarkerContext *,const BHVec3 *position,
    const BHVec3 *normal,float size,unsigned long atlas_flags,BHEntity *owner);
void __cdecl bh_hit_marker_draw(BHEntity *);
/* Count-zero lifetime adapter. Unlinks the same two lists and releases real
   pooled slots; callers must not independently release active markers. */
int __cdecl bh_hit_marker_dispose(BHHitMarkerContext *);
#endif
