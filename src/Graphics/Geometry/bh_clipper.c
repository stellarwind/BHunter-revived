#include "bh_clipper.h"
#include "../../Math/bh_matrix_inverse.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <stddef.h>
#include <float.h>
static_assert(sizeof(BHClipVertex) == 60, "BHClipVertex size");
static_assert(offsetof(BHClipVertex, next) == 0x2c, "BHClipVertex.next offset");
static_assert(offsetof(BHClipVertex, projected) == 0x38, "BHClipVertex.projected offset");
static BHClipVertex *fail(BHClipContext *context, BHClipError e)
{
    if (context)
        context->error = e;
    return NULL;
}
static void identity(BHMat34 *matrix)
{
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = matrix->m[1][1] = matrix->m[2][2] = 1;
}
int bh_clip_init(BHClipContext *context, float near_z, float far_z, float left, float right, float top, float bottom)
{
    if (!context || !_finite(near_z) || !_finite(far_z) || !_finite(left) || !_finite(right) || !_finite(top) ||
        !_finite(bottom) || near_z <= 0 || far_z < near_z || right < left || bottom < top)
        return 0;
    memset(context, 0, sizeof(*context));
    context->near_z = near_z;
    context->far_z = far_z;
    context->left = left;
    context->right = right;
    context->top = top;
    context->bottom = bottom;
    identity(&context->camera);
    identity(&context->inverse);
    return 1;
}
int bh_clip_set_camera(BHClipContext *context, const BHMat34 *camera)
{
    unsigned long i;
    if (!context || !camera)
        return 0;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)camera)[i]))
            return 0;
    context->camera = *camera;
    context->inverse_ready = 0;
    return 1;
}
static float coordinate(const BHClipVertex *vertex, unsigned long axis)
{
    return axis == 0 ? vertex->point.z : axis == 1 ? vertex->sx : vertex->sy;
}
static int outside(const BHClipVertex *p, unsigned long axis, float bound, int low)
{
    float v = coordinate(p, axis);
    return low ? v < bound : bound < v;
}
static void extrema(BHClipVertex *head, unsigned long axis, BHClipVertex **minimum, BHClipVertex **maximum)
{
    BHClipVertex *vertex = head->next;
    *minimum = *maximum = head;
    while (vertex != head)
    {
        if (coordinate(vertex, axis) < coordinate(*minimum, axis))
            *minimum = vertex;
        if (coordinate(*maximum, axis) < coordinate(vertex, axis))
            *maximum = vertex;
        vertex = vertex->next;
    }
}
static void depth_intersection(BHClipVertex *out, const BHClipVertex *first_vertex, const BHClipVertex *second_vertex, float bound)
{
    double da = (double)first_vertex->point.z - bound, db = (double)bound - second_vertex->point.z, d = (double)first_vertex->point.z - second_vertex->point.z;
    bh_round_float(&out->point.x, (db * first_vertex->point.x + (double)second_vertex->point.x * da) / d);
    bh_round_float(&out->point.y, (db * first_vertex->point.y + (double)second_vertex->point.y * da) / d);
    bh_round_float(&out->u, (db * first_vertex->u + da * second_vertex->u) / d);
    bh_round_float(&out->v, (db * first_vertex->v + da * second_vertex->v) / d);
    out->point.z = bound;
}
static void side_intersection(BHClipVertex *out, const BHClipVertex *first_vertex, const BHClipVertex *second_vertex, unsigned long axis,
                              float bound)
{
    double av = coordinate(first_vertex, axis), bv = coordinate(second_vertex, axis), da = av - bound, db = (double)bound - bv, d = av - bv,
           inverse;
    float stored_da, stored_db, stored_d;
    /* Original spills these three values, but retains da for reciprocal-Z.
       U consumes the spills; V and the other screen axis recompute weights. */
    bh_round_float(&stored_da, da);
    bh_round_float(&stored_db, db);
    bh_round_float(&stored_d, d);
    inverse = (da * (1.0 / second_vertex->point.z) - stored_db * (-1.0 / first_vertex->point.z)) / stored_d;
    bh_round_float(&out->u, (((double)second_vertex->u / second_vertex->point.z) * stored_da + ((double)first_vertex->u / first_vertex->point.z) * stored_db) /
                                stored_d / inverse);
    bh_round_float(&out->point.z, 1.0 / inverse);
    bh_round_float(&out->v, (((double)first_vertex->v / first_vertex->point.z) * db + da * ((double)second_vertex->v / second_vertex->point.z)) / d / inverse);
    if (axis == 1)
    {
        out->sx = bound;
        bh_round_float(&out->sy, (db * first_vertex->sy + da * second_vertex->sy) / d);
    }
    else
    {
        out->sy = bound;
        bh_round_float(&out->sx, (db * first_vertex->sx + da * second_vertex->sx) / d);
    }
}
static BHClipVertex *cut(BHClipContext *context, BHClipVertex *extreme, unsigned long axis, float bound, int low)
{
    BHClipVertex *before = extreme->previous, *after = extreme->next, *first, *second;
    unsigned long steps = 0;
    while (outside(before, axis, bound, low))
    {
        if (++steps > 1036)
            return fail(context, BH_CLIP_TOPOLOGY);
        before = before->previous;
    }
    steps = 0;
    while (outside(after, axis, bound, low))
    {
        if (++steps > 1036)
            return fail(context, BH_CLIP_TOPOLOGY);
        after = after->next;
    }
    if (context->used > 10)
        return fail(context, BH_CLIP_CAPACITY);
    first = context->scratch + context->used;
    second = first + 1;
    context->used += 2;
    memset(first, 0, 2 * sizeof(*first));
    if (axis == 0)
    {
        depth_intersection(first, before, before->next, bound);
        depth_intersection(second, after, after->previous, bound);
    }
    else
    {
        side_intersection(first, before, before->next, axis, bound);
        side_intersection(second, after, after->previous, axis, bound);
    }
    first->previous = before;
    first->next = second;
    second->previous = first;
    second->next = after;
    before->next = first;
    after->previous = second;
    return first;
}
BHClipVertex *bh_clip_polygon(BHClipContext *context, BHClipVertex *head, const BHTextureImage *image)
{
    BHClipVertex *vertex, *minimum, *maximum;
    unsigned long n = 0, axis, i;
    float coefficients[6], low, high;
    BHVec3 world;
    if (!context || !head)
        return fail(context, BH_CLIP_ARGUMENT);
    context->used = 0;
    context->error = BH_CLIP_OK;
    /* Valid pointer storage is a caller contract; link consistency/count and
       numerical checks reject malformed source data before modifying the ring. */
    vertex = head;
    do
    {
        if (!vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++n > 1024)
            return fail(context, BH_CLIP_TOPOLOGY);
        if (!_finite(vertex->point.x) || !_finite(vertex->point.y) || !_finite(vertex->point.z) || !_finite(vertex->u) || !_finite(vertex->v) ||
            (vertex->projected && (!_finite(vertex->sx) || !_finite(vertex->sy))))
            return fail(context, BH_CLIP_NUMERICAL);
        vertex = vertex->next;
    } while (vertex != head);
    if (image && image->unused54)
    {
        memcpy(coefficients, image->padding, sizeof(coefficients));
        for (i = 0; i < 6; ++i)
            if (!_finite(coefficients[i]))
                return fail(context, BH_CLIP_NUMERICAL);
        if (!context->inverse_ready)
        {
            if (!bh_mat34_inverse_general(&context->camera, &context->inverse))
                return fail(context, BH_CLIP_NUMERICAL);
            context->inverse_ready = 1;
        }
        vertex = head;
        do
        {
            bh_mat34_transform_point(&context->inverse, &vertex->point, &world);
            bh_round_float(&vertex->u,
                           ((double)world.z * coefficients[1] + (double)world.x * coefficients[0]) + coefficients[2]);
            bh_round_float(&vertex->v,
                           ((double)world.z * coefficients[4] + (double)world.x * coefficients[3]) + coefficients[5]);
            if (!_finite(vertex->u) || !_finite(vertex->v))
                return fail(context, BH_CLIP_NUMERICAL);
            vertex = vertex->next;
        } while (vertex != head);
    }
    extrema(head, 0, &minimum, &maximum);
    if (context->far_z < minimum->point.z || maximum->point.z < context->near_z)
        return NULL;
    if (minimum->point.z < context->near_z)
    {
        head = cut(context, minimum, 0, context->near_z, 1);
        if (!head)
            return NULL;
    }
    if (context->far_z < maximum->point.z)
    {
        head = cut(context, maximum, 0, context->far_z, 0);
        if (!head)
            return NULL;
    }
    vertex = head;
    do
    {
        if (!vertex->projected)
        {
            vertex->projected = 1;
            bh_round_float(&vertex->sx, (double)vertex->point.x / vertex->point.z);
            bh_round_float(&vertex->sy, (double)vertex->point.y / vertex->point.z);
        }
        vertex = vertex->next;
    } while (vertex != head);
    for (axis = 1; axis <= 2; ++axis)
    {
        low = axis == 1 ? context->left : context->top;
        high = axis == 1 ? context->right : context->bottom;
        extrema(head, axis, &minimum, &maximum);
        if (high < coordinate(minimum, axis) || coordinate(maximum, axis) < low)
            return NULL;
        if (coordinate(minimum, axis) < low)
        {
            head = cut(context, minimum, axis, low, 1);
            if (!head)
                return NULL;
        }
        if (high < coordinate(maximum, axis))
        {
            head = cut(context, maximum, axis, high, 0);
            if (!head)
                return NULL;
        }
    }
    return head;
}
