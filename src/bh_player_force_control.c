#include "bh_player_force_control.h"
#include "bh_player_propulsion.h"
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
static float read_float(const BHEntity *entity, unsigned long offset)
{
    float v;
    memcpy(&v, (const unsigned char *)entity + offset, 4);
    return v;
}
static void write_float(BHEntity *entity, unsigned long offset, float value)
{
    memcpy((unsigned char *)entity + offset, &value, 4);
}
static void vector_store(BHEntity *entity, unsigned long offset, const BHVec3 *v)
{
    memcpy((unsigned char *)entity + offset, v, 12);
}
static void local_velocity(const BHPhysicsState *physics, BHVec3 *out)
{
    unsigned long k;
    for (k = 0; k < 3; ++k)
        bh_round_float(((float *)out) + k, ((double)physics->pose.m[0][k] * physics->velocity.x +
                                            (double)physics->pose.m[2][k] * physics->velocity.z) +
                                               (double)physics->pose.m[1][k] * physics->velocity.y);
}
static void point(const BHPhysicsState *physics, double x, double y, double z, int variant, BHVec3 *out)
{
    unsigned long k;
    double a, b, d, sum;
    for (k = 0; k < 3; ++k)
    {
        a = (double)physics->pose.m[k][0] * x;
        b = (double)physics->pose.m[k][1] * y;
        d = (double)physics->pose.m[k][2] * z;
        if (variant == 0)
            sum = k == 2 ? (b + a) + d : (a + b) + d;
        else if (variant == 1)
            sum = k == 2 ? (a + b) + d : (b + d) + a;
        else if (variant == 2)
            sum = k == 2 ? (b + d) + a : (a + d) + b;
        else
            sum = k == 1 ? (b + d) + a : (d + b) + a;
        bh_round_float(((float *)out) + k, sum + physics->pose.m[k][3]);
    }
}
static void nozzle_base(const BHPhysicsState *physics, float x, float z, int variant, BHVec3 *out)
{
    unsigned long k;
    double sum;
    for (k = 0; k < 3; ++k)
    {
        if (variant == 1)
            sum = k == 2 ? ((double)physics->pose.m[k][1] * .3f - (double)physics->pose.m[k][2] * -z) -
                               (double)physics->pose.m[k][0] * -x
                         : ((double)physics->pose.m[k][2] * z - (double)physics->pose.m[k][0] * -x) -
                               (double)physics->pose.m[k][1] * -.3f;
        else if (k == 1)
            sum = ((double)physics->pose.m[k][2] * z - (double)physics->pose.m[k][1] * -.3f) -
                  (double)physics->pose.m[k][0] * -x;
        else if (variant == 3)
            sum = ((double)physics->pose.m[k][0] * x - (double)physics->pose.m[k][2] * -z) -
                  (double)physics->pose.m[k][1] * -.3f;
        else if (k == 0)
            sum = ((double)physics->pose.m[k][1] * .3f - (double)physics->pose.m[k][0] * -x) -
                  (double)physics->pose.m[k][2] * -z;
        else
            sum = ((double)physics->pose.m[k][0] * x - (double)physics->pose.m[k][1] * -.3f) -
                  (double)physics->pose.m[k][2] * -z;
        bh_round_float(((float *)out) + k, sum + physics->pose.m[k][3]);
    }
}
static int validate(BHPlayerForceControl *control)
{
    BHVec3 inertia;
    BHActorBasisContext *ctx;
    unsigned long i;
    if (!control || !control->body || !control->body->entity || !control->body->physics ||
        !control->instability_counter || !(ctx = control->basis) || !ctx->samples || ctx->sample_count < 14 ||
        ctx->sample_count > ctx->sample_capacity || ctx->a_count != 7 || ctx->b_count != 7 || ctx->a_capacity < 7 ||
        ctx->b_capacity < 7 || !ctx->groups_a || !ctx->groups_b || !ctx->indices_a || !ctx->indices_b ||
        !ctx->scales_a || !ctx->scales_b || !control->body->mass || !_finite(control->body->mass) ||
        !bh_math_environment_supported())
    {
        if (control)
            control->error = 1;
        return 0;
    }
    memcpy(&inertia, (unsigned char *)control->body->entity + 0x78, 12);
    if (!inertia.x || !inertia.y || !inertia.z || !_finite(inertia.x) || !_finite(inertia.y) || !_finite(inertia.z))
    {
        control->error = 1;
        return 0;
    }
    for (i = 0; i < 7; ++i)
    {
        long j;
        const BHActorBasisIndices *g[2];
        int kind;
        g[0] = ctx->indices_a + i;
        g[1] = ctx->indices_b + i;
        for (kind = 0; kind < 2; ++kind)
        {
            if (g[kind]->count < 1 || g[kind]->count > 16)
            {
                control->error = 1;
                return 0;
            }
            for (j = 0; j < g[kind]->count - 1; ++j)
                if (g[kind]->sample[j] < 0 || g[kind]->sample[j] >= 15 ||
                    (unsigned long)g[kind]->sample[j] >= ctx->sample_count)
                {
                    control->error = 1;
                    return 0;
                }
        }
    }
    control->error = 0;
    return 1;
}
static int apply(BHPlayerForceControl *control, float channels[15], const BHActorBasisIndices *g, float value)
{
    BHVec3 inertia;
    memcpy(&inertia, (unsigned char *)control->body->entity + 0x78, 12);
    return bh_actor_basis_apply_group(control->basis, control->body->physics, control->body->mass, &inertia, channels,
                                      g, value);
}
static int solve(BHPlayerForceControl *control, float channels[15], BHVec3 *torque, BHVec3 *force, int variant_b)
{
    int i, j;
    float a, b, delta;
    unsigned long k;
    if (bh_actor_basis_a_pair(control->basis, torque, &i, &j, &a, &b))
    {
        if (fabs(a) > 0 && !apply(control, channels, control->basis->indices_a + i, a))
            return 0;
        if (fabs(b) > 0 && !apply(control, channels, control->basis->indices_a + j, b))
            return 0;
        for (k = 0; k < 3; ++k)
        {
            if (k == 2 && variant_b)
            {
                bh_round_float(&force->z, ((double)force->z + (double)control->basis->groups_a[j].direction.z * b) +
                                              (double)control->basis->groups_a[i].direction.z * a);
                continue;
            }
            delta = stored((double)((float *)&control->basis->groups_a[j].direction)[k] * b +
                           (double)((float *)&control->basis->groups_a[i].direction)[k] * a);
            bh_round_float(((float *)force) + k, (double)((float *)force)[k] + delta);
        }
    }
    if (bh_actor_basis_b_pair(control->basis, force, &i, &j, &a, &b))
    {
        if (fabs(a) > 0 && !apply(control, channels, control->basis->indices_b + i, a))
            return 0;
        if (fabs(b) > 0 && !apply(control, channels, control->basis->indices_b + j, b))
            return 0;
    }
    return 1;
}
const BHVec3 bh_player_nozzles[BH_PLAYER_NOZZLE_COUNT] = {
    {.5f, .3f, 1.5f}, {-.5f, .3f, 1.5f}, {-.7f, .3f, -1.2f}, {.7f, .3f, -1.2f}};

static void visual(BHPlayerForceControl *control, float channels[15], const float previous[15], int variant)
{
    BHEntity *entity = control->body->entity;
    BHPhysicsState *physics = control->body->physics;
    unsigned long i, j;
    float total = 0;
    double x, y, z, smoothed;
    BHVec3 output;
    for (i = 0; i < 15; ++i)
    {
        /* Original FST publishes the channel without popping its retained
           value; the total consumes that value, not the rounded channel. */
        smoothed = sqrt((double)channels[i]) * .0012500000000000002 - (double)previous[i] * -.9;
        channels[i] = stored(smoothed);
        total = stored(smoothed + total);
    }
    memcpy((unsigned char *)entity + 0x2bc, channels, 60);
    write_float(entity, 0x2f8, total);
    for (i = 0; i < BH_PLAYER_NOZZLE_COUNT; ++i)
    {
        /* Each controller retains its own original projection sum order. */
        nozzle_base(physics, bh_player_nozzles[i].x, bh_player_nozzles[i].z, variant, &output);
        vector_store(entity, 0x2fc + i * 24, &output);
        j = 1 + i * 3;
        x = i == 0   ? (double)channels[j] - (-.5)
            : i == 1 ? -.5 - (double)channels[j]
            : i == 2 ? -.7 - (double)channels[j]
                     : (double)channels[j] - (-.7);
        y = .3 - (double)channels[j + 1] * -2.0f;
        z = i < 2 ? (double)channels[j + 2] - (-1.5) : -1.2 - (double)channels[j + 2];
        if (i == 1)
        {
            x = stored(x);
            y = stored(y);
        }
        point(physics, x, y, z, variant, &output);
        vector_store(entity, 0x308 + i * 24, &output);
    }
}
int bh_player_force_control_b(BHPlayerForceControl *control, float unused, int skip)
{
    BHEntity *entity;
    BHPhysicsState *physics;
    BHVec3 v, noise = {0, 0, 0}, torque, force;
    float old[15], channels[15];
    float pitch, yaw, thrust, instability, pitch_factor, speed_factor, signed_x, signed_y, pitch_target, roll_target;
    double noise_y;
    unsigned long k;
    (void)unused;
    if (!validate(control))
        return 0;
    entity = control->body->entity;
    physics = control->body->physics;
    memcpy(old, (unsigned char *)entity + 0x2bc, 60);
    memset(channels, 0, sizeof(channels));
    if (!skip)
    {
        local_velocity(physics, &v);
        torque.x = -physics->angular_velocity.x;
        torque.y = -physics->angular_velocity.y;
        torque.z = -physics->angular_velocity.z;
        instability = read_float(entity, 0x250);
        if (physics->pose.m[1][1] >= .5)
            write_float(entity, 0x250, 1);
        else
        {
            ++*control->instability_counter;
            if ((*control->instability_counter & 3) == 0)
            {
                noise.x = stored((double)(rand() - 16384) * instability * .0001220703125);
                noise.z = stored((double)(rand() - 16384) * instability * .0001220703125);
                noise_y = noise.y;
                if (instability > 6)
                    /* The third draw has no binary32 spill before rotation. */
                    noise_y = (double)(rand() - 16384) * instability * .0001220703125;
                for (k = 0; k < 3; ++k)
                    bh_round_float(((float *)&torque) + k,
                                   ((double)physics->pose.m[0][k] * noise.x + (double)physics->pose.m[2][k] * noise.z) +
                                       (double)physics->pose.m[1][k] * noise_y + ((float *)&torque)[k]);
            }
            write_float(entity, 0x250, stored((double)instability - (-.03333333333333333)));
        }
        pitch = read_float(entity, 0x18c);
        yaw = read_float(entity, 0x190);
        thrust = read_float(entity, 0x194);
        pitch_factor = stored((double)pitch * 2.75);
        speed_factor = stored((double)v.z * v.z * .00048828125);
        signed_x = v.x;
        signed_y = v.y;
        if (v.z > 0)
        {
            signed_x = -signed_x;
            signed_y = -signed_y;
        }
        force.x = stored((double)stored((double)v.x * -.85) * control->body->mass * 5.0f);
        force.y = stored(((double)stored((double)v.y * -.85) - 1.9639999866485596f) * control->body->mass * 5.0f);
        force.z = stored(((double)stored((double)v.z * -.4) - (double)thrust * -35.0f) * control->body->mass * 5.0f);
        pitch_target = stored((((double)physics->pose.m[1][0] * pitch_factor + torque.x) - ((double)yaw + yaw)) +
                              (double)pitch * pitch * sqrt((double)v.z * v.z) * .00625);
        pitch_target = stored((double)speed_factor * physics->pose.m[1][2] + pitch_target);
        torque.x = stored(((double)pitch_target - (double)signed_y * -.03) * control->body->mass * 5.0f);
        torque.y = stored((((double)physics->pose.m[1][1] * pitch_factor + torque.y) - (double)signed_x * .02) *
                          control->body->mass * 5.0f);
        roll_target = stored((double)physics->pose.m[1][2] * pitch_factor + torque.z);
        roll_target = stored((double)roll_target - (double)physics->pose.m[1][0] * 16.0f);
        torque.z = stored((double)roll_target * control->body->mass * 5.0f);
        if (!solve(control, channels, &torque, &force, 1))
        {
            control->error = 2;
            return 0;
        }
    }
    visual(control, channels, old, 3);
    return 1;
}
/* Variants A/C/D share the up-vector stabilizer and drag construction. C/D
   additionally couple three player control scalars into the local targets.
   The original x87 CIpow calls are verified library reuse: exponent3 over
   local stabilizer.x and exponent2 over input0. No original code is called. */
static int stabilized(BHPlayerForceControl *control, float dt, int skip, int variant)
{
    BHEntity *entity;
    BHPhysicsState *physics;
    BHVec3 up, cross, local, velocity, target, force, torque;
    float old[15], channels[15], ratio, blend, instability, input0, input1, input2;
    float saved_x, saved_y, saved_z, speed, scaled_x, scaled_y, selected_factor;
    double angle_x, controlled_x, retained_ratio;
    unsigned long k;
    if (!validate(control) || (!skip && variant && (!(dt > 0) || !_finite(dt))))
    {
        if (control)
            control->error = 1;
        return 0;
    }
    entity = control->body->entity;
    physics = control->body->physics;
    memcpy(old, (unsigned char *)entity + 0x2bc, 60);
    memset(channels, 0, sizeof(channels));
    if (!skip)
    {
        input0 = read_float(entity, 0x18c);
        input1 = read_float(entity, 0x190);
        input2 = read_float(entity, 0x194);
        up.x = physics->pose.m[0][1];
        up.y = physics->pose.m[1][1];
        up.z = physics->pose.m[2][1];
        cross.x = -up.z;
        cross.y = 0;
        cross.z = up.x;
        retained_ratio = 20.0 / (sqrt((double)physics->velocity.z * physics->velocity.z +
                                      (double)physics->velocity.x * physics->velocity.x) -
                                 (-20.0));
        ratio = stored(retained_ratio);
        if (variant)
            cross.y = stored((double)cross.y - ((double)input0 * (1.5 - retained_ratio * -2.0f) *
                                                (variant == 2 ? -2.0999999999999996 : -1.5)) /
                                                   ((double)dt * 250.0f));
        if (up.y >= .5)
        {
            write_float(entity, 0x250, 1);
            blend = stored(sqrt(ratio));
        }
        else
        {
            instability = read_float(entity, 0x250);
            ++*control->instability_counter;
            if ((*control->instability_counter & 3) == 0)
            {
                cross.x = stored((double)cross.x - (double)(rand() - 16384) * instability * -.0001220703125);
                cross.z = stored((double)cross.z - (double)(rand() - 16384) * instability * -.0001220703125);
                if (instability > 6)
                    cross.y = stored((double)cross.y - (double)(rand() - 16384) * instability * -.0001220703125);
            }
            write_float(entity, 0x250, stored((double)instability - (-.03333333333333333)));
        }
        for (k = 0; k < 3; ++k)
            bh_round_float(((float *)&local) + k,
                           ((double)physics->pose.m[0][k] * cross.x + (double)physics->pose.m[1][k] * cross.y) +
                               (double)physics->pose.m[2][k] * cross.z);
        angle_x = local.x;
        if (up.y >= .5)
            angle_x = (1.0 - blend) * local.x + pow((double)local.x, 3.0) * .05 * blend;
        else
            /* The unstable X projection stays live through dt scaling;
               only Y/Z are spilled before constructing the torque. */
            angle_x = ((double)physics->pose.m[0][0] * cross.x + (double)physics->pose.m[1][0] * cross.y) +
                      (double)physics->pose.m[2][0] * cross.z;
        local_velocity(physics, &velocity);
        saved_x = velocity.x;
        saved_y = velocity.y;
        saved_z = velocity.z;
        torque.x = stored(angle_x * dt * 250.0f - physics->angular_velocity.x);
        torque.y = stored((double)local.y * dt * 250.0f - physics->angular_velocity.y);
        torque.z = stored((double)local.z * dt * 250.0f - physics->angular_velocity.z);
        target = velocity;
        if (variant)
        {
            target.y = stored((double)velocity.y - (double)ratio * input1 * -35.0f);
            torque.z = stored((double)torque.z - (double)input0 * -1.5);
            /* Original FST keeps the first input correction live across
               CIpow; its scratch spill is not the subsequent operand. */
            controlled_x = (double)torque.x - (double)input1 * 2.25;
            torque.x = stored(controlled_x - pow((double)input0, 2.0) * (variant == 2 ? -1.0499999999999998 : -.75));
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
        force.x = stored(((double)speed * target.x - saved_x) * control->body->mass * 5.0f);
        force.y = stored((((double)speed * target.y - saved_y) - 1.9639999866485596f) * control->body->mass * 5.0f);
        force.z = stored(((double)speed * target.z - saved_z) * control->body->mass * 5.0f);
        for (k = 0; k < 3; ++k)
            bh_round_float(((float *)&torque) + k, (double)((float *)&torque)[k] * control->body->mass * 5.0f);
        if (!solve(control, channels, &torque, &force, 0))
        {
            control->error = 2;
            return 0;
        }
    }
    visual(control, channels, old, variant);
    return 1;
}
int bh_player_force_control_a(BHPlayerForceControl *control, float dt, int skip)
{
    return stabilized(control, dt, skip, 0);
}
int bh_player_force_control_c(BHPlayerForceControl *control, float dt, int skip)
{
    return stabilized(control, dt, skip, 1);
}
int bh_player_force_control_d(BHPlayerForceControl *control, float dt, int skip)
{
    return stabilized(control, dt, skip, 2);
}
