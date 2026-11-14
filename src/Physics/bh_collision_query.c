#include "bh_collision_query.h"
#include "../Math/bh_fp.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <string.h>

typedef struct SourceFrame
{
    long node;
    int phase;
} SourceFrame;

static int query_fail(BHCollisionQuery *query, BHBspError error)
{
    query->error = error;
    return -1;
}
int bh_collision_query_init(BHCollisionQuery *q, BHGeometry *first_geometry, BHGeometry *second_geometry)
{
    BHCollisionQuery next;
    unsigned long size;
    if (!q || !first_geometry || !second_geometry || !first_geometry->bsp || !second_geometry->bsp || !first_geometry->face_flags || !second_geometry->face_flags || !first_geometry->allocator.allocate ||
        !first_geometry->allocator.release || first_geometry->bsp->counts[4] > 65535 || second_geometry->bsp->counts[4] > 65535 || first_geometry->bsp->counts[3] > 65535)
        return 0;
    memset(&next, 0, sizeof(next));
    next.a = first_geometry;
    next.b = second_geometry;
    next.allocator = first_geometry->allocator;
    next.a_capacity = first_geometry->bsp->counts[4] + 1;
    next.b_capacity = second_geometry->bsp->counts[4] + 1;
    size = next.a_capacity * sizeof(SourceFrame) + next.b_capacity * sizeof(long) +
           first_geometry->bsp->counts[3] * sizeof(unsigned long);
    next.workspace = next.allocator.allocate(size, next.allocator.user);
    if (!next.workspace)
        return 0;
    next.a_stack = next.workspace;
    next.b_stack = (long *)((SourceFrame *)next.a_stack + next.a_capacity);
    next.suppressed = (unsigned long *)(next.b_stack + next.b_capacity);
    memset(next.suppressed, 0, first_geometry->bsp->counts[3] * sizeof(unsigned long));
    *q = next;
    return 1;
}
int bh_collision_query_dispose(BHCollisionQuery *query)
{
    if (!query || query->busy)
        return 0;
    if (query->workspace)
        query->allocator.release(query->workspace, query->allocator.user);
    memset(query, 0, sizeof(*query));
    return 1;
}
static int step(BHCollisionQuery *query)
{
    if (++query->work > 1048576)
        return query_fail(query, BH_BSP_LIMIT);
    return 1;
}
static int valid_node(const BHBsp *bsp, long node)
{
    return node >= 0 && (unsigned long)node < bsp->counts[4] && bsp->nodes && bsp->bounds;
}
static int spheres(BHCollisionQuery *query, long an, long bn, const BHMat34 *a_to_b, int source_tree)
{
    const BHBsp *first_bsp = query->a->bsp, *second_bsp = query->b->bsp;
    BHVec3 point, d;
    float checked_radius;
    double x, y, z, radius;
    if (!valid_node(first_bsp, an) || !valid_node(second_bsp, bn))
        return query_fail(query, BH_BSP_FORMAT);
    bh_mat34_transform_point(a_to_b, &first_bsp->bounds[an].center, &point);
    /* Preserve source rejection guards independently of the original gate.
       The rounded validation copies must not feed its retained x87 deltas. */
    bh_round_float(&d.x, (double)point.x - second_bsp->bounds[bn].center.x);
    bh_round_float(&d.y, (double)point.y - second_bsp->bounds[bn].center.y);
    bh_round_float(&d.z, (double)point.z - second_bsp->bounds[bn].center.z);
    bh_round_float(&checked_radius, (double)first_bsp->bounds[an].radius + second_bsp->bounds[bn].radius);
    if (!_finite(d.x) || !_finite(d.y) || !_finite(d.z) || !_finite(checked_radius) || checked_radius < 0)
        return query_fail(query, BH_BSP_NUMERICAL);
    /* 0044d34b..0044d386 retain the differences and B+A radius sum, then
       square/add X,Y,Z in that order. No binary32 stores precede the gate. */
    x = (double)point.x - second_bsp->bounds[bn].center.x;
    y = (double)point.y - second_bsp->bounds[bn].center.y;
    z = (double)point.z - second_bsp->bounds[bn].center.z;
    if (source_tree)
        radius = (double)second_bsp->bounds[bn].radius + first_bsp->bounds[an].radius;
    else
        /* Destination owner0044c2bb loads A then B instead. */
        radius = (double)first_bsp->bounds[an].radius + second_bsp->bounds[bn].radius;
    return (x * x + y * y) + z * z <= radius * radius;
}
static int leaf(BHCollisionQuery *q, const BHBsp *bsp, long index, const BHBspLeaf **out)
{
    if (!bsp->leaves || !bsp->faces || index < 0 || (unsigned long)index >= bsp->counts[5])
        return query_fail(q, BH_BSP_FORMAT);
    *out = bsp->leaves + index;
    if ((*out)->face < 0 || (unsigned long)(*out)->face >= bsp->counts[3])
        return query_fail(q, BH_BSP_FORMAT);
    return step(q);
}
/* Source node plane tested against each destination sphere before leaf work. */
static int node_contacts(BHCollisionQuery *query, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                         BHCollisionHit *out)
{
    const BHBsp *first_bsp = query->a->bsp, *second_bsp = query->b->bsp;
    const BHBspNode *na = first_bsp->nodes + an, *nb = second_bsp->nodes + bn;
    const BHBspLeaf *al, *bl;
    BHVec3 center, normal, local_normal;
    BHCollisionHit hit;
    double d;
    long ai, bi;
    unsigned long as, bs, ac, bc;
    int result;
    bh_mat34_inverse_transform_point(a_to_b, &second_bsp->bounds[bn].center, &center);
    /* 0044c301..0044c33d sums Y/Z before X; cancellation affects pruning. */
    d = (((double)na->normal[1] * center.y + (double)na->normal[2] * center.z) + (double)na->normal[0] * center.x) +
        na->plane;
    if (!_finite(d))
        return query_fail(query, BH_BSP_NUMERICAL);
    if (second_bsp->bounds[bn].radius < d || d < -(double)second_bsp->bounds[bn].radius)
        return 2;
    local_normal.x = nb->normal[0];
    local_normal.y = nb->normal[1];
    local_normal.z = nb->normal[2];
    bh_mat34_transform_direction(b_to_world, &local_normal, &normal);
    for (as = 0; as < 2; ++as)
    {
        ai = as ? na->back_leaf : na->front_leaf;
        ac = 0;
        while (ai >= 0)
        {
            if (++ac > first_bsp->counts[5] || leaf(query, first_bsp, ai, &al) < 0)
                return query_fail(query, BH_BSP_CYCLE);
            if (!query->suppressed[al->face] && !(query->a->face_flags[al->face] & 1))
            {
                for (bs = 0; bs < 2; ++bs)
                {
                    BHVec3 oriented = normal;
                    if (bs)
                    {
                        oriented.x = -oriented.x;
                        oriented.y = -oriented.y;
                        oriented.z = -oriented.z;
                    }
                    bi = bs ? nb->back_leaf : nb->front_leaf;
                    bc = 0;
                    while (bi >= 0)
                    {
                        if (++bc > second_bsp->counts[5] || leaf(query, second_bsp, bi, &bl) < 0)
                            return query_fail(query, BH_BSP_CYCLE);
                        if (!(query->b->face_flags[bl->face] & 1))
                        {
                            result = bh_collision_node_contact(first_bsp, al->face, a_to_b, second_bsp, bl->face, bn, b_to_world,
                                                               &oriented, &hit.contact);
                            if (result < 0)
                                return query_fail(query, BH_BSP_FORMAT);
                            if (result)
                            {
                                hit.normal = oriented;
                                hit.a_node = an;
                                hit.b_node = bn;
                                hit.a_face = al->face;
                                hit.b_face = bl->face;
                                hit.flags = query->b->face_flags[bl->face];
                                *out = hit;
                                return 1;
                            }
                        }
                        bi = bl->next;
                    }
                }
            }
            ai = al->next;
        }
    }
    return 0;
}
static int destination_tree(BHCollisionQuery *query, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                            BHCollisionHit *out)
{
    unsigned long depth = 0;
    long node;
    const BHBspNode *bsp_node;
    int result;
    if (an < 0 || bn < 0)
        return 0;
    query->b_stack[depth++] = bn;
    while (depth)
    {
        node = query->b_stack[--depth];
        if (step(query) < 0)
            return -1;
        result = spheres(query, an, node, a_to_b, 0);
        if (result < 0)
            return -1;
        if (!result)
            continue;
        result = node_contacts(query, an, node, a_to_b, b_to_world, out);
        if (result == 2)
            continue;
        if (result)
            return result;
        bsp_node = query->b->bsp->nodes + node;
        if (depth + 2 > query->b_capacity)
            return query_fail(query, BH_BSP_CYCLE);
        if (bsp_node->back_node >= 0)
            query->b_stack[depth++] = bsp_node->back_node;
        if (bsp_node->front_node >= 0)
            query->b_stack[depth++] = bsp_node->front_node;
    }
    return 0;
}
static int mark_faces(BHCollisionQuery *query, long an, int delta)
{
    const BHBsp *bsp = query->a->bsp;
    const BHBspNode *bsp_node = bsp->nodes + an;
    const BHBspLeaf *bsp_leaf;
    long index;
    unsigned long side, count;
    for (side = 0; side < 2; ++side)
    {
        index = side ? bsp_node->back_leaf : bsp_node->front_leaf;
        count = 0;
        while (index >= 0)
        {
            if (++count > bsp->counts[5] || leaf(query, bsp, index, &bsp_leaf) < 0)
                return query_fail(query, BH_BSP_CYCLE);
            if (delta > 0)
                ++query->suppressed[bsp_leaf->face];
            else
            {
                if (!query->suppressed[bsp_leaf->face])
                    return query_fail(query, BH_BSP_FORMAT);
                --query->suppressed[bsp_leaf->face];
            }
            index = bsp_leaf->next;
        }
    }
    return 1;
}
static int execute(BHCollisionQuery *query, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                   BHCollisionHit *out, int source_tree)
{
    SourceFrame *stack, *source_frame;
    BHCollisionHit hit;
    unsigned long depth = 0, i;
    long child;
    int result = 0;
    if (!query || !query->workspace || query->busy || !a_to_b || !b_to_world || !out || !bh_math_environment_supported())
        return -1;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)a_to_b)[i]) || !_finite(((const float *)b_to_world)[i]))
            return query_fail(query, BH_BSP_NUMERICAL);
    query->busy = 1;
    query->work = 0;
    query->error = BH_BSP_OK;
    if (an < 0 || bn < 0)
        goto done;
    if (!source_tree)
    {
        result = destination_tree(query, an, bn, a_to_b, b_to_world, &hit);
        goto done;
    }
    stack = (SourceFrame *)query->a_stack;
    stack[depth].node = an;
    stack[depth++].phase = 0;
    while (depth)
    {
        source_frame = stack + depth - 1;
        if (step(query) < 0)
        {
            result = -1;
            break;
        }
        if (source_frame->phase == 0)
        {
            result = spheres(query, source_frame->node, bn, a_to_b, 1);
            if (result < 0)
                break;
            if (!result)
            {
                --depth;
                continue;
            }
            result = destination_tree(query, source_frame->node, bn, a_to_b, b_to_world, &hit);
            if (result)
                break;
            if (mark_faces(query, source_frame->node, 1) < 0)
            {
                result = -1;
                break;
            }
            source_frame->phase = 1;
            child = query->a->bsp->nodes[source_frame->node].front_node;
        }
        else if (source_frame->phase == 1)
        {
            source_frame->phase = 2;
            child = query->a->bsp->nodes[source_frame->node].back_node;
        }
        else
        {
            if (mark_faces(query, source_frame->node, -1) < 0)
            {
                result = -1;
                break;
            }
            --depth;
            continue;
        }
        if (child >= 0)
        {
            if (depth >= query->a_capacity)
            {
                result = query_fail(query, BH_BSP_CYCLE);
                break;
            }
            stack[depth].node = child;
            stack[depth++].phase = 0;
        }
    }
done:
    /* Cleanup also covers malformed topology and early hit while ancestors are
       marked; no query state leaks into the next collision attempt. */
    memset(query->suppressed, 0, query->a->bsp->counts[3] * sizeof(unsigned long));
    query->busy = 0;
    if (result == 1)
        *out = hit;
    return result;
}
int bh_collision_query_node(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                            BHCollisionHit *out)
{
    return execute(q, an, bn, a_to_b, b_to_world, out, 0);
}
int bh_collision_query_run(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                           BHCollisionHit *out)
{
    return execute(q, an, bn, a_to_b, b_to_world, out, 1);
}
