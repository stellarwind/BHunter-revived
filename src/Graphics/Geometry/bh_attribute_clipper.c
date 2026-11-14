#include "bh_attribute_clipper.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <string.h>
#include <float.h>
static_assert(sizeof(BHClipVertex) == 0x3c, "BHClipVertex size");
static_assert(offsetof(BHClipVertex, point) == 0xc, "BHClipVertex.point offset");
static_assert(offsetof(BHClipVertex, point) + offsetof(BHVec3, z) == 0x14, "attribute vertex z");
static_assert(offsetof(BHClipVertex, u) == 0x18 && offsetof(BHClipVertex, v) == 0x1c, "attribute vertex uv");
static_assert(offsetof(BHClipVertex, sx) == 0x20 && offsetof(BHClipVertex, sy) == 0x24, "attribute vertex screen");
static_assert(offsetof(BHClipVertex, unknown28) == 0x28, "BHClipVertex.unknown28 offset");
static_assert(offsetof(BHClipVertex, next) == 0x2c && offsetof(BHClipVertex, previous) == 0x30,
              "attribute vertex links");
static_assert(offsetof(BHClipVertex, projected) == 0x38, "BHClipVertex.projected offset");
static BHClipVertex *fail(BHAttributeClipContext *context, BHClipError error)
{
    if (context)
        context->error = error;
    return NULL;
}
static float scalar(const BHClipVertex *vertex)
{
    float value;
    memcpy(&value, &vertex->unknown28, 4);
    return value;
}
static void store_scalar(BHClipVertex *vertex, double value)
{
    float stored;
    bh_round_float(&stored, value);
    memcpy(&vertex->unknown28, &stored, 4);
}
int bh_attribute_clip_init(BHAttributeClipContext *context, float near_z, float far_z, float left, float right, float top,
                           float bottom)
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
    return 1;
}
static float coordinate(const BHClipVertex *vertex, unsigned long axis)
{
    return axis == 0 ? vertex->point.z : axis == 1 ? vertex->sx : vertex->sy;
}
static int outside(const BHClipVertex *p, unsigned long axis, float bound, int low)
{
    float value = coordinate(p, axis);
    return low ? value < bound : bound < value;
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
static int depth_intersect(BHClipVertex *out, const BHClipVertex *first_vertex, const BHClipVertex *second_vertex, float bound)
{
    double da = (double)first_vertex->point.z - bound, db = (double)bound - second_vertex->point.z, d = (double)first_vertex->point.z - second_vertex->point.z;
    out->projected = 0;
    bh_round_float(&out->point.x, (db * first_vertex->point.x + (double)second_vertex->point.x * da) / d);
    bh_round_float(&out->point.y, (db * first_vertex->point.y + (double)second_vertex->point.y * da) / d);
    store_scalar(out, (db * scalar(first_vertex) + da * scalar(second_vertex)) / d);
    bh_round_float(&out->u, (db * first_vertex->u + da * second_vertex->u) / d);
    bh_round_float(&out->v, (db * first_vertex->v + da * second_vertex->v) / d);
    out->point.z = bound;
    return _finite(out->point.x) && _finite(out->point.y) && _finite(out->u) && _finite(out->v) && _finite(scalar(out));
}
static int side_intersect(BHClipVertex *out, const BHClipVertex *first_vertex, const BHClipVertex *second_vertex, unsigned long axis,
                          float bound)
{
    double av = coordinate(first_vertex, axis), bv = coordinate(second_vertex, axis), da = av - bound, db = (double)bound - bv, d = av - bv,
           inverse;
    float stored_da, stored_db, stored_d;
    if (first_vertex->point.z == 0 || second_vertex->point.z == 0)
        return 0;
    out->projected = 0;
    /* Original FST retains da for reciprocal Z; U consumes three stored weights.
       V, other screen coordinate and scalar28 recompute unspilled weights. */
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
    store_scalar(out, (db * scalar(first_vertex) + da * scalar(second_vertex)) / d);
    return _finite(out->point.z) && _finite(out->u) && _finite(out->v) && _finite(out->sx) && _finite(out->sy) &&
           _finite(scalar(out));
}
static BHClipVertex *cut(BHAttributeClipContext *context, BHClipVertex *extreme, unsigned long axis, float bound, int low)
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
    if (axis == 0)
    {
        if (!depth_intersect(first, before, before->next, bound) ||
            !depth_intersect(second, after, after->previous, bound))
            return fail(context, BH_CLIP_NUMERICAL);
    }
    else if (!side_intersect(first, before, before->next, axis, bound) ||
             !side_intersect(second, after, after->previous, axis, bound))
        return fail(context, BH_CLIP_NUMERICAL);
    first->previous = before;
    first->next = second;
    second->previous = first;
    second->next = after;
    before->next = first;
    after->previous = second;
    return first;
}
BHClipVertex *bh_attribute_clip_polygon(BHAttributeClipContext *context, BHClipVertex *head, int project_depth)
{
    BHClipVertex *vertex, *minimum, *maximum;
    unsigned long n = 0, i, axis;
    float low, high;
    if (!context)
        return NULL;
    context->used = 0;
    context->error = BH_CLIP_OK;
    if (!head)
        return fail(context, BH_CLIP_ARGUMENT);
    if (!_finite(context->near_z) || !_finite(context->far_z) || !_finite(context->left) || !_finite(context->right) || !_finite(context->top) ||
        !_finite(context->bottom) || context->near_z <= 0 || context->far_z < context->near_z || context->right < context->left || context->bottom < context->top)
        return fail(context, BH_CLIP_ARGUMENT);
    vertex = head;
    do
    {
        for (i = 0; i < 12; ++i)
            if (vertex == context->scratch + i)
                return fail(context, BH_CLIP_ARGUMENT);
        if (!vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++n > 1024)
            return fail(context, BH_CLIP_TOPOLOGY);
        if (!_finite(vertex->point.z) || !_finite(vertex->u) || !_finite(vertex->v) || !_finite(scalar(vertex)) ||
            (project_depth && (!_finite(vertex->point.x) || !_finite(vertex->point.y))) ||
            ((!project_depth || vertex->projected) && (!_finite(vertex->sx) || !_finite(vertex->sy))))
            return fail(context, BH_CLIP_NUMERICAL);
        vertex = vertex->next;
    } while (vertex != head);
    if (project_depth)
    {
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
            if (!_finite(vertex->sx) || !_finite(vertex->sy))
                return fail(context, BH_CLIP_NUMERICAL);
            vertex = vertex->next;
        } while (vertex != head);
    }
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
