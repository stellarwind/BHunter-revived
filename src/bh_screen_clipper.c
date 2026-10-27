#include "bh_screen_clipper.h"
#include "bh_round.h"
#include <stddef.h>
#include <float.h>
#include <string.h>
typedef char screen_vertex_size[sizeof(BHClipVertex) == 0x3c ? 1 : -1];
typedef char screen_vertex_depth[offsetof(BHClipVertex, point) + offsetof(BHVec3, z) == 0x14 ? 1 : -1];
typedef char screen_vertex_u[offsetof(BHClipVertex, u) == 0x18 ? 1 : -1];
typedef char screen_vertex_v[offsetof(BHClipVertex, v) == 0x1c ? 1 : -1];
typedef char screen_vertex_x[offsetof(BHClipVertex, sx) == 0x20 ? 1 : -1];
typedef char screen_vertex_y[offsetof(BHClipVertex, sy) == 0x24 ? 1 : -1];
typedef char screen_vertex_next[offsetof(BHClipVertex, next) == 0x2c ? 1 : -1];
typedef char screen_vertex_previous[offsetof(BHClipVertex, previous) == 0x30 ? 1 : -1];
typedef char screen_vertex_projected[offsetof(BHClipVertex, projected) == 0x38 ? 1 : -1];
static BHClipVertex *fail(BHScreenClipContext *c, BHClipError error)
{
    if (c)
        c->error = error;
    return NULL;
}
int __cdecl bh_screen_clip_init(BHScreenClipContext *c, float near_z, float far_z, float left, float right, float top,
                                float bottom)
{
    if (!c || !_finite(near_z) || !_finite(far_z) || !_finite(left) || !_finite(right) || !_finite(top) ||
        !_finite(bottom) || near_z <= 0 || far_z < near_z || right < left || bottom < top)
        return 0;
    memset(c, 0, sizeof(*c));
    c->near_z = near_z;
    c->far_z = far_z;
    c->left = left;
    c->right = right;
    c->top = top;
    c->bottom = bottom;
    return 1;
}
static float coordinate(const BHClipVertex *p, unsigned long axis)
{
    return axis ? p->sy : p->sx;
}
static int outside(const BHClipVertex *p, unsigned long axis, float bound, int low)
{
    float value = coordinate(p, axis);
    return low ? value < bound : bound < value;
}
static void extrema(BHClipVertex *head, unsigned long axis, BHClipVertex **minimum, BHClipVertex **maximum)
{
    BHClipVertex *p = head->next;
    *minimum = *maximum = head;
    while (p != head)
    {
        if (coordinate(p, axis) < coordinate(*minimum, axis))
            *minimum = p;
        if (coordinate(*maximum, axis) < coordinate(p, axis))
            *maximum = p;
        p = p->next;
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
static BHClipVertex *cut(BHScreenClipContext *c, BHClipVertex *extreme, unsigned long axis, float bound, int low,
                         float depth)
{
    BHClipVertex *before = extreme->previous, *after = extreme->next, *first, *second;
    unsigned long steps = 0;
    while (outside(before, axis, bound, low))
    {
        if (++steps > 1032)
            return fail(c, BH_CLIP_TOPOLOGY);
        before = before->previous;
    }
    steps = 0;
    while (outside(after, axis, bound, low))
    {
        if (++steps > 1032)
            return fail(c, BH_CLIP_TOPOLOGY);
        after = after->next;
    }
    if (c->used > 6)
        return fail(c, BH_CLIP_CAPACITY);
    first = c->scratch + c->used;
    second = first + 1;
    c->used += 2;
    if (!intersect(first, before, before->next, axis, bound, depth) ||
        !intersect(second, after, after->previous, axis, bound, depth))
        return fail(c, BH_CLIP_NUMERICAL);
    first->previous = before;
    first->next = second;
    second->previous = first;
    second->next = after;
    before->next = first;
    after->previous = second;
    return first;
}
BHClipVertex *__cdecl bh_screen_clip_polygon(BHScreenClipContext *c, BHClipVertex *head, float depth)
{
    BHClipVertex *p, *minimum, *maximum;
    unsigned long n = 0, i, axis;
    float low, high;
    if (!c)
        return NULL;
    c->used = 0;
    c->error = BH_CLIP_OK;
    if (!_finite(depth))
        return fail(c, BH_CLIP_NUMERICAL);
    /* Original rejects uniform depth before dereferencing the input ring. */
    if (c->far_z < depth || depth < c->near_z)
        return NULL;
    if (!head)
        return fail(c, BH_CLIP_ARGUMENT);
    if (!_finite(c->near_z) || !_finite(c->far_z) || !_finite(c->left) || !_finite(c->right) || !_finite(c->top) ||
        !_finite(c->bottom) || c->near_z <= 0 || c->far_z < c->near_z || c->right < c->left || c->bottom < c->top)
        return fail(c, BH_CLIP_ARGUMENT);
    p = head;
    do
    {
        for (i = 0; i < 8; ++i)
            if (p == c->scratch + i)
                return fail(c, BH_CLIP_ARGUMENT);
        if (!p->next || !p->previous || p->next->previous != p || p->previous->next != p || ++n > 1024)
            return fail(c, BH_CLIP_TOPOLOGY);
        if (!_finite(p->sx) || !_finite(p->sy) || !_finite(p->u) || !_finite(p->v))
            return fail(c, BH_CLIP_NUMERICAL);
        p = p->next;
    } while (p != head);
    for (axis = 0; axis < 2; ++axis)
    {
        low = axis ? c->top : c->left;
        high = axis ? c->bottom : c->right;
        extrema(head, axis, &minimum, &maximum);
        if (high < coordinate(minimum, axis) || coordinate(maximum, axis) < low)
            return NULL;
        if (coordinate(minimum, axis) < low)
        {
            head = cut(c, minimum, axis, low, 1, depth);
            if (!head)
                return NULL;
        }
        if (high < coordinate(maximum, axis))
        {
            head = cut(c, maximum, axis, high, 0, depth);
            if (!head)
                return NULL;
        }
    }
    return head;
}
