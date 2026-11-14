#include "bh_sweep.h"
#include "../Math/bh_rotation.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <float.h>
#include <stddef.h>
#include <string.h>

static_assert(sizeof(BHSweepQuery) == 108, "sweep query size");
static_assert(offsetof(BHSweepQuery, moving_geometry) == 0, "moving geometry offset");
static_assert(offsetof(BHSweepQuery, target_geometry) == 4, "target geometry offset");
static_assert(offsetof(BHSweepQuery, collision) == 8, "collision workspace offset");

int bh_sweep_init(BHSweepQuery *query, BHGeometry *moving_geometry, BHGeometry *target_geometry)
{
    BHSweepQuery initialized_query;
    if (!query)
        return 0;
    memset(&initialized_query, 0, sizeof(initialized_query));
    initialized_query.moving_geometry = moving_geometry;
    initialized_query.target_geometry = target_geometry;
    if (moving_geometry && target_geometry &&
        !bh_collision_query_init(&initialized_query.collision, moving_geometry, target_geometry))
        return 0;
    if (target_geometry && !bh_segment_query_init(&initialized_query.segment, target_geometry))
    {
        bh_collision_query_dispose(&initialized_query.collision);
        return 0;
    }
    initialized_query.initialized = 1;
    *query = initialized_query;
    return 1;
}
int bh_sweep_dispose(BHSweepQuery *query)
{
    if (!query || query->busy || query->collision.busy || query->segment.busy)
        return 0;
    bh_collision_query_dispose(&query->collision);
    bh_segment_query_dispose(&query->segment);
    memset(query, 0, sizeof(*query));
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
static void angle_midpoint(const BHVec3 *a, const BHVec3 *b, BHVec3 *out)
{
    float x_sum;
    /* 0044dda8/0044ddb4 spill only the angle-X sum before halving. */
    bh_round_float(&x_sum, (double)a->x + b->x);
    bh_round_float(&out->x, (double)x_sum * .5);
    bh_round_float(&out->y, ((double)a->y + b->y) * .5);
    bh_round_float(&out->z, ((double)a->z + b->z) * .5);
}
static int mesh(BHSweepQuery *query, const BHMat34 *pose, const BHMat34 *world, const BHMat34 *local, BHMat34 *relative,
                BHCollisionHit *out)
{
    int result;
    if (!query->moving_geometry || !query->target_geometry)
        return 0;
    bh_mat34_multiply(local, pose, relative);
    result = bh_collision_query_run(&query->collision, 0, 0, relative, world, out);
    if (result < 0)
        query->error = query->collision.error;
    return result;
}
static int sweep(BHSweepQuery *query, BHMat34 *pose, const BHVec3 *movement, const BHVec3 *angles, const BHMat34 *world,
                 const BHMat34 *local, int fallback, BHSweepHit *out)
{
    const BHVec3 zero = {0, 0, 0};
    BHMat34 start = *pose, full, relative;
    BHVec3 clear_movement = zero, clear_angles = zero, blocked_movement = *movement, blocked_angles = *angles;
    BHVec3 mid_movement, mid_angles, point, from, to, scaled_movement, scaled_angles;
    BHFaceContact contact;
    BHSegmentHit segment;
    float clear_fraction = 0, blocked_fraction = 1;
    int colliding, result, i;
    apply(pose, movement, angles);
    colliding = mesh(query, pose, world, local, &relative, &out->surface);
    if (colliding < 0)
        return -1;
    out->fraction = 1;
    out->surface_present = colliding;
    if (!colliding)
    {
        if (!fallback || !query->target_geometry)
            return 0;
        bh_mat34_transform_point(&start, &zero, &from);
        bh_mat34_transform_point(pose, &zero, &to);
        result = bh_segment_query_run(&query->segment, &from, &to, world, local, &segment);
        if (result < 0)
        {
            query->error = query->segment.error;
            return -1;
        }
        if (!result)
            return 0;
        scale(movement, segment.fraction, &scaled_movement);
        scale(angles, segment.fraction, &scaled_angles);
        *pose = start;
        /* Original recursion disables the global fallback flag for one call.
           A local argument gives the same bound without shared mutable state. */
        result = sweep(query, pose, &scaled_movement, &scaled_angles, world, local, 0, out);
        if (result < 0)
            return -1;
        bh_round_float(&out->fraction, (double)out->fraction * segment.fraction);
        return 1;
    }
    full = *pose;
    for (i = 0; i < 3; ++i)
    {
        midpoint(&blocked_movement, &clear_movement, &mid_movement);
        angle_midpoint(&blocked_angles, &clear_angles, &mid_angles);
        *pose = start;
        apply(pose, &mid_movement, &mid_angles);
        if (colliding)
        {
            result = bh_collision_cached_contact(query->moving_geometry->bsp, out->surface.a_face, pose,
                                                 query->target_geometry->bsp, out->surface.b_face, out->surface.b_node,
                                                 world, local, &out->surface.normal, &contact);
            if (result < 0)
            {
                query->error = BH_BSP_FORMAT;
                return -1;
            }
            colliding = result;
            if (result)
                out->surface.contact = contact;
        }
        if (!colliding)
        {
            colliding = mesh(query, pose, world, local, &relative, &out->surface);
            if (colliding < 0)
                return -1;
        }
        if (colliding)
        {
            blocked_movement = mid_movement;
            blocked_angles = mid_angles;
            bh_round_float(&blocked_fraction, ((double)blocked_fraction + clear_fraction) * .5);
        }
        else
        {
            clear_movement = mid_movement;
            clear_angles = mid_angles;
            bh_round_float(&clear_fraction, ((double)blocked_fraction + clear_fraction) * .5);
        }
    }
    out->fraction = clear_fraction;
    bh_mat34_inverse_transform_point(pose, &out->surface.contact.raw_point, &point);
    bh_mat34_transform_point(&start, &point, &from);
    bh_mat34_transform_point(&full, &point, &to);
    bh_round_float(&out->motion.x, (double)to.x - from.x);
    bh_round_float(&out->motion.y, (double)to.y - from.y);
    bh_round_float(&out->motion.z, (double)to.z - from.z);
    *pose = start;
    apply(pose, &clear_movement, &clear_angles);
    return 1;
}
int bh_sweep_run(BHSweepQuery *query, BHMat34 *pose, const BHVec3 *movement, const BHVec3 *angles, const BHMat34 *world,
                 const BHMat34 *local, int fallback, BHSweepHit *out)
{
    BHMat34 next;
    BHSweepHit hit;
    unsigned long i;
    int result;
    if (!query || !query->initialized || query->busy || !pose || !movement || !angles || !world || !local || !out ||
        !bh_math_environment_supported())
        return -1;
    query->error = BH_BSP_OK;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)pose)[i]) || !_finite(((const float *)world)[i]) ||
            !_finite(((const float *)local)[i]))
        {
            query->error = BH_BSP_NUMERICAL;
            return -1;
        }
    for (i = 0; i < 3; ++i)
        if (!_finite(((const float *)movement)[i]) || !_finite(((const float *)angles)[i]))
        {
            query->error = BH_BSP_NUMERICAL;
            return -1;
        }
    query->busy = 1;
    next = *pose;
    hit = *out;
    result = sweep(query, &next, movement, angles, world, local, fallback, &hit);
    if (result >= 0)
    {
        *pose = next;
        *out = hit;
    }
    query->busy = 0;
    return result;
}
