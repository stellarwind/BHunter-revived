#include "bh_clipper.h"
#include "bh_matrix_inverse.h"
#include "bh_round.h"
#include <string.h>
#include <stddef.h>
#include <float.h>
typedef char vertex_size[sizeof(BHClipVertex) == 60 ? 1 : -1];
typedef char vertex_next[offsetof(BHClipVertex, next) == 0x2c ? 1 : -1];
typedef char vertex_cache[offsetof(BHClipVertex, projected) == 0x38 ? 1 : -1];
static BHClipVertex *fail(BHClipContext *c, BHClipError e)
{
    if (c)
        c->error = e;
    return NULL;
}
static void identity(BHMat34 *m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}
int __cdecl bh_clip_init(BHClipContext *c, float near_z, float far_z, float left, float right, float top, float bottom)
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
    identity(&c->camera);
    identity(&c->inverse);
    return 1;
}
int __cdecl bh_clip_set_camera(BHClipContext *c, const BHMat34 *camera)
{
    unsigned long i;
    if (!c || !camera)
        return 0;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)camera)[i]))
            return 0;
    c->camera = *camera;
    c->inverse_ready = 0;
    return 1;
}
static float coordinate(const BHClipVertex *p, unsigned long axis)
{
    return axis == 0 ? p->point.z : axis == 1 ? p->sx : p->sy;
}
static int outside(const BHClipVertex *p, unsigned long axis, float bound, int low)
{
    float v = coordinate(p, axis);
    return low ? v < bound : bound < v;
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
static void depth_intersection(BHClipVertex *out, const BHClipVertex *a, const BHClipVertex *b, float bound)
{
    double da = (double)a->point.z - bound, db = (double)bound - b->point.z, d = (double)a->point.z - b->point.z;
    bh_round_float(&out->point.x, (db * a->point.x + (double)b->point.x * da) / d);
    bh_round_float(&out->point.y, (db * a->point.y + (double)b->point.y * da) / d);
    bh_round_float(&out->u, (db * a->u + da * b->u) / d);
    bh_round_float(&out->v, (db * a->v + da * b->v) / d);
    out->point.z = bound;
}
static void side_intersection(BHClipVertex *out, const BHClipVertex *a, const BHClipVertex *b, unsigned long axis,
                              float bound)
{
    double av = coordinate(a, axis), bv = coordinate(b, axis), da = av - bound, db = (double)bound - bv, d = av - bv,
           inverse;
    float stored_da, stored_db, stored_d;
    /* Original spills these three values, but retains da for reciprocal-Z.
       U consumes the spills; V and the other screen axis recompute weights. */
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
}
static BHClipVertex *cut(BHClipContext *c, BHClipVertex *extreme, unsigned long axis, float bound, int low)
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
BHClipVertex *__cdecl bh_clip_polygon(BHClipContext *c, BHClipVertex *head, const BHTextureImage *image)
{
    BHClipVertex *p, *minimum, *maximum;
    unsigned long n = 0, axis, i;
    float coefficients[6], low, high;
    BHVec3 world;
    if (!c || !head)
        return fail(c, BH_CLIP_ARGUMENT);
    c->used = 0;
    c->error = BH_CLIP_OK;
    /* Valid pointer storage is a caller contract; link consistency/count and
       numerical checks reject malformed source data before modifying the ring. */
    p = head;
    do
    {
        if (!p->next || !p->previous || p->next->previous != p || p->previous->next != p || ++n > 1024)
            return fail(c, BH_CLIP_TOPOLOGY);
        if (!_finite(p->point.x) || !_finite(p->point.y) || !_finite(p->point.z) || !_finite(p->u) || !_finite(p->v) ||
            (p->projected && (!_finite(p->sx) || !_finite(p->sy))))
            return fail(c, BH_CLIP_NUMERICAL);
        p = p->next;
    } while (p != head);
    if (image && image->unused54)
    {
        memcpy(coefficients, image->padding, sizeof(coefficients));
        for (i = 0; i < 6; ++i)
            if (!_finite(coefficients[i]))
                return fail(c, BH_CLIP_NUMERICAL);
        if (!c->inverse_ready)
        {
            if (!bh_mat34_inverse_general(&c->camera, &c->inverse))
                return fail(c, BH_CLIP_NUMERICAL);
            c->inverse_ready = 1;
        }
        p = head;
        do
        {
            bh_mat34_transform_point(&c->inverse, &p->point, &world);
            bh_round_float(&p->u,
                           ((double)world.z * coefficients[1] + (double)world.x * coefficients[0]) + coefficients[2]);
            bh_round_float(&p->v,
                           ((double)world.z * coefficients[4] + (double)world.x * coefficients[3]) + coefficients[5]);
            if (!_finite(p->u) || !_finite(p->v))
                return fail(c, BH_CLIP_NUMERICAL);
            p = p->next;
        } while (p != head);
    }
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
        p = p->next;
    } while (p != head);
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
