#include "bh_screen_clipper.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <float.h>
#include <string.h>
static_assert(sizeof(BHClipVertex) == 0x3c, "BHClipVertex size");
static_assert(offsetof(BHClipVertex, point) + offsetof(BHVec3, z) == 0x14, "screen vertex depth");
static_assert(offsetof(BHClipVertex, u) == 0x18, "BHClipVertex.u offset");
static_assert(offsetof(BHClipVertex, v) == 0x1c, "BHClipVertex.v offset");
static_assert(offsetof(BHClipVertex, sx) == 0x20, "BHClipVertex.sx offset");
static_assert(offsetof(BHClipVertex, sy) == 0x24, "BHClipVertex.sy offset");
static_assert(offsetof(BHClipVertex, next) == 0x2c, "BHClipVertex.next offset");
static_assert(offsetof(BHClipVertex, previous) == 0x30, "BHClipVertex.previous offset");
static_assert(offsetof(BHClipVertex, projected) == 0x38, "BHClipVertex.projected offset");
static BHClipVertex *fail(BHScreenClipContext *context, BHClipError error)
{
    if (context)
        context->error = error;
    return NULL;
}
int bh_screen_clip_init(BHScreenClipContext *context, float near_z, float far_z, float left, float right, float top,
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
    return axis ? vertex->sy : vertex->sx;
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
static int intersect(BHClipVertex *out, const BHClipVertex *inside, const BHClipVertex *outside_node,
                     unsigned long axis, float bound, float depth)
{
    double a = coordinate(inside, axis), b = coordinate(outside_node, axis), wa = (double)bound - b, wb = a - bound,
           denominator = a - b;
    out->point.z = depth;
    out->projected = 0;
    bh_round_float(&out->u, (wa * inside->u + wb * outside_node->u) / denominator);
    bh_round_float(&out->v, (wa * inside->v + wb * outside_node->v) / denominator);
    if (!axis)
    {
        out->sx = bound;
        bh_round_float(&out->sy, (wa * inside->sy + wb * outside_node->sy) / denominator);
    }
    else
    {
        out->sy = bound;
        bh_round_float(&out->sx, (wa * inside->sx + wb * outside_node->sx) / denominator);
    }
    return _finite(out->u) && _finite(out->v) && _finite(out->sx) && _finite(out->sy);
}
static BHClipVertex *cut(BHScreenClipContext *context, BHClipVertex *extreme, unsigned long axis, float bound, int low,
                         float depth)
{
    BHClipVertex *before = extreme->previous, *after = extreme->next, *first, *second;
    unsigned long steps = 0;
    while (outside(before, axis, bound, low))
    {
        if (++steps > 1032)
            return fail(context, BH_CLIP_TOPOLOGY);
        before = before->previous;
    }
    steps = 0;
    while (outside(after, axis, bound, low))
    {
        if (++steps > 1032)
            return fail(context, BH_CLIP_TOPOLOGY);
        after = after->next;
    }
    if (context->used > 6)
        return fail(context, BH_CLIP_CAPACITY);
    first = context->scratch + context->used;
    second = first + 1;
    context->used += 2;
    if (!intersect(first, before, before->next, axis, bound, depth) ||
        !intersect(second, after, after->previous, axis, bound, depth))
        return fail(context, BH_CLIP_NUMERICAL);
    first->previous = before;
    first->next = second;
    second->previous = first;
    second->next = after;
    before->next = first;
    after->previous = second;
    return first;
}
BHClipVertex *bh_screen_clip_polygon(BHScreenClipContext *context, BHClipVertex *head, float depth)
{
    BHClipVertex *vertex, *minimum, *maximum;
    unsigned long n = 0, i, axis;
    float low, high;
    if (!context)
        return NULL;
    context->used = 0;
    context->error = BH_CLIP_OK;
    if (!_finite(depth))
        return fail(context, BH_CLIP_NUMERICAL);
    /* Original rejects uniform depth before dereferencing the input ring. */
    if (context->far_z < depth || depth < context->near_z)
        return NULL;
    if (!head)
        return fail(context, BH_CLIP_ARGUMENT);
    if (!_finite(context->near_z) || !_finite(context->far_z) || !_finite(context->left) || !_finite(context->right) || !_finite(context->top) ||
        !_finite(context->bottom) || context->near_z <= 0 || context->far_z < context->near_z || context->right < context->left || context->bottom < context->top)
        return fail(context, BH_CLIP_ARGUMENT);
    vertex = head;
    do
    {
        for (i = 0; i < 8; ++i)
            if (vertex == context->scratch + i)
                return fail(context, BH_CLIP_ARGUMENT);
        if (!vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++n > 1024)
            return fail(context, BH_CLIP_TOPOLOGY);
        if (!_finite(vertex->sx) || !_finite(vertex->sy) || !_finite(vertex->u) || !_finite(vertex->v))
            return fail(context, BH_CLIP_NUMERICAL);
        vertex = vertex->next;
    } while (vertex != head);
    for (axis = 0; axis < 2; ++axis)
    {
        low = axis ? context->top : context->left;
        high = axis ? context->bottom : context->right;
        extrema(head, axis, &minimum, &maximum);
        if (high < coordinate(minimum, axis) || coordinate(maximum, axis) < low)
            return NULL;
        if (coordinate(minimum, axis) < low)
        {
            head = cut(context, minimum, axis, low, 1, depth);
            if (!head)
                return NULL;
        }
        if (high < coordinate(maximum, axis))
        {
            head = cut(context, maximum, axis, high, 0, depth);
            if (!head)
                return NULL;
        }
    }
    return head;
}
