#include "bh_sweep.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <string.h>

int __cdecl bh_sweep_init(BHSweepQuery *q, BHGeometry *a, BHGeometry *b)
{
    BHSweepQuery next;
    if (!q)
        return 0;
    memset(&next, 0, sizeof(next));
    next.a = a;
    next.b = b;
    if (a && b && !bh_collision_query_init(&next.collision, a, b))
        return 0;
    if (b && !bh_segment_query_init(&next.segment, b))
    {
        bh_collision_query_dispose(&next.collision);
        return 0;
    }
    next.initialized = 1;
    *q = next;
    return 1;
}
int __cdecl bh_sweep_dispose(BHSweepQuery *q)
{
    if (!q || q->busy || q->collision.busy || q->segment.busy)
        return 0;
    bh_collision_query_dispose(&q->collision);
    bh_segment_query_dispose(&q->segment);
    memset(q, 0, sizeof(*q));
    return 1;
}
static void apply(BHMat34 *pose, const BHVec3 *movement, const BHVec3 *angles)
{
    bh_mat34_translate_world(movement->x, movement->y, movement->z, pose);
    bh_mat34_rotate_x(angles->x, pose);
    bh_mat34_rotate_y(angles->y, pose);
    bh_mat34_rotate_z(angles->z, pose);
}
static void scale(const BHVec3 *v, float fraction, BHVec3 *out)
{
    bh_round_float(&out->x, (double)v->x * fraction);
    bh_round_float(&out->y, (double)v->y * fraction);
    bh_round_float(&out->z, (double)v->z * fraction);
}
static void midpoint(const BHVec3 *a, const BHVec3 *b, BHVec3 *out)
{
    bh_round_float(&out->x, ((double)a->x + b->x) * .5);
    bh_round_float(&out->y, ((double)a->y + b->y) * .5);
    bh_round_float(&out->z, ((double)a->z + b->z) * .5);
}
static int mesh(BHSweepQuery *q, const BHMat34 *pose, const BHMat34 *world, const BHMat34 *local, BHMat34 *relative,
                BHCollisionHit *out)
{
    int result;
    if (!q->a || !q->b)
        return 0;
    bh_mat34_multiply(local, pose, relative);
    result = bh_collision_query_run(&q->collision, 0, 0, relative, world, out);
    if (result < 0)
        q->error = q->collision.error;
    return result;
}
static int sweep(BHSweepQuery *q, BHMat34 *pose, const BHVec3 *movement, const BHVec3 *angles, const BHMat34 *world,
                 const BHMat34 *local, int fallback, BHSweepHit *out)
{
    const BHVec3 zero = {0, 0, 0};
    BHMat34 start = *pose, full, relative;
    BHVec3 clear = zero, clear_angles = zero, blocked = *movement, blocked_angles = *angles;
    BHVec3 mid, mid_angles, point, from, to, scaled_movement, scaled_angles;
    BHFaceContact contact;
    BHSegmentHit segment;
    float low = 0, high = 1;
    int colliding, result, i;
    apply(pose, movement, angles);
    colliding = mesh(q, pose, world, local, &relative, &out->surface);
    if (colliding < 0)
        return -1;
    out->fraction = 1;
    out->surface_present = colliding;
    if (!colliding)
    {
        if (!fallback || !q->b)
            return 0;
        bh_mat34_transform_point(&start, &zero, &from);
        bh_mat34_transform_point(pose, &zero, &to);
        result = bh_segment_query_run(&q->segment, &from, &to, world, local, &segment);
        if (result < 0)
        {
            q->error = q->segment.error;
            return -1;
        }
        if (!result)
            return 0;
        scale(movement, segment.fraction, &scaled_movement);
        scale(angles, segment.fraction, &scaled_angles);
        *pose = start;
        /* Original recursion disables the global fallback flag for one call.
           A local argument gives the same bound without shared mutable state. */
        result = sweep(q, pose, &scaled_movement, &scaled_angles, world, local, 0, out);
        if (result < 0)
            return -1;
        bh_round_float(&out->fraction, (double)out->fraction * segment.fraction);
        return 1;
    }
    full = *pose;
    for (i = 0; i < 3; ++i)
    {
        midpoint(&blocked, &clear, &mid);
        midpoint(&blocked_angles, &clear_angles, &mid_angles);
        *pose = start;
        apply(pose, &mid, &mid_angles);
        if (colliding)
        {
            result = bh_collision_cached_contact(q->a->bsp, out->surface.a_face, pose, q->b->bsp, out->surface.b_face,
                                                 out->surface.b_node, world, local, &out->surface.normal, &contact);
            if (result < 0)
            {
                q->error = BH_BSP_FORMAT;
                return -1;
            }
            colliding = result;
            if (result)
                out->surface.contact = contact;
        }
        if (!colliding)
        {
            colliding = mesh(q, pose, world, local, &relative, &out->surface);
            if (colliding < 0)
                return -1;
        }
        if (colliding)
        {
            blocked = mid;
            blocked_angles = mid_angles;
            bh_round_float(&high, ((double)high + low) * .5);
        }
        else
        {
            clear = mid;
            clear_angles = mid_angles;
            bh_round_float(&low, ((double)high + low) * .5);
        }
    }
    out->fraction = low;
    bh_mat34_inverse_transform_point(pose, &out->surface.contact.raw_point, &point);
    bh_mat34_transform_point(&start, &point, &from);
    bh_mat34_transform_point(&full, &point, &to);
    bh_round_float(&out->motion.x, (double)to.x - from.x);
    bh_round_float(&out->motion.y, (double)to.y - from.y);
    bh_round_float(&out->motion.z, (double)to.z - from.z);
    *pose = start;
    apply(pose, &clear, &clear_angles);
    return 1;
}
int __cdecl bh_sweep_run(BHSweepQuery *q, BHMat34 *pose, const BHVec3 *movement, const BHVec3 *angles,
                         const BHMat34 *world, const BHMat34 *local, int fallback, BHSweepHit *out)
{
    BHMat34 next;
    BHSweepHit hit;
    unsigned long i;
    int result;
    if (!q || !q->initialized || q->busy || !pose || !movement || !angles || !world || !local || !out ||
        !bh_math_environment_supported())
        return -1;
    q->error = BH_BSP_OK;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)pose)[i]) || !_finite(((const float *)world)[i]) ||
            !_finite(((const float *)local)[i]))
        {
            q->error = BH_BSP_NUMERICAL;
            return -1;
        }
    for (i = 0; i < 3; ++i)
        if (!_finite(((const float *)movement)[i]) || !_finite(((const float *)angles)[i]))
        {
            q->error = BH_BSP_NUMERICAL;
            return -1;
        }
    q->busy = 1;
    next = *pose;
    hit = *out;
    result = sweep(q, &next, movement, angles, world, local, fallback, &hit);
    if (result >= 0)
    {
        *pose = next;
        *out = hit;
    }
    q->busy = 0;
    return result;
}
