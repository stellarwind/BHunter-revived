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
/* 0044cc20: intersect one transformed source face with a destination face
   on an explicitly supplied BSP plane. a_to_b and b_to_world are borrowed;
   world_normal is the caller-selected oriented contact normal. Returns 1/0/-1
   for contact/miss/invalid. Strict plane crossings and strict interval overlap
   exclude coplanar faces and endpoint-only contact. Output changes on hit only.
   Does not select faces, filter flags, compute a sweep fraction or deliver damage.
   PC53 finite input contract; malformed/cyclic edge lists reject. */
typedef struct BHFaceContact { BHVec3 raw_point, point; } BHFaceContact;
int __cdecl bh_collision_face_contact(const BHBsp *a, long a_face,
    const BHMat34 *a_to_b, const BHBsp *b, long b_face, long b_node,
    const BHMat34 *b_to_world, const BHVec3 *world_normal, BHFaceContact *out);
/* 0044d500: recheck the cached face pair at a trial world pose. Original
   context globals become explicit inputs; compose to_local * pose before the
   complete face-contact owner. Contact output changes only on a hit. */
int __cdecl bh_collision_cached_contact(const BHBsp *a,long a_face,const BHMat34 *pose,
    const BHBsp *b,long b_face,long b_node,const BHMat34 *to_world,
    const BHMat34 *to_local,const BHVec3 *world_normal,BHFaceContact *out);
#endif
