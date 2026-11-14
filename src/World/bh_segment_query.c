#include "bh_segment_query.h"
#include "../Math/bh_fp.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <string.h>

typedef struct Frame
{
    long node;
    int faces;
} Frame;

static int fail(BHSegmentQuery *query, BHBspError error)
{
    if (query)
        query->error = error;
    return -1;
}
int bh_segment_query_init(BHSegmentQuery *q, BHGeometry *geometry)
{
    BHSegmentQuery next;
    if (!q || !geometry || !geometry->bsp || !geometry->face_flags || geometry->bsp->counts[4] > 65535 ||
        !geometry->allocator.allocate || !geometry->allocator.release)
        return 0;
    memset(&next, 0, sizeof(next));
    next.geometry = geometry;
    next.allocator = geometry->allocator;
    next.capacity = 2 * geometry->bsp->counts[4] + 1;
    next.frames = next.allocator.allocate(next.capacity * sizeof(Frame), next.allocator.user);
    if (!next.frames)
        return 0;
    *q = next;
    return 1;
}
int bh_segment_query_dispose(BHSegmentQuery *query)
{
    if (!query || query->busy)
        return 0;
    if (query->frames)
        query->allocator.release(query->frames, query->allocator.user);
    memset(query, 0, sizeof(*query));
    return 1;
}
static double distance(const BHBspNode *bsp_node, const BHVec3 *point)
{
    return (((double)bsp_node->normal[0] * point->x + (double)bsp_node->normal[1] * point->y) + (double)bsp_node->normal[2] * point->z) + bsp_node->plane;
}
static int polygon(BHSegmentQuery *query, long face, const BHVec3 *point, const BHVec3 *normal)
{
    const BHBsp *bsp = query->geometry->bsp;
    long first, edge, next;
    const BHBspVertex *first_bsp_vertex, *second_bsp_vertex;
    BHVec3 delta, relative, cross;
    unsigned long signs = 0, steps = 0;
    if (face < 0 || (unsigned long)face >= bsp->counts[3])
        return -1;
    if (query->geometry->face_flags[face] & 1)
        return 0;
    first = edge = bsp->faces[face].first_edge;
    if (first < 0 || (unsigned long)first >= bsp->counts[2])
        return -1;
    do
    {
        if (++steps > bsp->counts[2] || edge < 0 || (unsigned long)edge >= bsp->counts[2])
            return -1;
        next = bsp->edges[edge].next;
        if (bsp->edges[edge].vertex < 0 || (unsigned long)bsp->edges[edge].vertex >= bsp->counts[1])
            return -1;
        first_bsp_vertex = bsp->vertices + bsp->edges[edge].vertex;
        if (next >= 0 && (unsigned long)next >= bsp->counts[2])
            return -1;
        if (bsp->edges[next < 0 ? first : next].vertex < 0 ||
            (unsigned long)bsp->edges[next < 0 ? first : next].vertex >= bsp->counts[1])
            return -1;
        second_bsp_vertex = bsp->vertices + bsp->edges[next < 0 ? first : next].vertex;
        bh_round_float(&delta.x, (double)second_bsp_vertex->x - first_bsp_vertex->x);
        bh_round_float(&delta.y, (double)second_bsp_vertex->y - first_bsp_vertex->y);
        bh_round_float(&delta.z, (double)second_bsp_vertex->z - first_bsp_vertex->z);
        bh_round_float(&relative.x, (double)point->x - first_bsp_vertex->x);
        bh_round_float(&relative.y, (double)point->y - first_bsp_vertex->y);
        bh_round_float(&relative.z, (double)point->z - first_bsp_vertex->z);
        bh_vec3_cross(&delta, normal, &cross);
        /* 0044d7cc TEST AH,41 sends unordered to the same side as <=0. */
        if (((double)cross.x * relative.x + (double)cross.y * relative.y) + (double)cross.z * relative.z > 0)
            signs |= 1;
        else
            signs |= 2;
        edge = next;
    } while (edge >= 0 && signs != 3);
    return signs == 1 || signs == 2;
}
static int faces(BHSegmentQuery *query, const BHBspNode *node, const BHVec3 *first_vector, const BHVec3 *second_vector, const BHMat34 *to_world,
                 BHSegmentHit *out)
{
    const BHBsp *bsp = query->geometry->bsp;
    BHVec3 point, normal;
    float da, db, denominator, y;
    double inverse;
    long leaf;
    unsigned long side, steps;
    int result;
    bh_round_float(&da, distance(node, first_vector));
    bh_round_float(&db, distance(node, second_vector));
    bh_round_float(&denominator, (double)db - da);
    inverse = 1.0 / ((double)db - da);
    bh_round_float(&point.x, ((double)db * first_vector->x - (double)da * second_vector->x) * inverse);
    bh_round_float(&y, (double)db * first_vector->y - (double)da * second_vector->y);
    bh_round_float(&point.y, (double)y * inverse);
    bh_round_float(&point.z, ((double)db * first_vector->z - (double)da * second_vector->z) * inverse);
    normal.x = node->normal[0];
    normal.y = node->normal[1];
    normal.z = node->normal[2];
    for (side = 0; side < 2; ++side)
    {
        leaf = side ? node->back_leaf : node->front_leaf;
        steps = 0;
        while (leaf >= 0)
        {
            if ((unsigned long)leaf >= bsp->counts[5] || ++steps > bsp->counts[5])
                return -1;
            result = polygon(query, bsp->leaves[leaf].face, &point, &normal);
            if (result < 0)
                return -1;
            if (result)
            {
                bh_mat34_transform_point(to_world, &point, &out->point);
                bh_mat34_transform_direction(to_world, &normal, &out->normal);
                if (side)
                {
                    out->normal.x = -out->normal.x;
                    out->normal.y = -out->normal.y;
                    out->normal.z = -out->normal.z;
                }
                bh_round_float(&out->point.x, (double)out->point.x - (double)out->normal.x * -0.05);
                bh_round_float(&out->point.y, (double)out->point.y - (double)out->normal.y * -0.05);
                bh_round_float(&out->point.z, (double)out->point.z - (double)out->normal.z * -0.05);
                out->face = bsp->leaves[leaf].face;
                out->flags = query->geometry->face_flags[out->face];
                bh_round_float(&out->fraction, (double)da / denominator);
                /* 0044d98f takes unordered too, but FMUL -1 preserves the
                   NaN sign. C unary minus emits FCHS, which would flip it. */
                if (!_isnan(out->fraction) && out->fraction < 0)
                    out->fraction = -out->fraction;
                return 1;
            }
            leaf = bsp->leaves[leaf].next;
        }
    }
    return 0;
}
int bh_segment_query_run(BHSegmentQuery *query, const BHVec3 *start, const BHVec3 *end, const BHMat34 *to_world,
                         const BHMat34 *to_local, BHSegmentHit *out)
{
    const BHBsp *bsp;
    const BHBspNode *node;
    Frame *stack, task;
    BHVec3 a, b;
    BHSegmentHit hit;
    double da, db;
    float stored_b;
    unsigned long depth = 0, work = 0, i;
    int result;
    if (!query || !query->frames || !query->geometry || !start || !end || !to_world || !to_local || !out || query->busy ||
        !bh_math_environment_supported())
        return fail(query, BH_BSP_ARGUMENT);
    /* Original 0044d9b0 transforms both endpoints without a finite-value gate.
       A prior coplanar hit can supply a NaN endpoint to the next model query;
       preserve the original unordered traversal instead of aborting the hit. */
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)to_world)[i]) || !_finite(((const float *)to_local)[i]))
            return fail(query, BH_BSP_NUMERICAL);
    bsp = query->geometry->bsp;
    query->busy = 1;
    query->error = BH_BSP_OK;
    bh_mat34_transform_point(to_local, start, &a);
    bh_mat34_transform_point(to_local, end, &b);
    stack = (Frame *)query->frames;
    if (bsp->counts[4])
    {
        stack[depth].node = 0;
        stack[depth++].faces = 0;
    }
    while (depth)
    {
        task = stack[--depth];
        if (++work > 1048576 || task.node < 0 || (unsigned long)task.node >= bsp->counts[4])
            goto invalid;
        node = bsp->nodes + task.node;
        if (task.faces)
        {
            result = faces(query, node, &a, &b, to_world, &hit);
            if (result < 0)
                goto invalid;
            if (result)
            {
                *out = hit;
                query->busy = 0;
                return 1;
            }
            continue;
        }
        da = distance(node, &a);
        db = distance(node, &b);
        bh_round_float(&stored_b, db);
        if (da * db > 0)
        {
            task.node = ((double)stored_b + da < 0) ? node->back_node : node->front_node;
            if (task.node >= 0)
                stack[depth++] = task;
        }
        else
        {
            float stored_a;
            long near_node, far_node;
            bh_round_float(&stored_a, da);
            near_node = da >= 0 ? node->front_node : node->back_node;
            far_node = stored_a >= 0 ? node->back_node : node->front_node;
            if (depth + 3 > query->capacity)
                goto invalid;
            if (far_node >= 0)
            {
                stack[depth].node = far_node;
                stack[depth++].faces = 0;
            }
            stack[depth].node = task.node;
            stack[depth++].faces = 1;
            if (near_node >= 0)
            {
                stack[depth].node = near_node;
                stack[depth++].faces = 0;
            }
        }
    }
    query->busy = 0;
    return 0;
invalid:
    query->busy = 0;
    return fail(query, BH_BSP_CYCLE);
}
