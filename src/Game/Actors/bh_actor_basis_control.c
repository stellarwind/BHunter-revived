#include "bh_actor_basis_control.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <float.h>

static float stored(double value)
{
    float result;
    bh_round_float(&result, value);
    return result;
}
static int vector_ok(const BHVec3 *vector)
{
    return vector && _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static int search(const BHActorBasisGroup *g, const BHActorBasisScale *s, unsigned long n, const BHVec3 *vector, int *first,
                  int *second, float *a, float *b, int torque)
{
    unsigned long i, j;
    float best, p, q, d, den, x, y;
    double wa, wb, rx, ry, rz, length;
    const BHVec3 *first_unit, *second_unit, *raw_u, *raw_w;
    *a = 0;
    *b = 0;
    *first = 0;
    *second = 0;
    best = stored(sqrt(((double)vector->x * vector->x + (double)vector->z * vector->z) + (double)vector->y * vector->y));
    for (i = 0; i < n; ++i)
        for (j = i + 1; j < n; ++j)
        {
            first_unit = torque ? &g[i].unit_torque : &g[i].unit_direction;
            second_unit = torque ? &g[j].unit_torque : &g[j].unit_direction;
            raw_u = torque ? &g[i].torque : &g[i].direction;
            raw_w = torque ? &g[j].torque : &g[j].direction;
            p = stored(((double)first_unit->x * vector->x + (double)first_unit->y * vector->y) + (double)first_unit->z * vector->z);
            q = stored(((double)second_unit->x * vector->x + (double)second_unit->z * vector->z) + (double)second_unit->y * vector->y);
            d = stored(((double)first_unit->x * second_unit->x + (double)first_unit->y * second_unit->y) + (double)first_unit->z * second_unit->z);
            den = stored(1.0 - (double)d * d);
            if (!(den > 0))
                continue;
            x = stored(((double)d * q - p) / den);
            if (!(x >= 0))
                continue;
            y = stored(((double)d * p - q) / den);
            if (!(y >= 0) || !((double)x + y > 0))
                continue;
            wa = (double)x * (torque ? s[i].torque : s[i].direction);
            x = stored(wa);
            if (wa > 1000000.0)
                x = 1000000;
            wb = (double)y * (torque ? s[j].torque : s[j].direction);
            if (wb > 1000000.0)
                wb = 1000000;
            rx = (double)raw_u->x * x + (double)raw_w->x * wb + vector->x;
            ry = (double)vector->y + (double)raw_u->y * x + (double)raw_w->y * wb;
            rz = (double)vector->z + (double)raw_u->z * x + (double)raw_w->z * wb;
            length = sqrt(rx * stored(rx) + ry * stored(ry) + rz * stored(rz));
            if (length < best)
            {
                *first = (int)i;
                *second = (int)j;
                *a = x;
                *b = stored(wb);
                best = stored(length);
            }
        }
    if (!*first && !*second)
        return 0;
    return 1;
}
int bh_actor_basis_a_pair(const BHActorBasisContext *ctx, const BHVec3 *v, int *i, int *j, float *a, float *b)
{
    if (!ctx || !vector_ok(v) || !i || !j || !a || !b || ctx->a_count > ctx->a_capacity ||
        (ctx->a_count && (!ctx->groups_a || !ctx->scales_a)))
        return 0;
    return search(ctx->groups_a, ctx->scales_a, ctx->a_count, v, i, j, a, b, 1);
}
int bh_actor_basis_b_pair(const BHActorBasisContext *ctx, const BHVec3 *v, int *i, int *j, float *a, float *b)
{
    if (!ctx || !vector_ok(v) || !i || !j || !a || !b || ctx->b_count > ctx->b_capacity ||
        (ctx->b_count && (!ctx->groups_b || !ctx->scales_b)))
        return 0;
    return -search(ctx->groups_b, ctx->scales_b, ctx->b_count, v, i, j, a, b, 0);
}
int bh_actor_basis_apply_group(const BHActorBasisContext *ctx, BHPhysicsState *state, float mass, const BHVec3 *inertia,
                               float accumulated[15], const BHActorBasisIndices *group, float value)
{
    long i, index;
    BHVec3 force, torque;
    const BHActorBasisSample *sample;
    unsigned long row;
    double reciprocal;
    if (!ctx || !state || !accumulated || !group || !ctx->samples || ctx->sample_count > ctx->sample_capacity ||
        group->count < 1 || group->count > 16 || !vector_ok(inertia) || !mass || !_finite(mass) || !inertia->x ||
        !inertia->y || !inertia->z || !_finite(value))
        return 0;
    for (i = 0; i < group->count - 1; ++i)
        if (group->sample[i] < 0 || group->sample[i] >= 15 || (unsigned long)group->sample[i] >= ctx->sample_count)
            return 0;
    for (i = 0; i < group->count - 1; ++i)
    {
        index = group->sample[i];
        sample = ctx->samples + index;
        bh_round_float(accumulated + index, (double)value + accumulated[index]);
        reciprocal = 1.0 / mass;
        force.x = stored((double)sample->direction.x * value);
        force.y = stored((double)sample->direction.y * value);
        force.z = stored((double)sample->direction.z * value);
        for (row = 0; row < 3; ++row)
            bh_round_float(((float *)&state->acceleration) + row,
                           ((float *)&state->acceleration)[row] -
                               (((double)state->pose.m[row][0] * force.x + (double)state->pose.m[row][1] * force.y) +
                                (double)state->pose.m[row][2] * force.z) *
                                   reciprocal);
        bh_vec3_cross(&sample->point, &force, &torque);
        torque.x = stored((double)torque.x / inertia->x);
        torque.y = stored((double)torque.y / inertia->y);
        torque.z = stored((double)torque.z / inertia->z);
        for (row = 0; row < 3; ++row)
            bh_round_float(((float *)&state->angular_acceleration) + row,
                           (double)((float *)&state->angular_acceleration)[row] - ((float *)&torque)[row]);
    }
    return 1;
}
