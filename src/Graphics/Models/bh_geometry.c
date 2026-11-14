#include "bh_geometry.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

static_assert(sizeof(BHFaceInfo) == 44 && offsetof(BHFaceInfo, normal) == 28, "face info layout");
/* 0044e060 publishes one process-persistent descriptor at004e0f98. Faces
   without a node normal retain its previous normal, even across owners. */
static BHFaceInfo enumeration_descriptor;
#include "bh_geometry_core.inc"
static void *allocate(unsigned long n, void *user)
{
    return malloc(n);
}
static void release(void *p, void *user)
{
    free(p);
}
static int fail(BHGeometry *geometry, BHBspError e)
{
    if (geometry)
        geometry->error = e;
    return 0;
}
static int face_index(BHGeometry *geometry, long face)
{
    return geometry && geometry->bsp && geometry->face_flags && face >= 0 && (unsigned long)face < geometry->bsp->counts[3];
}

int bh_geometry_init(BHGeometry *g, const BHBsp *bsp, const BHSceneAllocator *allocator)
{
    BHGeometry next;
    if (!g || !bsp || !bsp->storage || !bsp->faces || !bsp->edges || !bsp->vertices || !bsp->nodes || !bsp->leaves ||
        !bsp->materials || !bsp->counts[3] || bsp->counts[3] > 65535 ||
        (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(&next, 0, sizeof(next));
    if (allocator)
        next.allocator = *allocator;
    else
    {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    next.face_flags = (unsigned long *)next.allocator.allocate(bsp->counts[3] * 4, next.allocator.user);
    if (!next.face_flags)
        return 0;
    /* The original collision loader initializes its separate +78 array to 0;
       these are runtime face flags, not the serialized face's third word. */
    memset(next.face_flags, 0, bsp->counts[3] * 4);
    next.bsp = bsp;
    *g = next;
    return 1;
}
int bh_geometry_dispose(BHGeometry *geometry)
{
    if (!geometry || geometry->enumerating)
        return fail(geometry, BH_BSP_ARGUMENT);
    if (geometry->face_flags)
        geometry->allocator.release(geometry->face_flags, geometry->allocator.user);
    memset(geometry, 0, sizeof(*geometry));
    return 1;
}
int bh_geometry_set_flags(BHGeometry *geometry, long face, unsigned long flags)
{
    BHGeometryCoreView view;
    if (!face_index(geometry, face))
        return fail(geometry, BH_BSP_ARGUMENT);
    view.flags = geometry->face_flags;
    bh_geometry_set_flags_core(&view, face, flags);
    geometry->error = BH_BSP_OK;
    return 1;
}
int bh_geometry_get_flags(BHGeometry *geometry, long face, unsigned long *flags)
{
    BHGeometryCoreView view;
    if (!face_index(geometry, face) || !flags)
        return fail(geometry, BH_BSP_ARGUMENT);
    view.flags = geometry->face_flags;
    *flags = bh_geometry_get_flags_core(&view, face);
    geometry->error = BH_BSP_OK;
    return 1;
}
int bh_geometry_vertex(BHGeometry *geometry, long face, long ordinal, BHVec3 *out)
{
    const BHBsp *bsp;
    BHGeometryCoreView view;
    long requested = ordinal;
    long edge;
    unsigned long steps = 0;
    if (!face_index(geometry, face) || ordinal < 0 || !out)
        return fail(geometry, BH_BSP_ARGUMENT);
    bsp = geometry->bsp;
    edge = bsp->faces[face].first_edge;
    while (ordinal && edge >= 0 && (unsigned long)edge < bsp->counts[2] && steps++ < bsp->counts[2])
    {
        edge = bsp->edges[edge].next;
        --ordinal;
    }
    if (ordinal || edge < 0 || (unsigned long)edge >= bsp->counts[2] || bsp->edges[edge].vertex < 0 ||
        (unsigned long)bsp->edges[edge].vertex >= bsp->counts[1])
        return fail(geometry, BH_BSP_FORMAT);
    view.vertices = bsp->vertices;
    view.edges = bsp->edges;
    view.faces = bsp->faces;
    bh_geometry_vertex_core(&view, face, requested, out);
    geometry->error = BH_BSP_OK;
    return 1;
}
int bh_geometry_face(BHGeometry *geometry, long face, BHFaceInfo *out)
{
    const BHBsp *bsp;
    BHFaceInfo next;
    long edge, root;
    unsigned long i, j, steps;
    int found;
    if (!face_index(geometry, face) || !out)
        return fail(geometry, BH_BSP_ARGUMENT);
    bsp = geometry->bsp;
    next = *out;
    next.face = face;
    next.vertices = 0;
    next.geometry = geometry;
    next.flags = geometry->face_flags[face];
    if (bsp->faces[face].material < 0 || (unsigned long)bsp->faces[face].material >= bsp->counts[0])
        return fail(geometry, BH_BSP_FORMAT);
    next.texture = bsp->materials[bsp->faces[face].material].texture;
    edge = bsp->faces[face].first_edge;
    while (edge >= 0)
    {
        if ((unsigned long)edge >= bsp->counts[2] || next.vertices++ >= bsp->counts[2])
            return fail(geometry, BH_BSP_CYCLE);
        edge = bsp->edges[edge].next;
    }
    if (!bh_geometry_vertex(geometry, face, 0, &next.point))
        return 0;
    /* Preserve exact scan/overwrite order, including the first predecessor
       when next chains share a tail. The decoded BSP owns acyclic chains. */
    for (i = 0; i < bsp->counts[5]; ++i)
        if (bsp->leaves[i].face == face)
        {
            root = (long)i;
            steps = 0;
            do
            {
                found = 0;
                for (j = 0; j < bsp->counts[5]; ++j)
                    if (bsp->leaves[j].next == root)
                    {
                        root = (long)j;
                        found = 1;
                        break;
                    }
                if (found && ++steps > bsp->counts[5])
                    return fail(geometry, BH_BSP_CYCLE);
            } while (found);
            for (j = 0; j < bsp->counts[4]; ++j)
            {
                const BHBspNode *bsp_node = bsp->nodes + j;
                if (bsp_node->front_leaf == root)
                    memcpy(&next.normal, bsp_node->normal, 12);
                else if (bsp_node->back_leaf == root)
                {
                    next.normal.x = -bsp_node->normal[0];
                    next.normal.y = -bsp_node->normal[1];
                    next.normal.z = -bsp_node->normal[2];
                }
            }
        }
    *out = next;
    geometry->error = BH_BSP_OK;
    return 1;
}
int bh_geometry_each_face(BHGeometry *geometry, BHFaceCallback callback, void *user)
{
    unsigned long i;
    int active;
    if (!geometry || !geometry->bsp || !callback)
        return fail(geometry, BH_BSP_ARGUMENT);
    /* 0044e060 keeps its loop/model state on the caller's stack. Nested
       enumeration shares the descriptor and must preserve the outer lifetime. */
    active = geometry->enumerating;
    geometry->enumerating = 1;
    for (i = 0; i < geometry->bsp->counts[3]; ++i)
    {
        if (!bh_geometry_face(geometry, (long)i, &enumeration_descriptor))
        {
            geometry->enumerating = active;
            return 0;
        }
        callback(&enumeration_descriptor, user);
    }
    geometry->enumerating = active;
    geometry->error = BH_BSP_OK;
    return 1;
}
