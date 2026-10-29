#include "bh_geometry_core.h"

/* Complete 0044e210 and 0044e230. Checked source adapters own index checks. */
void __cdecl bh_geometry_set_flags_core(BHGeometryCoreView *geometry, long face, unsigned long flags)
{
    geometry->flags[face] = flags;
}

unsigned long __cdecl bh_geometry_get_flags_core(const BHGeometryCoreView *geometry, long face)
{
    return geometry->flags[face];
}

/* Complete 0044e240 on a checked, live edge chain. Keep this in the owning
   geometry translation unit: its VC5 context reproduces all 87 original bytes. */
void __cdecl bh_geometry_vertex_core(const BHGeometryCoreView *geometry, long face, long ordinal, BHVec3 *out)
{
    long edge = geometry->faces[face].first_edge;
    while (ordinal--)
        edge = geometry->edges[edge].next;
    *out = *(const BHVec3 *)(geometry->vertices + geometry->edges[edge].vertex);
}

