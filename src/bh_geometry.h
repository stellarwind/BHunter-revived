#ifndef BH_GEOMETRY_H
#define BH_GEOMETRY_H
#include "bh_bsp.h"

typedef struct BHGeometry BHGeometry;
typedef struct BHFaceInfo
{
    long face;
    unsigned long vertices, flags;
    const char *texture;
    BHVec3 point, normal;
    BHGeometry *geometry;
} BHFaceInfo;
typedef void(__cdecl *BHFaceCallback)(BHFaceInfo *face, void *user);

/* Mutable per-instance collision metadata over borrowed, immutable decoded BSP
   data. Dispose before the BSP is replaced/destroyed. Copying this owner or its
   ownership fields is not supported. Scratch has the original persistent-normal
   behavior, with a defined initial zero normal instead of guessed globals. */
struct BHGeometry
{
    const BHBsp *bsp;
    unsigned long *face_flags;
    BHSceneAllocator allocator;
    BHFaceInfo scratch;
    BHBspError error;
    int enumerating;
};
int __cdecl bh_geometry_init(BHGeometry *geometry, const BHBsp *bsp, const BHSceneAllocator *allocator);
int __cdecl bh_geometry_dispose(BHGeometry *geometry);
/* Original 0044e210/0044e230. Invalid index rejects; getter leaves output alone. */
int __cdecl bh_geometry_set_flags(BHGeometry *geometry, long face, unsigned long flags);
int __cdecl bh_geometry_get_flags(BHGeometry *geometry, long face, unsigned long *flags);
/* 0044e240: local vertex ordinal follows next edges, not contiguous vertices. */
int __cdecl bh_geometry_vertex(BHGeometry *geometry, long face, long ordinal, BHVec3 *out);
/* 0044e2a0: initialize out before first use. If topology supplies no normal,
   its previous normal is retained, just as in the original descriptor record.
   Later matching leaves/nodes overwrite earlier normals in original order. */
int __cdecl bh_geometry_face(BHGeometry *geometry, long face, BHFaceInfo *out);
/* 0044e060: borrowed mutable scratch valid only during callback; no mutation of
   topology. Flag updates are allowed. Nested enumeration/disposal is rejected.
   Callbacks are real caller behavior and may not destroy this owner or its BSP. */
int __cdecl bh_geometry_each_face(BHGeometry *geometry, BHFaceCallback callback, void *user);
#endif
