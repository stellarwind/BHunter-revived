#include "bh_collision_query.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>

typedef struct SourceFrame
{
    long node;
    int phase;
} SourceFrame;

static int query_fail(BHCollisionQuery *q, BHBspError error)
{
    q->error = error;
    return -1;
}
int __cdecl bh_collision_query_init(BHCollisionQuery *q, BHGeometry *a, BHGeometry *b)
{
    BHCollisionQuery next;
    unsigned long size;
    if (!q || !a || !b || !a->bsp || !b->bsp || !a->face_flags || !b->face_flags || !a->allocator.allocate ||
        !a->allocator.release || a->bsp->counts[4] > 65535 || b->bsp->counts[4] > 65535 || a->bsp->counts[3] > 65535)
        return 0;
    memset(&next, 0, sizeof(next));
    next.a = a;
    next.b = b;
    next.allocator = a->allocator;
    next.a_capacity = a->bsp->counts[4] + 1;
    next.b_capacity = b->bsp->counts[4] + 1;
    size = next.a_capacity * sizeof(SourceFrame) + next.b_capacity * sizeof(long) +
           a->bsp->counts[3] * sizeof(unsigned long);
    next.workspace = next.allocator.allocate(size, next.allocator.user);
    if (!next.workspace)
        return 0;
    next.a_stack = next.workspace;
    next.b_stack = (long *)((SourceFrame *)next.a_stack + next.a_capacity);
    next.suppressed = (unsigned long *)(next.b_stack + next.b_capacity);
    memset(next.suppressed, 0, a->bsp->counts[3] * sizeof(unsigned long));
    *q = next;
    return 1;
}
int __cdecl bh_collision_query_dispose(BHCollisionQuery *q)
{
    if (!q || q->busy)
        return 0;
    if (q->workspace)
        q->allocator.release(q->workspace, q->allocator.user);
    memset(q, 0, sizeof(*q));
    return 1;
}
static int step(BHCollisionQuery *q)
{
    if (++q->work > 1048576)
        return query_fail(q, BH_BSP_LIMIT);
    return 1;
}
static int valid_node(const BHBsp *b, long node)
{
    return node >= 0 && (unsigned long)node < b->counts[4] && b->nodes && b->bounds;
}
static int spheres(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b, int source_tree)
{
    const BHBsp *a = q->a->bsp, *b = q->b->bsp;
    BHVec3 point, d;
    float checked_radius;
    double x, y, z, radius;
    if (!valid_node(a, an) || !valid_node(b, bn))
        return query_fail(q, BH_BSP_FORMAT);
    bh_mat34_transform_point(a_to_b, &a->bounds[an].center, &point);
    /* Preserve source rejection guards independently of the original gate.
       The rounded validation copies must not feed its retained x87 deltas. */
    bh_round_float(&d.x, (double)point.x - b->bounds[bn].center.x);
    bh_round_float(&d.y, (double)point.y - b->bounds[bn].center.y);
    bh_round_float(&d.z, (double)point.z - b->bounds[bn].center.z);
    bh_round_float(&checked_radius, (double)a->bounds[an].radius + b->bounds[bn].radius);
    if (!_finite(d.x) || !_finite(d.y) || !_finite(d.z) || !_finite(checked_radius) || checked_radius < 0)
        return query_fail(q, BH_BSP_NUMERICAL);
    /* 0044d34b..0044d386 retain the differences and B+A radius sum, then
       square/add X,Y,Z in that order. No binary32 stores precede the gate. */
    x = (double)point.x - b->bounds[bn].center.x;
    y = (double)point.y - b->bounds[bn].center.y;
    z = (double)point.z - b->bounds[bn].center.z;
    if (source_tree)
        radius = (double)b->bounds[bn].radius + a->bounds[an].radius;
    else
        /* Destination owner0044c2bb loads A then B instead. */
        radius = (double)a->bounds[an].radius + b->bounds[bn].radius;
    return (x * x + y * y) + z * z <= radius * radius;
}
static int leaf(BHCollisionQuery *q, const BHBsp *b, long index, const BHBspLeaf **out)
{
    if (!b->leaves || !b->faces || index < 0 || (unsigned long)index >= b->counts[5])
        return query_fail(q, BH_BSP_FORMAT);
    *out = b->leaves + index;
    if ((*out)->face < 0 || (unsigned long)(*out)->face >= b->counts[3])
        return query_fail(q, BH_BSP_FORMAT);
    return step(q);
}
/* Source node plane tested against each destination sphere before leaf work. */
static int node_contacts(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                         BHCollisionHit *out)
{
    const BHBsp *a = q->a->bsp, *b = q->b->bsp;
    const BHBspNode *na = a->nodes + an, *nb = b->nodes + bn;
    const BHBspLeaf *al, *bl;
    BHVec3 center, normal, local_normal;
    BHCollisionHit hit;
    double d;
    long ai, bi;
    unsigned long as, bs, ac, bc;
    int result;
    bh_mat34_inverse_transform_point(a_to_b, &b->bounds[bn].center, &center);
    /* 0044c301..0044c33d sums Y/Z before X; cancellation affects pruning. */
    d = (((double)na->normal[1] * center.y + (double)na->normal[2] * center.z) + (double)na->normal[0] * center.x) +
        na->plane;
    if (!_finite(d))
        return query_fail(q, BH_BSP_NUMERICAL);
    if (b->bounds[bn].radius < d || d < -(double)b->bounds[bn].radius)
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
            if (++ac > a->counts[5] || leaf(q, a, ai, &al) < 0)
                return query_fail(q, BH_BSP_CYCLE);
            if (!q->suppressed[al->face] && !(q->a->face_flags[al->face] & 1))
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
                        if (++bc > b->counts[5] || leaf(q, b, bi, &bl) < 0)
                            return query_fail(q, BH_BSP_CYCLE);
                        if (!(q->b->face_flags[bl->face] & 1))
                        {
                            result = bh_collision_node_contact(a, al->face, a_to_b, b, bl->face, bn, b_to_world,
                                                               &oriented, &hit.contact);
                            if (result < 0)
                                return query_fail(q, BH_BSP_FORMAT);
                            if (result)
                            {
                                hit.normal = oriented;
                                hit.a_node = an;
                                hit.b_node = bn;
                                hit.a_face = al->face;
                                hit.b_face = bl->face;
                                hit.flags = q->b->face_flags[bl->face];
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
static int destination_tree(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                            BHCollisionHit *out)
{
    unsigned long depth = 0;
    long node;
    const BHBspNode *n;
    int result;
    if (an < 0 || bn < 0)
        return 0;
    q->b_stack[depth++] = bn;
    while (depth)
    {
        node = q->b_stack[--depth];
        if (step(q) < 0)
            return -1;
        result = spheres(q, an, node, a_to_b, 0);
        if (result < 0)
            return -1;
        if (!result)
            continue;
        result = node_contacts(q, an, node, a_to_b, b_to_world, out);
        if (result == 2)
            continue;
        if (result)
            return result;
        n = q->b->bsp->nodes + node;
        if (depth + 2 > q->b_capacity)
            return query_fail(q, BH_BSP_CYCLE);
        if (n->back_node >= 0)
            q->b_stack[depth++] = n->back_node;
        if (n->front_node >= 0)
            q->b_stack[depth++] = n->front_node;
    }
    return 0;
}
static int mark_faces(BHCollisionQuery *q, long an, int delta)
{
    const BHBsp *a = q->a->bsp;
    const BHBspNode *n = a->nodes + an;
    const BHBspLeaf *l;
    long index;
    unsigned long side, count;
    for (side = 0; side < 2; ++side)
    {
        index = side ? n->back_leaf : n->front_leaf;
        count = 0;
        while (index >= 0)
        {
            if (++count > a->counts[5] || leaf(q, a, index, &l) < 0)
                return query_fail(q, BH_BSP_CYCLE);
            if (delta > 0)
                ++q->suppressed[l->face];
            else
            {
                if (!q->suppressed[l->face])
                    return query_fail(q, BH_BSP_FORMAT);
                --q->suppressed[l->face];
            }
            index = l->next;
        }
    }
    return 1;
}
static int execute(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b, const BHMat34 *b_to_world,
                   BHCollisionHit *out, int source_tree)
{
    SourceFrame *stack, *f;
    BHCollisionHit hit;
    unsigned long depth = 0, i;
    long child;
    int result = 0;
    if (!q || !q->workspace || q->busy || !a_to_b || !b_to_world || !out || !bh_math_environment_supported())
        return -1;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)a_to_b)[i]) || !_finite(((const float *)b_to_world)[i]))
            return query_fail(q, BH_BSP_NUMERICAL);
    q->busy = 1;
    q->work = 0;
    q->error = BH_BSP_OK;
    if (an < 0 || bn < 0)
        goto done;
    if (!source_tree)
    {
        result = destination_tree(q, an, bn, a_to_b, b_to_world, &hit);
        goto done;
    }
    stack = (SourceFrame *)q->a_stack;
    stack[depth].node = an;
    stack[depth++].phase = 0;
    while (depth)
    {
        f = stack + depth - 1;
        if (step(q) < 0)
        {
            result = -1;
            break;
        }
        if (f->phase == 0)
        {
            result = spheres(q, f->node, bn, a_to_b, 1);
            if (result < 0)
                break;
            if (!result)
            {
                --depth;
                continue;
            }
            result = destination_tree(q, f->node, bn, a_to_b, b_to_world, &hit);
            if (result)
                break;
            if (mark_faces(q, f->node, 1) < 0)
            {
                result = -1;
                break;
            }
            f->phase = 1;
            child = q->a->bsp->nodes[f->node].front_node;
        }
        else if (f->phase == 1)
        {
            f->phase = 2;
            child = q->a->bsp->nodes[f->node].back_node;
        }
        else
        {
            if (mark_faces(q, f->node, -1) < 0)
            {
                result = -1;
                break;
            }
            --depth;
            continue;
        }
        if (child >= 0)
        {
            if (depth >= q->a_capacity)
            {
                result = query_fail(q, BH_BSP_CYCLE);
                break;
            }
            stack[depth].node = child;
            stack[depth++].phase = 0;
        }
    }
done:
    /* Cleanup also covers malformed topology and early hit while ancestors are
       marked; no query state leaks into the next collision attempt. */
    memset(q->suppressed, 0, q->a->bsp->counts[3] * sizeof(unsigned long));
    q->busy = 0;
    if (result == 1)
        *out = hit;
    return result;
}
int __cdecl bh_collision_query_node(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b,
                                    const BHMat34 *b_to_world, BHCollisionHit *out)
{
    return execute(q, an, bn, a_to_b, b_to_world, out, 0);
}
int __cdecl bh_collision_query_run(BHCollisionQuery *q, long an, long bn, const BHMat34 *a_to_b,
                                   const BHMat34 *b_to_world, BHCollisionHit *out)
{
    return execute(q, an, bn, a_to_b, b_to_world, out, 1);
}
