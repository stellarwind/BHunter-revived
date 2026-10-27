#include "bh_player_force_control.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

static float stored(double v)
{
    float out;
    bh_round_float(&out, v);
    return out;
}
static float read_float(const BHEntity *e, unsigned long offset)
{
    float v;
    memcpy(&v, (const unsigned char *)e + offset, 4);
    return v;
}
static void write_float(BHEntity *e, unsigned long offset, float value)
{
    memcpy((unsigned char *)e + offset, &value, 4);
}
static void vector_store(BHEntity *e, unsigned long offset, const BHVec3 *v)
{
    memcpy((unsigned char *)e + offset, v, 12);
}
static void local_velocity(const BHPhysicsState *p, BHVec3 *out)
{
    unsigned long k;
    for (k = 0; k < 3; ++k)
        bh_round_float(((float *)out) + k,
                       ((double)p->pose.m[0][k] * p->velocity.x + (double)p->pose.m[2][k] * p->velocity.z) +
                           (double)p->pose.m[1][k] * p->velocity.y);
}
static void point(const BHPhysicsState *p, double x, double y, double z, BHVec3 *out)
{
    unsigned long k;
    for (k = 0; k < 3; ++k)
        bh_round_float(((float *)out) + k, ((double)p->pose.m[k][0] * x + (double)p->pose.m[k][1] * y) +
                                               (double)p->pose.m[k][2] * z + p->pose.m[k][3]);
}
static int validate(BHPlayerForceControl *c)
{
    BHVec3 inertia;
    BHActorBasisContext *b;
    unsigned long i;
    if (!c || !c->body || !c->body->entity || !c->body->physics || !c->instability_counter || !(b = c->basis) ||
        !b->samples || b->sample_count < 14 || b->sample_count > b->sample_capacity || b->a_count != 7 ||
        b->b_count != 7 || b->a_capacity < 7 || b->b_capacity < 7 || !b->groups_a || !b->groups_b || !b->indices_a ||
        !b->indices_b || !b->scales_a || !b->scales_b || !c->body->mass || !_finite(c->body->mass) ||
        !bh_math_environment_supported())
    {
        if (c)
            c->error = 1;
        return 0;
    }
    memcpy(&inertia, (unsigned char *)c->body->entity + 0x78, 12);
    if (!inertia.x || !inertia.y || !inertia.z || !_finite(inertia.x) || !_finite(inertia.y) || !_finite(inertia.z))
    {
        c->error = 1;
        return 0;
    }
    for (i = 0; i < 7; ++i)
    {
        long j;
        const BHActorBasisIndices *g[2];
        int kind;
        g[0] = b->indices_a + i;
        g[1] = b->indices_b + i;
        for (kind = 0; kind < 2; ++kind)
        {
            if (g[kind]->count < 1 || g[kind]->count > 16)
            {
                c->error = 1;
                return 0;
            }
            for (j = 0; j < g[kind]->count - 1; ++j)
                if (g[kind]->sample[j] < 0 || g[kind]->sample[j] >= 15 ||
                    (unsigned long)g[kind]->sample[j] >= b->sample_count)
                {
                    c->error = 1;
                    return 0;
                }
        }
    }
    c->error = 0;
    return 1;
}
static int apply(BHPlayerForceControl *c, float channels[15], const BHActorBasisIndices *g, float value)
{
    BHVec3 inertia;
    memcpy(&inertia, (unsigned char *)c->body->entity + 0x78, 12);
    return bh_actor_basis_apply_group(c->basis, c->body->physics, c->body->mass, &inertia, channels, g, value);
}
static int solve(BHPlayerForceControl *c, float channels[15], BHVec3 *torque, BHVec3 *force, int variant_b)
{
    int i, j;
    float a, b, delta;
    unsigned long k;
    if (bh_actor_basis_a_pair(c->basis, torque, &i, &j, &a, &b))
    {
        if (fabs(a) > 0 && !apply(c, channels, c->basis->indices_a + i, a))
            return 0;
        if (fabs(b) > 0 && !apply(c, channels, c->basis->indices_a + j, b))
            return 0;
        for (k = 0; k < 3; ++k)
        {
            if (k == 2 && variant_b)
            {
                bh_round_float(&force->z, ((double)force->z + (double)c->basis->groups_a[j].direction.z * b) +
                                              (double)c->basis->groups_a[i].direction.z * a);
                continue;
            }
            delta = stored((double)((float *)&c->basis->groups_a[j].direction)[k] * b +
                           (double)((float *)&c->basis->groups_a[i].direction)[k] * a);
            bh_round_float(((float *)force) + k, (double)((float *)force)[k] + delta);
        }
    }
    if (bh_actor_basis_b_pair(c->basis, force, &i, &j, &a, &b))
    {
        if (fabs(a) > 0 && !apply(c, channels, c->basis->indices_b + i, a))
            return 0;
        if (fabs(b) > 0 && !apply(c, channels, c->basis->indices_b + j, b))
            return 0;
    }
    return 1;
}
static void visual(BHPlayerForceControl *c, float channels[15], const float previous[15])
{
    static const float nozzle[4][3] = {{.5f, .3f, 1.5f}, {-.5f, .3f, 1.5f}, {-.7f, .3f, -1.2f}, {.7f, .3f, -1.2f}};
    BHEntity *e = c->body->entity;
    BHPhysicsState *p = c->body->physics;
    unsigned long i, j;
    float total = 0, x, y, z;
    BHVec3 output;
    for (i = 0; i < 15; ++i)
    {
        channels[i] = stored(sqrt((double)channels[i]) * .0012500000000000002 - (double)previous[i] * -.9);
        total = stored((double)channels[i] + total);
    }
    memcpy((unsigned char *)e + 0x2bc, channels, 60);
    write_float(e, 0x2f8, total);
    for (i = 0; i < 4; ++i)
    {
        point(p, nozzle[i][0], nozzle[i][1], nozzle[i][2], &output);
        vector_store(e, 0x2fc + i * 24, &output);
        j = 1 + i * 3;
        x = stored((i == 0 || i == 3) ? (double)channels[j] + nozzle[i][0] : (double)nozzle[i][0] - channels[j]);
        y = stored(.3 - (double)channels[j + 1] * -2.0f);
        z = stored(i < 2 ? (double)channels[j + 2] + 1.5 : -1.2 - channels[j + 2]);
        point(p, x, y, z, &output);
        vector_store(e, 0x308 + i * 24, &output);
    }
}
int __cdecl bh_player_force_control_b(BHPlayerForceControl *c, float unused, int skip)
{
    BHEntity *e;
    BHPhysicsState *p;
    BHVec3 v, noise = {0, 0, 0}, torque, force;
    float old[15], channels[15];
    float pitch, yaw, thrust, instability, pitch_factor, speed_factor, signed_x, signed_y;
    unsigned long k;
    (void)unused;
    if (!validate(c))
        return 0;
    e = c->body->entity;
    p = c->body->physics;
    memcpy(old, (unsigned char *)e + 0x2bc, 60);
    memset(channels, 0, sizeof(channels));
    if (!skip)
    {
        local_velocity(p, &v);
        torque.x = -p->angular_velocity.x;
        torque.y = -p->angular_velocity.y;
        torque.z = -p->angular_velocity.z;
        instability = read_float(e, 0x250);
        if (p->pose.m[1][1] >= .5)
            write_float(e, 0x250, 1);
        else
        {
            ++*c->instability_counter;
            if ((*c->instability_counter & 3) == 0)
            {
                noise.x = stored((double)(rand() - 16384) * instability * .0001220703125);
                noise.z = stored((double)(rand() - 16384) * instability * .0001220703125);
                if (instability > 6)
                    noise.y = stored((double)(rand() - 16384) * instability * .0001220703125);
                for (k = 0; k < 3; ++k)
                    bh_round_float(((float *)&torque) + k,
                                   ((double)p->pose.m[0][k] * noise.x + (double)p->pose.m[2][k] * noise.z) +
                                       (double)p->pose.m[1][k] * noise.y + ((float *)&torque)[k]);
            }
            write_float(e, 0x250, stored((double)instability - (-.03333333333333333)));
        }
        pitch = read_float(e, 0x18c);
        yaw = read_float(e, 0x190);
        thrust = read_float(e, 0x194);
        pitch_factor = stored((double)pitch * 2.75);
        speed_factor = stored((double)v.z * v.z * .00048828125);
        signed_x = v.x;
        signed_y = v.y;
        if (v.z > 0)
        {
            signed_x = -signed_x;
            signed_y = -signed_y;
        }
        force.x = stored((double)stored((double)v.x * -.85) * c->body->mass * 5.0f);
        force.y = stored(((double)stored((double)v.y * -.85) - 1.9639999866485596f) * c->body->mass * 5.0f);
        force.z = stored(((double)stored((double)v.z * -.4) - (double)thrust * -35.0f) * c->body->mass * 5.0f);
        torque.x = stored((((double)speed_factor * p->pose.m[1][2] +
                            (((double)p->pose.m[1][0] * pitch_factor + torque.x) - ((double)yaw + yaw)) +
                            (double)pitch * pitch * sqrt((double)v.z * v.z) * .00625) -
                           (double)signed_y * -.03) *
                          c->body->mass * 5.0f);
        torque.y = stored((((double)p->pose.m[1][1] * pitch_factor + torque.y) - (double)signed_x * .02) *
                          c->body->mass * 5.0f);
        torque.z = stored((((double)p->pose.m[1][2] * pitch_factor + torque.z) - (double)p->pose.m[1][0] * 16.0f) *
                          c->body->mass * 5.0f);
        if (!solve(c, channels, &torque, &force, 1))
        {
            c->error = 2;
            return 0;
        }
    }
    visual(c, channels, old);
    return 1;
}
/* Variants A/C/D share the up-vector stabilizer and drag construction. C/D
   additionally couple three player control scalars into the local targets.
   The original x87 CIpow calls are verified library reuse: exponent3 over
   local stabilizer.x and exponent2 over input0. No original code is called. */
static int stabilized(BHPlayerForceControl *c, float dt, int skip, int variant)
{
    BHEntity *e;
    BHPhysicsState *p;
    BHVec3 up, cross, local, velocity, target, force, torque;
    float old[15], channels[15], ratio, blend, instability, input0, input1, input2;
    float saved_x, saved_y, saved_z, speed, scaled_x, scaled_y, selected_factor;
    double angle_x;
    unsigned long k;
    if (!validate(c) || (!skip && variant && (!(dt > 0) || !_finite(dt))))
    {
        if (c)
            c->error = 1;
        return 0;
    }
    e = c->body->entity;
    p = c->body->physics;
    memcpy(old, (unsigned char *)e + 0x2bc, 60);
    memset(channels, 0, sizeof(channels));
    if (!skip)
    {
        input0 = read_float(e, 0x18c);
        input1 = read_float(e, 0x190);
        input2 = read_float(e, 0x194);
        up.x = p->pose.m[0][1];
        up.y = p->pose.m[1][1];
        up.z = p->pose.m[2][1];
        cross.x = -up.z;
        cross.y = 0;
        cross.z = up.x;
        ratio = stored(20.0 /
                       (sqrt((double)p->velocity.z * p->velocity.z + (double)p->velocity.x * p->velocity.x) - (-20.0)));
        if (variant)
            cross.y = stored((double)cross.y - ((double)input0 * (1.5 - (double)ratio * -2.0f) *
                                                (variant == 2 ? -2.0999999999999996 : -1.5)) /
                                                   ((double)dt * 250.0f));
        if (up.y >= .5)
        {
            write_float(e, 0x250, 1);
            blend = stored(sqrt(ratio));
        }
        else
        {
            instability = read_float(e, 0x250);
            ++*c->instability_counter;
            if ((*c->instability_counter & 3) == 0)
            {
                cross.x = stored((double)cross.x - (double)(rand() - 16384) * instability * -.0001220703125);
                cross.z = stored((double)cross.z - (double)(rand() - 16384) * instability * -.0001220703125);
                if (instability > 6)
                    cross.y = stored((double)cross.y - (double)(rand() - 16384) * instability * -.0001220703125);
            }
            write_float(e, 0x250, stored((double)instability - (-.03333333333333333)));
        }
        for (k = 0; k < 3; ++k)
            bh_round_float(((float *)&local) + k,
                           ((double)p->pose.m[0][k] * cross.x + (double)p->pose.m[1][k] * cross.y) +
                               (double)p->pose.m[2][k] * cross.z);
        angle_x = local.x;
        if (up.y >= .5)
            angle_x = (1.0 - blend) * local.x + pow((double)local.x, 3.0) * .05 * blend;
        local_velocity(p, &velocity);
        saved_x = velocity.x;
        saved_y = velocity.y;
        saved_z = velocity.z;
        torque.x = stored(angle_x * dt * 250.0f - p->angular_velocity.x);
        torque.y = stored((double)local.y * dt * 250.0f - p->angular_velocity.y);
        torque.z = stored((double)local.z * dt * 250.0f - p->angular_velocity.z);
        target = velocity;
        if (variant)
        {
            target.y = stored((double)velocity.y - (double)ratio * input1 * -35.0f);
            torque.z = stored((double)torque.z - (double)input0 * -1.5);
            torque.x = stored((double)torque.x - (double)input1 * 2.25);
            torque.x =
                stored((double)torque.x - pow((double)input0, 2.0) * (variant == 2 ? -1.0499999999999998 : -.75));
            selected_factor = (double)velocity.z * input2 > 0 ? -10.0f : -30.0f;
            target.z = stored((double)velocity.z - (double)input2 * selected_factor * (variant == 2 ? 1.3 : 1.0));
        }
        if (variant == 2)
        {
            if (target.z > 3750.0)
                target.z = 3250;
            if (target.z < -3750.0)
                target.z = -3250;
        }
        else
        {
            if (target.z > 2500)
                target.z = 2500;
            if (target.z < -2500)
                target.z = -2500;
        }
        speed = stored(sqrt(((double)target.z * target.z + (double)target.y * target.y) + (double)target.x * target.x) *
                       (variant == 2 ? .985 : .97));
        scaled_x = stored((double)target.x * (variant == 2 ? .3 : .1));
        scaled_y = stored((double)target.y * (variant == 2 ? .3 : .1));
        target.x = scaled_x;
        target.y = scaled_y;
        bh_vec3_normalize(&target);
        force.x = stored(((double)speed * target.x - saved_x) * c->body->mass * 5.0f);
        force.y = stored((((double)speed * target.y - saved_y) - 1.9639999866485596f) * c->body->mass * 5.0f);
        force.z = stored(((double)speed * target.z - saved_z) * c->body->mass * 5.0f);
        for (k = 0; k < 3; ++k)
            bh_round_float(((float *)&torque) + k, (double)((float *)&torque)[k] * c->body->mass * 5.0f);
        if (!solve(c, channels, &torque, &force, 0))
        {
            c->error = 2;
            return 0;
        }
    }
    visual(c, channels, old);
    return 1;
}
int __cdecl bh_player_force_control_a(BHPlayerForceControl *c, float dt, int skip)
{
    return stabilized(c, dt, skip, 0);
}
int __cdecl bh_player_force_control_c(BHPlayerForceControl *c, float dt, int skip)
{
    return stabilized(c, dt, skip, 1);
}
int __cdecl bh_player_force_control_d(BHPlayerForceControl *c, float dt, int skip)
{
    return stabilized(c, dt, skip, 2);
}
