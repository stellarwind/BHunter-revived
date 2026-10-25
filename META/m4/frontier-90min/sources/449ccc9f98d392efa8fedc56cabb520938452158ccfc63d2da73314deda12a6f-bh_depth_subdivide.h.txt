#ifndef BH_DEPTH_SUBDIVIDE_H
#define BH_DEPTH_SUBDIVIDE_H
#include "bh_clipper.h"
typedef int (__cdecl *BHDepthLeaf)(BHClipVertex *,void *);
typedef enum BHDepthSplitError {BH_DEPTH_SPLIT_OK,BH_DEPTH_SPLIT_ARGUMENT,BH_DEPTH_SPLIT_TOPOLOGY,
    BH_DEPTH_SPLIT_NUMERICAL,BH_DEPTH_SPLIT_CAPACITY,BH_DEPTH_SPLIT_LEAF} BHDepthSplitError;
typedef struct BHDepthSplitContext {
    float left,right,top,bottom;
    BHDepthLeaf leaf;void *user;
    unsigned long splits,leaves,deepest;
    BHDepthSplitError error;
} BHDepthSplitContext;
/* Source binding adapter: live original screen limits and selected raster
   callback004e2ddc are explicit. Does not implement0046dda0 or raster kernels.
   Leaf consumes synchronously; it may not retain or mutate vertices/links or
   recursively enter this context. Caller owns PC53/nearest/masked profile. */
int __cdecl bh_depth_split_init(BHDepthSplitContext *,float left,float right,float top,float bottom,BHDepthLeaf,void *);
/* Complete0046d630 /0046d9d0. Positive finite depth convex reciprocal ring,
   at most1024 source nodes. Split condition is STRICT (max-min)*16 < min+max;
   equality subdivides. Midpoint is stored binary32; nearer half runs first.
   u/v and scalar interpolate in depth; sx/sy use depth-weighted perspective
   interpolation and live inclusive screen clamps. Input fields remain intact;
   every temporary next/previous splice is restored, including leaf failures.
   Plain owner leaves unknown28 unspecified on generated nodes; scalar owner
   treats it as a binary32 value. Source additionally rejects malformed inputs,
   nonprogressing/overflowed arithmetic, recursion>256 and >65536 leaves. */
int __cdecl bh_depth_subdivide(BHDepthSplitContext *,BHClipVertex *); /*0046d630*/
int __cdecl bh_depth_subdivide_scalar(BHDepthSplitContext *,BHClipVertex *); /*0046d9d0*/
#endif
