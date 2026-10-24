#ifndef BH_COLLISION_MODEL_H
#define BH_COLLISION_MODEL_H
#include "bh_geometry.h"

typedef struct BHCollisionModel {
    BHBsp bsp;
    BHGeometry geometry;
} BHCollisionModel;

/* Fresh explicit owner. Do not copy; query/face consumers borrow its geometry.
   Dispose queries before replacing this model. Input decode bytes must not
   overlap its storage. Serialized face runtime words are reset to zero in the
   model's own copy; original files and caller input remain immutable. */
int __cdecl bh_collision_model_init(BHCollisionModel *model, const BHSceneAllocator *allocator);
int __cdecl bh_collision_model_dispose(BHCollisionModel *model);
int __cdecl bh_collision_model_decode(BHCollisionModel *model, const void *bytes,
                                      unsigned long size);
/* Complete 0044b2e0 loading behavior over explicit source owners: real VFILE
   search/open/read/close; all six sections; zeroed face flags/runtime metadata;
   node centers/squared radii/radii. Path/search and bounded PC53 data contracts
   are bh_bsp_load's. Failure retains the previous model and frees partial work.
   Geometry must not be in a callback during replacement. No renderer or texture
   object is part of this collision-only original routine. */
int __cdecl bh_collision_model_load(BHCollisionModel *model, const char *name, const char *prefix,
                                    BHBspSearch search);
#endif
