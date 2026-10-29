#ifndef BH_COLLISION_CORE_MODEL_H
#define BH_COLLISION_CORE_MODEL_H
#include "bh_bsp.h"
#include <stddef.h>

/* Source view of the verified original collision fields. Reserved bytes are
   never read. Pointers bind actual source-owned arrays; this is not an owning
   replacement for the complete original model object. */
typedef struct BHCollisionCoreModel
{
    unsigned char before_vertices[0x44];
    const BHBspVertex *vertices;
    unsigned char before_edges[4];
    const BHBspEdge *edges;
    long face_count;
    const BHBspFace *source_faces;
    long node_count;
    const BHBspNode *nodes;
    long leaf_count;
    const BHBspLeaf *leaves;
    unsigned char before_centers[4];
    BHVec3 *centers;
    float *radii, *squared_radii;
    unsigned long *flags;
    BHBspFace *faces;
} BHCollisionCoreModel;

typedef char collision_core_vertices[offsetof(BHCollisionCoreModel, vertices) == 0x44 ? 1 : -1];
typedef char collision_core_edges[offsetof(BHCollisionCoreModel, edges) == 0x4c ? 1 : -1];
typedef char collision_core_face_count[offsetof(BHCollisionCoreModel, face_count) == 0x50 ? 1 : -1];
typedef char collision_core_source_faces[offsetof(BHCollisionCoreModel, source_faces) == 0x54 ? 1 : -1];
typedef char collision_core_node_count[offsetof(BHCollisionCoreModel, node_count) == 0x58 ? 1 : -1];
typedef char collision_core_nodes[offsetof(BHCollisionCoreModel, nodes) == 0x5c ? 1 : -1];
typedef char collision_core_leaves[offsetof(BHCollisionCoreModel, leaves) == 0x64 ? 1 : -1];
typedef char collision_core_centers[offsetof(BHCollisionCoreModel, centers) == 0x6c ? 1 : -1];
typedef char collision_core_radii[offsetof(BHCollisionCoreModel, radii) == 0x70 ? 1 : -1];
typedef char collision_core_squared_radii[offsetof(BHCollisionCoreModel, squared_radii) == 0x74 ? 1 : -1];
typedef char collision_core_flags[offsetof(BHCollisionCoreModel, flags) == 0x78 ? 1 : -1];
typedef char collision_core_faces[offsetof(BHCollisionCoreModel, faces) == 0x7c ? 1 : -1];
typedef char collision_core_vertex_stride[sizeof(BHBspVertex) == 60 ? 1 : -1];
typedef char collision_core_edge_stride[sizeof(BHBspEdge) == 16 ? 1 : -1];
typedef char collision_core_face_stride[sizeof(BHBspFace) == 12 ? 1 : -1];
typedef char collision_core_node_stride[sizeof(BHBspNode) == 56 ? 1 : -1];
typedef char collision_core_leaf_stride[sizeof(BHBspLeaf) == 8 ? 1 : -1];
#endif
