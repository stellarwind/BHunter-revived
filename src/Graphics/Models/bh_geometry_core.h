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

static_assert(offsetof(BHGeometryCoreView, vertices) == 0x44, "BHGeometryCoreView.vertices offset");
static_assert(offsetof(BHGeometryCoreView, edges) == 0x4c, "BHGeometryCoreView.edges offset");
static_assert(offsetof(BHGeometryCoreView, flags) == 0x78, "BHGeometryCoreView.flags offset");
static_assert(offsetof(BHGeometryCoreView, faces) == 0x7c, "BHGeometryCoreView.faces offset");
static_assert(sizeof(BHBspVertex) == 60, "BHBspVertex size");
static_assert(sizeof(BHBspEdge) == 16, "BHBspEdge size");
static_assert(sizeof(BHBspFace) == 12, "BHBspFace size");

void bh_geometry_set_flags_core(BHGeometryCoreView *, long face, unsigned long flags);
unsigned long bh_geometry_get_flags_core(const BHGeometryCoreView *, long face);
void bh_geometry_vertex_core(const BHGeometryCoreView *, long face, long ordinal, BHVec3 *out);
#endif
