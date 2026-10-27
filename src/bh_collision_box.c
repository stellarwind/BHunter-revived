#include "bh_collision_box.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>
#include <stddef.h>
/* Source representation adapter: same six typed sections consumed by the real
   validated collision owner. Each section count is immediately before data. */
typedef struct BHBoxPayload
{
    unsigned long material_count;
    BHBspMaterial materials[2];
    unsigned long vertex_count;
    BHBspVertex vertices[8];
    unsigned long edge_count;
    BHBspEdge edges[24];
    unsigned long face_count;
    BHBspFace faces[6];
    unsigned long node_count;
    BHBspNode nodes[6];
    unsigned long leaf_count;
    BHBspLeaf leaves[6];
} BHBoxPayload;
typedef char box_payload_size[sizeof(BHBoxPayload) == 1408 ? 1 : -1];
typedef char box_vertices[offsetof(BHBoxPayload, vertices) == 72 ? 1 : -1];
typedef char box_edges[offsetof(BHBoxPayload, edges) == 556 ? 1 : -1];
typedef char box_faces[offsetof(BHBoxPayload, faces) == 944 ? 1 : -1];
typedef char box_nodes[offsetof(BHBoxPayload, nodes) == 1020 ? 1 : -1];
typedef char box_leaves[offsetof(BHBoxPayload, leaves) == 1360 ? 1 : -1];
int __cdecl bh_collision_box_build(BHCollisionModel *model, const BHVec3 *extent, const BHVec3 *padding)
{
    static const unsigned char face_vertices[6][4] = {{0, 4, 6, 2}, {3, 7, 5, 1}, {1, 5, 4, 0},
                                                      {2, 6, 7, 3}, {2, 3, 1, 0}, {4, 5, 7, 6}};
    static const float uv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    BHBoxPayload payload;
    float low[3], high[3];
    const float *size, *pad;
    unsigned long i, j, stamp = 0xffffffffUL;
    if (!model)
        return 0;
    if (!extent || !padding || !model->bsp.allocator.allocate || model->geometry.enumerating)
    {
        model->bsp.error = BH_BSP_ARGUMENT;
        return 0;
    }
    if (!bh_math_environment_supported())
    {
        model->bsp.error = BH_BSP_NUMERICAL;
        return 0;
    }
    size = &extent->x;
    pad = &padding->x;
    for (i = 0; i < 3; ++i)
    {
        if (!_finite(size[i]) || !_finite(pad[i]) || size[i] < 0)
        {
            model->bsp.error = BH_BSP_ARGUMENT;
            return 0;
        }
        bh_round_float(&low[i], -(double)pad[i]);
        bh_round_float(&high[i], (double)size[i] - pad[i]);
    }
    memset(&payload, 0, sizeof(payload));
    payload.material_count = 2;
    payload.vertex_count = 8;
    payload.edge_count = 24;
    payload.face_count = payload.node_count = payload.leaf_count = 6;
    strcpy(payload.materials[0].texture, "default");
    for (i = 0; i < 8; ++i)
    {
        payload.vertices[i].x = (i & 1) ? high[0] : low[0];
        payload.vertices[i].y = (i & 2) ? high[1] : low[1];
        payload.vertices[i].z = (i & 4) ? high[2] : low[2];
        memcpy((unsigned char *)&payload.vertices[i] + 0x34, &stamp, 4);
    }
    for (i = 0; i < 6; ++i)
    {
        BHBspNode *node = payload.nodes + i;
        unsigned long axis = i / 2;
        payload.faces[i].material = 1;
        payload.faces[i].first_edge = i * 4;
        payload.leaves[i].face = i;
        payload.leaves[i].next = -1;
        for (j = 0; j < 4; ++j)
        {
            BHBspEdge *edge = payload.edges + i * 4 + j;
            edge->vertex = face_vertices[i][j];
            edge->next = j == 3 ? -1 : (long)(i * 4 + j + 1);
            edge->u = uv[j][0];
            edge->v = uv[j][1];
        }
        memcpy(node->minimum, low, 12);
        memcpy(node->maximum, high, 12);
        node->normal[axis] = (i & 1) ? 1.0f : -1.0f;
        node->plane = -((i & 1) ? high[axis] : low[axis]);
        node->front_leaf = i;
        node->back_leaf = node->front_node = -1;
        node->back_node = i == 5 ? -1 : (long)i + 1;
    }
    /* Real decode owns source arrays, preserves float-spill midpoint/radii,
    clears final face runtime/flags and binds actual geometry consumers. Its
    atomic replacement/free behavior is the explicit source failure policy. */
    return bh_collision_model_decode(model, &payload, sizeof(payload));
}
