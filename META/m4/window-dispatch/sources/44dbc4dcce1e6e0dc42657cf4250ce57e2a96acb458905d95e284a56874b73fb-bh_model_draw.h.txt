#ifndef BH_MODEL_DRAW_H
#define BH_MODEL_DRAW_H
#include "bh_model.h"
#include "bh_clipper.h"
#include "bh_view_camera.h"
typedef int (__cdecl *BHModelPolygonEmit)(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user);
typedef enum BHModelDrawError {BH_MODEL_DRAW_OK,BH_MODEL_DRAW_ARGUMENT,BH_MODEL_DRAW_MEMORY,BH_MODEL_DRAW_TOPOLOGY,BH_MODEL_DRAW_CLIP,BH_MODEL_DRAW_EMIT} BHModelDrawError;
typedef struct BHModelDrawTask {long node,leaf,tail;unsigned long mask,phase;} BHModelDrawTask;
typedef struct BHModelDrawCache {
    const BHModel *model;
    BHClipVertex *vertices;
    unsigned long *face_frames;
    BHModelDrawTask *tasks;
    unsigned long vertex_count,face_count,task_capacity;
    BHSceneAllocator allocator;
} BHModelDrawCache;
typedef struct BHModelDrawContext {
    const BHModel *model;BHModelDrawCache *cache;
    const BHViewCamera *view;BHClipContext *clip;
    unsigned long frame,texture_mask,color;
    BHModelPolygonEmit emit;void *user;
    BHModelDrawError error;
    unsigned long nodes_visited,faces_marked,vertices_transformed,polygons_emitted;
} BHModelDrawContext;
/* Explicit working-cache/lifetime adapters. Initialize cache to zero; model
   arrays stay immutable and outlive cache. Original opaque pointer words are
   never interpreted as pointers; face/vertex frame caches start invalid(0).
   Reuse nonzero frame tokens only after resetting cache. Validated acyclic
   BSP model/leaf/edge topology is a caller contract (admitted loader checks it). */
int __cdecl bh_model_draw_cache_init(BHModelDrawCache *,const BHModel *,const BHSceneAllocator *);
void __cdecl bh_model_draw_cache_dispose(BHModelDrawCache *);
void __cdecl bh_model_draw_cache_reset(BHModelDrawCache *);
int __cdecl bh_model_draw_context_init(BHModelDrawContext *,const BHModel *,BHModelDrawCache *,
    const BHViewCamera *,BHClipContext *,unsigned long frame,unsigned long texture_mask,
    unsigned long color,BHModelPolygonEmit,void *user);
/* Complete00458310 /00458430. Original dispatch0046e390 is explicitly bound
   through emit; this is not a claim its software branch is recovered. Ring
   callback consumes synchronously and must not retain ring/cache pointers or
   recursively mutate this context. Callback failure is an adapter error.
   Uses real00452140 clipper including projected-UV image mode. */
int __cdecl bh_model_draw_face(BHModelDrawContext *,long face);
int __cdecl bh_model_draw_leaf_chain(BHModelDrawContext *,long leaf);
/* Complete00458470 unlit traversal over per-model local-space planes/eye.
   AABB touching outside plane is rejected (minimum support>=0); wholly inside
   clears via original16-bit AND masks. BSP order/visible leaf side, recursive
   first child and tail second child preserved with explicit bounded stack.
   Mask affects AABB traversal; face owner clips with real config regardless.
   Caller owns PC53/nearest/masked profile; no game/backend execution here. */
int __cdecl bh_model_draw_unlit(BHModelDrawContext *,long node,unsigned long mask);
#endif
