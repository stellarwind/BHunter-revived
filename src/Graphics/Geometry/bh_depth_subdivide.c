#include "bh_depth_subdivide.h"
#include "../../Math/bh_round.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
static_assert(offsetof(BHClipVertex, point) + offsetof(BHVec3, z) == 0x14, "depth layout z");
static_assert(offsetof(BHClipVertex, u) == 0x18, "BHClipVertex.u offset");
static_assert(offsetof(BHClipVertex, unknown28) == 0x28, "BHClipVertex.unknown28 offset");
static_assert(offsetof(BHClipVertex, next) == 0x2c && offsetof(BHClipVertex, previous) == 0x30, "depth layout links");
static int fail(BHDepthSplitContext *context, BHDepthSplitError e)
{
    if (context)
        context->error = e;
    return 0;
}
static float scalar(const BHClipVertex *vertex)
{
    float value;
    memcpy(&value, &vertex->unknown28, 4);
    return value;
}
static int valid_ring(BHDepthSplitContext *c, BHClipVertex *ring, int attribute)
{
    BHClipVertex *vertex = ring;
    unsigned long n = 0;
    if (!vertex)
        return fail(c, BH_DEPTH_SPLIT_ARGUMENT);
    do
    {
        if (!vertex || !vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++n > 1024)
            return fail(c, BH_DEPTH_SPLIT_TOPOLOGY);
        if (!_finite(vertex->point.z) || vertex->point.z <= 0 || !_finite(vertex->u) || !_finite(vertex->v) || !_finite(vertex->sx) ||
            !_finite(vertex->sy) || (attribute && !_finite(scalar(vertex))))
            return fail(c, BH_DEPTH_SPLIT_NUMERICAL);
        vertex = vertex->next;
    } while (vertex != ring);
    return n >= 3 ? 1 : fail(c, BH_DEPTH_SPLIT_TOPOLOGY);
}
int bh_depth_split_init(BHDepthSplitContext *context, float left, float right, float top, float bottom, BHDepthLeaf leaf,
                        void *user)
{
    if (!context || !leaf || !_finite(left) || !_finite(right) || !_finite(top) || !_finite(bottom) || left > right ||
        top > bottom)
        return 0;
    memset(context, 0, sizeof(*context));
    context->left = left;
    context->right = right;
    context->top = top;
    context->bottom = bottom;
    context->leaf = leaf;
    context->user = user;
    return 1;
}
/* a is below the cut, b is at/above it. The original plain owner retains
   delta for UV but stores delta*mid for screen division. The scalar owner
   stores delta before BOTH divisions and retains that stored delta*mid. */
static int intersection(BHDepthSplitContext *context, BHClipVertex *out, const BHClipVertex *first_vertex, const BHClipVertex *second_vertex,
                        float mid, int attribute)
{
    double wa = (double)second_vertex->point.z - mid, wb = (double)mid - first_vertex->point.z, delta = (double)second_vertex->point.z - first_vertex->point.z;
    double divisor, x, y;
    float stored, value;
    memset(out, 0, sizeof(*out));
    out->point.z = mid;
    if (attribute)
    {
        bh_round_float(&stored, delta);
        delta = stored;
    }
    bh_round_float(&out->u, ((double)first_vertex->u * wa + (double)second_vertex->u * wb) / delta);
    bh_round_float(&out->v, ((double)first_vertex->v * wa + (double)second_vertex->v * wb) / delta);
    if (attribute)
    {
        bh_round_float(&value, ((double)scalar(first_vertex) * wa + (double)scalar(second_vertex) * wb) / delta);
        memcpy(&out->unknown28, &value, 4);
    }
    divisor = delta * mid;
    if (!attribute)
    {
        bh_round_float(&stored, divisor);
        divisor = stored;
    }
    if (!_finite(divisor) || divisor == 0)
        return fail(context, BH_DEPTH_SPLIT_NUMERICAL);
    x = (((double)first_vertex->sx * wa) * first_vertex->point.z + ((double)second_vertex->sx * wb) * second_vertex->point.z) / divisor;
    y = (((double)first_vertex->sy * wa) * first_vertex->point.z + ((double)second_vertex->sy * wb) * second_vertex->point.z) / divisor;
    bh_round_float(&out->sx, x);
    bh_round_float(&out->sy, y);
    /* Scalar owner compares retained x to left after FST; all other bounds
       use the stored binary32 value, as do both plain-owner x comparisons. */
    if ((attribute ? x : (double)out->sx) < context->left)
        out->sx = context->left;
    else if (out->sx > context->right)
        out->sx = context->right;
    if (out->sy < context->top)
        out->sy = context->top;
    else if (out->sy > context->bottom)
        out->sy = context->bottom;
    if (!_finite(out->u) || !_finite(out->v) || !_finite(out->sx) || !_finite(out->sy) ||
        (attribute && !_finite(scalar(out))))
        return fail(context, BH_DEPTH_SPLIT_NUMERICAL);
    return 1;
}
static int split(BHDepthSplitContext *context, BHClipVertex *ring, int attribute, unsigned long depth)
{
    BHClipVertex *vertex, *minimum = ring, *maximum = ring, *rising_start, *rising_end, *falling_end, *falling_start;
    BHClipVertex low, high;
    double sum;
    float mid;
    int ok;
    if (depth > 256)
        return fail(context, BH_DEPTH_SPLIT_CAPACITY);
    if (depth > context->deepest)
        context->deepest = depth;
    vertex = ring->next;
    while (vertex != ring)
    {
        if (vertex->point.z < minimum->point.z)
            minimum = vertex;
        if (maximum->point.z < vertex->point.z)
            maximum = vertex;
        vertex = vertex->next;
    }
    sum = (double)minimum->point.z + maximum->point.z;
    if (((double)maximum->point.z - minimum->point.z) * 16.0 < sum)
    {
        if (context->leaves == 65536)
            return fail(context, BH_DEPTH_SPLIT_CAPACITY);
        if (!context->leaf(ring, context->user))
            return fail(context, BH_DEPTH_SPLIT_LEAF);
        ++context->leaves;
        return 1;
    }
    bh_round_float(&mid, sum * .5);
    if (!_finite(mid) || mid <= minimum->point.z || mid >= maximum->point.z)
        return fail(context, BH_DEPTH_SPLIT_NUMERICAL);
    rising_start = ring;
    while (mid <= rising_start->point.z || rising_start->next->point.z < mid)
        rising_start = rising_start->next;
    rising_end = rising_start->next;
    falling_start = rising_start;
    while (falling_start->point.z < mid || mid <= falling_start->next->point.z)
        falling_start = falling_start->next;
    falling_end = falling_start->next;
    if (!intersection(context, &low, rising_start, rising_end, mid, attribute) || !intersection(context, &high, falling_end, falling_start, mid, attribute))
        return 0;
    ++context->splits;
    rising_start->next = &low;
    low.previous = rising_start;
    low.next = &high;
    high.previous = &low;
    high.next = falling_end;
    falling_end->previous = &high;
    ok = split(context, &low, attribute, depth + 1);
    if (ok)
    {
        high.next = &low;
        rising_end->previous = &low;
        low.previous = &high;
        falling_start->next = &high;
        low.next = rising_end;
        high.previous = falling_start;
        ok = split(context, &low, attribute, depth + 1);
    }
    rising_start->next = rising_end;
    rising_end->previous = rising_start;
    falling_start->next = falling_end;
    falling_end->previous = falling_start;
    return ok;
}
static int run(BHDepthSplitContext *context, BHClipVertex *ring, int attribute)
{
    if (!context || !context->leaf || !_finite(context->left) || !_finite(context->right) || !_finite(context->top) || !_finite(context->bottom) ||
        context->left > context->right || context->top > context->bottom)
        return fail(context, BH_DEPTH_SPLIT_ARGUMENT);
    context->error = BH_DEPTH_SPLIT_OK;
    context->splits = context->leaves = context->deepest = 0;
    if (!valid_ring(context, ring, attribute))
        return 0;
    return split(context, ring, attribute, 0);
}
int bh_depth_subdivide(BHDepthSplitContext *c, BHClipVertex *ring)
{
    return run(c, ring, 0);
}
int bh_depth_subdivide_scalar(BHDepthSplitContext *c, BHClipVertex *ring)
{
    return run(c, ring, 1);
}
