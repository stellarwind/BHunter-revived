#include "bh_attribute_clipper.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
#include <float.h>
typedef char attribute_vertex_size[sizeof(BHClipVertex) == 0x3c ? 1 : -1];
typedef char attribute_vertex_point[offsetof(BHClipVertex, point) == 0xc ? 1 : -1];
typedef char attribute_vertex_z[offsetof(BHClipVertex, point) + offsetof(BHVec3, z) == 0x14 ? 1 : -1];
typedef char attribute_vertex_uv[offsetof(BHClipVertex, u) == 0x18 && offsetof(BHClipVertex, v) == 0x1c ? 1 : -1];
typedef char attribute_vertex_screen[offsetof(BHClipVertex, sx) == 0x20 && offsetof(BHClipVertex, sy) == 0x24 ? 1 : -1];
typedef char attribute_vertex_scalar[offsetof(BHClipVertex, unknown28) == 0x28 ? 1 : -1];
typedef char
    attribute_vertex_links[offsetof(BHClipVertex, next) == 0x2c && offsetof(BHClipVertex, previous) == 0x30 ? 1 : -1];
typedef char attribute_vertex_projected[offsetof(BHClipVertex, projected) == 0x38 ? 1 : -1];
static BHClipVertex *fail(BHAttributeClipContext *c, BHClipError error)
{
    if (c)
        c->error = error;
    return NULL;
}
static float scalar(const BHClipVertex *p)
{
    float value;
    memcpy(&value, &p->unknown28, 4);
    return value;
}
static void store_scalar(BHClipVertex *p, double value)
{
    float stored;
    bh_round_float(&stored, value);
    memcpy(&p->unknown28, &stored, 4);
}
int __cdecl bh_attribute_clip_init(BHAttributeClipContext *c, float near_z, float far_z, float left, float right,
                                   float top, float bottom)
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
    return axis == 0 ? p->point.z : axis == 1 ? p->sx : p->sy;
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
static int depth_intersect(BHClipVertex *out, const BHClipVertex *a, const BHClipVertex *b, float bound)
{
    double da = (double)a->point.z - bound, db = (double)bound - b->point.z, d = (double)a->point.z - b->point.z;
    out->projected = 0;
    bh_round_float(&out->point.x, (db * a->point.x + (double)b->point.x * da) / d);
    bh_round_float(&out->point.y, (db * a->point.y + (double)b->point.y * da) / d);
    store_scalar(out, (db * scalar(a) + da * scalar(b)) / d);
    bh_round_float(&out->u, (db * a->u + da * b->u) / d);
    bh_round_float(&out->v, (db * a->v + da * b->v) / d);
    out->point.z = bound;
    return _finite(out->point.x) && _finite(out->point.y) && _finite(out->u) && _finite(out->v) && _finite(scalar(out));
}
static int side_intersect(BHClipVertex *out, const BHClipVertex *a, const BHClipVertex *b, unsigned long axis,
                          float bound)
{
    double av = coordinate(a, axis), bv = coordinate(b, axis), da = av - bound, db = (double)bound - bv, d = av - bv,
           inverse;
    float stored_da, stored_db, stored_d;
    if (a->point.z == 0 || b->point.z == 0)
        return 0;
    out->projected = 0;
    /* Original FST retains da for reciprocal Z; U consumes three stored weights.
       V, other screen coordinate and scalar28 recompute unspilled weights. */
    bh_round_float(&stored_da, da);
    bh_round_float(&stored_db, db);
    bh_round_float(&stored_d, d);
    inverse = (da * (1.0 / b->point.z) - stored_db * (-1.0 / a->point.z)) / stored_d;
    bh_round_float(&out->u, (((double)b->u / b->point.z) * stored_da + ((double)a->u / a->point.z) * stored_db) /
                                stored_d / inverse);
    bh_round_float(&out->point.z, 1.0 / inverse);
    bh_round_float(&out->v, (((double)a->v / a->point.z) * db + da * ((double)b->v / b->point.z)) / d / inverse);
    if (axis == 1)
    {
        out->sx = bound;
        bh_round_float(&out->sy, (db * a->sy + da * b->sy) / d);
    }
    else
    {
        out->sy = bound;
        bh_round_float(&out->sx, (db * a->sx + da * b->sx) / d);
    }
    store_scalar(out, (db * scalar(a) + da * scalar(b)) / d);
    return _finite(out->point.z) && _finite(out->u) && _finite(out->v) && _finite(out->sx) && _finite(out->sy) &&
           _finite(scalar(out));
}
static BHClipVertex *cut(BHAttributeClipContext *c, BHClipVertex *extreme, unsigned long axis, float bound, int low)
{
    BHClipVertex *before = extreme->previous, *after = extreme->next, *first, *second;
    unsigned long steps = 0;
    while (outside(before, axis, bound, low))
    {
        if (++steps > 1036)
            return fail(c, BH_CLIP_TOPOLOGY);
        before = before->previous;
    }
    steps = 0;
    while (outside(after, axis, bound, low))
    {
        if (++steps > 1036)
            return fail(c, BH_CLIP_TOPOLOGY);
        after = after->next;
    }
    if (c->used > 10)
        return fail(c, BH_CLIP_CAPACITY);
    first = c->scratch + c->used;
    second = first + 1;
    c->used += 2;
    if (axis == 0)
    {
        if (!depth_intersect(first, before, before->next, bound) ||
            !depth_intersect(second, after, after->previous, bound))
            return fail(c, BH_CLIP_NUMERICAL);
    }
    else if (!side_intersect(first, before, before->next, axis, bound) ||
             !side_intersect(second, after, after->previous, axis, bound))
        return fail(c, BH_CLIP_NUMERICAL);
    first->previous = before;
    first->next = second;
    second->previous = first;
    second->next = after;
    before->next = first;
    after->previous = second;
    return first;
}
BHClipVertex *__cdecl bh_attribute_clip_polygon(BHAttributeClipContext *c, BHClipVertex *head, int project_depth)
{
    BHClipVertex *p, *minimum, *maximum;
    unsigned long n = 0, i, axis;
    float low, high;
    if (!c)
        return NULL;
    c->used = 0;
    c->error = BH_CLIP_OK;
    if (!head)
        return fail(c, BH_CLIP_ARGUMENT);
    if (!_finite(c->near_z) || !_finite(c->far_z) || !_finite(c->left) || !_finite(c->right) || !_finite(c->top) ||
        !_finite(c->bottom) || c->near_z <= 0 || c->far_z < c->near_z || c->right < c->left || c->bottom < c->top)
        return fail(c, BH_CLIP_ARGUMENT);
    p = head;
    do
    {
        for (i = 0; i < 12; ++i)
            if (p == c->scratch + i)
                return fail(c, BH_CLIP_ARGUMENT);
        if (!p->next || !p->previous || p->next->previous != p || p->previous->next != p || ++n > 1024)
            return fail(c, BH_CLIP_TOPOLOGY);
        if (!_finite(p->point.z) || !_finite(p->u) || !_finite(p->v) || !_finite(scalar(p)) ||
            (project_depth && (!_finite(p->point.x) || !_finite(p->point.y))) ||
            ((!project_depth || p->projected) && (!_finite(p->sx) || !_finite(p->sy))))
            return fail(c, BH_CLIP_NUMERICAL);
        p = p->next;
    } while (p != head);
    if (project_depth)
    {
        extrema(head, 0, &minimum, &maximum);
        if (c->far_z < minimum->point.z || maximum->point.z < c->near_z)
            return NULL;
        if (minimum->point.z < c->near_z)
        {
            head = cut(c, minimum, 0, c->near_z, 1);
            if (!head)
                return NULL;
        }
        if (c->far_z < maximum->point.z)
        {
            head = cut(c, maximum, 0, c->far_z, 0);
            if (!head)
                return NULL;
        }
        p = head;
        do
        {
            if (!p->projected)
            {
                p->projected = 1;
                bh_round_float(&p->sx, (double)p->point.x / p->point.z);
                bh_round_float(&p->sy, (double)p->point.y / p->point.z);
            }
            if (!_finite(p->sx) || !_finite(p->sy))
                return fail(c, BH_CLIP_NUMERICAL);
            p = p->next;
        } while (p != head);
    }
    for (axis = 1; axis <= 2; ++axis)
    {
        low = axis == 1 ? c->left : c->top;
        high = axis == 1 ? c->right : c->bottom;
        extrema(head, axis, &minimum, &maximum);
        if (high < coordinate(minimum, axis) || coordinate(maximum, axis) < low)
            return NULL;
        if (coordinate(minimum, axis) < low)
        {
            head = cut(c, minimum, axis, low, 1);
            if (!head)
                return NULL;
        }
        if (high < coordinate(maximum, axis))
        {
            head = cut(c, maximum, axis, high, 0);
            if (!head)
                return NULL;
        }
    }
    return head;
}
