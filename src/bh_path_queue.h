#ifndef BH_PATH_QUEUE_H
#define BH_PATH_QUEUE_H
#include "bh_draw_submit.h"
#include "bh_view_cull.h"

typedef enum BHPathQueueError
{
    BH_PATH_QUEUE_OK,
    BH_PATH_QUEUE_ARGUMENT,
    BH_PATH_QUEUE_TOPOLOGY,
    BH_PATH_QUEUE_NUMERICAL,
    BH_PATH_QUEUE_CAPACITY,
    BH_PATH_QUEUE_IMAGE,
    BH_PATH_QUEUE_DRAW
} BHPathQueueError;
typedef struct BHPathQueueContext
{
    const BHViewConfig *view;
    const BHMainCamera *camera;
    BHSpriteOrientedContext *draw;
    BHDrawImageLookup lookup;
    void *lookup_user;
    unsigned long accepted, culled, deepest;
    BHPathQueueError error;
} BHPathQueueContext;
/* Complete00455ee0: append to the actual borrowed ribbon pool, transform by
   the real camera, preserve corner scratch and link previous slot. Increments
   used without resetting it. Source capacity/finite guards expose failure;
   callers retain all previously appended source vertices on failure. */
int __cdecl bh_path_queue_append(BHPathQueueContext *, const BHVec3 *point, float width, unsigned long flags,
                                 float texture_start, float texture_end);
/* Complete00477880 adaptive path selection. points supplies spans+1 contiguous
   BH_DRAW_PATH_POINT records. One span bypasses culling; zero span uses the
   original visible zero-radius acceptance under a positive near plane. The
   sphere radius is the stored difference of point words5; midpoint stored
   before cull, adjusted depth/ratio retained, threshold STRICT >16. Negative
   returned cull masks reject. Both recursive children inherit the returned
   mask. Returns number appended, -1 for an explicit source binding error.
   Finite points, nonincreasing words5 and positive view near_z are required. */
long __cdecl bh_path_queue_select(BHPathQueueContext *, const BHDrawCommand *points, unsigned long spans,
                                  unsigned long plane_mask);
/* Complete004779e0 owner. Header and its declared contiguous points belong to
   queue; linked first point equals the next physical record. Draws with the
   real recovered ribbon owner after selecting two independently masked halves
   and appending the final point. It then reads the LAST point's current next
   link, including source draw/lookup failure. Count0 is diagnosed (original
   negative-span/OOB domain); valid count1/2 semantics remain supported. Queue,
   image, camera and pool lifetimes are borrowed; next must not alias them.
   The real ribbon draw owns its successful used-count reset. No original code,
   fake callback or software rasterizer is supplied. PC53 finite profile. */
int __cdecl bh_path_queue_execute(BHPathQueueContext *, const BHDrawQueue *, const BHDrawCommand *header,
                                  unsigned long *next);
#endif
