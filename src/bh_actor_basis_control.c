#include "bh_actor_basis_control.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>

static float stored(double value)
{
    float result;
    bh_round_float(&result, value);
    return result;
}
static int vector_ok(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int search(const BHActorBasisGroup *g, const BHActorBasisScale *s, unsigned long n, const BHVec3 *v, int *first,
                  int *second, float *a, float *b, int torque)
{
    unsigned long i, j;
    float best, p, q, d, den, x, y;
    double wa, wb, rx, ry, rz, length;
    const BHVec3 *u, *w, *raw_u, *raw_w;
    *a = 0;
    *b = 0;
    *first = 0;
    *second = 0;
    best = stored(sqrt(((double)v->x * v->x + (double)v->z * v->z) + (double)v->y * v->y));
    for (i = 0; i < n; ++i)
        for (j = i + 1; j < n; ++j)
        {
            u = torque ? &g[i].unit_torque : &g[i].unit_direction;
            w = torque ? &g[j].unit_torque : &g[j].unit_direction;
            raw_u = torque ? &g[i].torque : &g[i].direction;
            raw_w = torque ? &g[j].torque : &g[j].direction;
            p = stored(((double)u->x * v->x + (double)u->y * v->y) + (double)u->z * v->z);
            q = stored(((double)w->x * v->x + (double)w->z * v->z) + (double)w->y * v->y);
            d = stored(((double)u->x * w->x + (double)u->y * w->y) + (double)u->z * w->z);
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
            rx = (double)raw_u->x * x + (double)raw_w->x * wb + v->x;
            ry = (double)v->y + (double)raw_u->y * x + (double)raw_w->y * wb;
            rz = (double)v->z + (double)raw_u->z * x + (double)raw_w->z * wb;
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
int __cdecl bh_actor_basis_a_pair(const BHActorBasisContext *c, const BHVec3 *v, int *i, int *j, float *a, float *b)
{
    if (!c || !vector_ok(v) || !i || !j || !a || !b || c->a_count > c->a_capacity ||
        (c->a_count && (!c->groups_a || !c->scales_a)))
        return 0;
    return search(c->groups_a, c->scales_a, c->a_count, v, i, j, a, b, 1);
}
int __cdecl bh_actor_basis_b_pair(const BHActorBasisContext *c, const BHVec3 *v, int *i, int *j, float *a, float *b)
{
    if (!c || !vector_ok(v) || !i || !j || !a || !b || c->b_count > c->b_capacity ||
        (c->b_count && (!c->groups_b || !c->scales_b)))
        return 0;
    return -search(c->groups_b, c->scales_b, c->b_count, v, i, j, a, b, 0);
}
int __cdecl bh_actor_basis_apply_group(const BHActorBasisContext *c, BHPhysicsState *state, float mass,
                                       const BHVec3 *inertia, float accumulated[15], const BHActorBasisIndices *group,
                                       float value)
{
    long i, index;
    BHVec3 force, torque;
    const BHActorBasisSample *sample;
    unsigned long row;
    double reciprocal;
    if (!c || !state || !accumulated || !group || !c->samples || c->sample_count > c->sample_capacity ||
        group->count < 1 || group->count > 16 || !vector_ok(inertia) || !mass || !_finite(mass) || !inertia->x ||
        !inertia->y || !inertia->z || !_finite(value))
        return 0;
    for (i = 0; i < group->count - 1; ++i)
        if (group->sample[i] < 0 || group->sample[i] >= 15 || (unsigned long)group->sample[i] >= c->sample_count)
            return 0;
    for (i = 0; i < group->count - 1; ++i)
    {
        index = group->sample[i];
        sample = c->samples + index;
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
