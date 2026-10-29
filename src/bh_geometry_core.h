#ifndef BH_GEOMETRY_CORE_H
#define BH_GEOMETRY_CORE_H
#include "bh_geometry.h"
#include <stddef.h>

/* Original collision-model pointer fields only. Every pointer borrows live
   decoded source storage; unrepresented original fields are never inspected. */
typedef struct BHGeometryCoreView
{
    unsigned char before_vertices[0x44];
    const BHBspVertex *vertices;
    unsigned char before_edges[4];
    const BHBspEdge *edges;
    unsigned char before_flags[0x28];
    unsigned long *flags;
    const BHBspFace *faces;
} BHGeometryCoreView;

typedef char geometry_core_vertices[offsetof(BHGeometryCoreView, vertices) == 0x44 ? 1 : -1];
typedef char geometry_core_edges[offsetof(BHGeometryCoreView, edges) == 0x4c ? 1 : -1];
typedef char geometry_core_flags[offsetof(BHGeometryCoreView, flags) == 0x78 ? 1 : -1];
typedef char geometry_core_faces[offsetof(BHGeometryCoreView, faces) == 0x7c ? 1 : -1];
typedef char geometry_core_vertex_stride[sizeof(BHBspVertex) == 60 ? 1 : -1];
typedef char geometry_core_edge_stride[sizeof(BHBspEdge) == 16 ? 1 : -1];
typedef char geometry_core_face_stride[sizeof(BHBspFace) == 12 ? 1 : -1];

void __cdecl bh_geometry_set_flags_core(BHGeometryCoreView *, long face, unsigned long flags);
unsigned long __cdecl bh_geometry_get_flags_core(const BHGeometryCoreView *, long face);
void __cdecl bh_geometry_vertex_core(const BHGeometryCoreView *, long face, long ordinal, BHVec3 *out);
#endif
