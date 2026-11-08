#include "bh_target_step.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>
static BHTargetStepContext *context(BHEntity *entity)
{
    BHTargetStepContext *ctx;
    memcpy(&ctx, entity->payload + 616, 4);
    return ctx;
}
static int live(BHEntityPool *pool, BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(pool, entity) && entity->generation == generation;
}
int bh_target_step_read(const BHEntity *entity, BHTargetStepState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->countdown, (const char *)entity + 0xec, 4);
    memcpy(&state->minimum, (const char *)entity + 0xfc, 12);
    memcpy(&state->maximum, (const char *)entity + 0x108, 12);
    memcpy(&state->emission, (const char *)entity + 0x114, 16);
    return 1;
}
int bh_target_step_write(BHEntity *entity, const BHTargetStepState *state)
{
    if (!entity || !state)
        return 0;
    memcpy((char *)entity + 0xec, &state->countdown, 4);
    memcpy((char *)entity + 0xfc, &state->minimum, 12);
    memcpy((char *)entity + 0x108, &state->maximum, 12);
    memcpy((char *)entity + 0x114, &state->emission, 16);
    if (entity->step == bh_target_step)
    {
        BHTargetStepContext *ctx = context(entity);
        unsigned long i;
        for (i = 0; i < ctx->physics->body_count; ++i)
            if (ctx->physics->bodies[i].entity == entity)
                ctx->physics->bodies[i].flags = (unsigned long)state->emission;
    }
    return 1;
}
int bh_target_step_bind(BHTargetStepContext *ctx, const BHTargetStepState *state)
{
    unsigned long i;
    BHDebrisBody *body;
    if (!ctx || !state || !(body = ctx->body) || !body->entity || !body->context || !ctx->physics || !ctx->explosions ||
        !ctx->particles || !ctx->pickups || !ctx->mode || !bh_math_environment_supported())
        return 0;
    if (body->entity->query != bh_debris_receive || !live(body->context->pool, body->entity, body->generation) ||
        ctx->physics->pool != body->context->pool || ctx->particles->pool != body->context->pool ||
        ctx->explosions->pool != body->context->pool || ctx->pickups->pool != body->context->pool ||
        ctx->pickups->outside_actors != body->context->registry || !ctx->pickups->first_world ||
        !ctx->pickups->world_count || ctx->pickups->resource_count < 20 ||
        ctx->pickups->default_draw != bh_pickup_default_step || ctx->pickups->special_draw != bh_pickup_special_step)
        return 0;
    for (i = 0; i < ctx->physics->body_count; ++i)
        if (ctx->physics->bodies[i].entity == body->entity && ctx->physics->bodies[i].physics == body->physics)
            break;
    if (i == ctx->physics->body_count)
        return 0;
    ctx->physics->bodies[i].flags = (unsigned long)state->emission;
    bh_target_step_write(body->entity, state);
    memcpy(body->entity->payload + 616, &ctx, 4);
    body->entity->step = bh_target_step;
    ctx->error = 0;
    return 1;
}
static double norm(const BHVec3 *v)
{
    return ((double)v->x * v->x + (double)v->y * v->y) + (double)v->z * v->z;
}
static void sample(BHVec3 *out, BHEntity *entity, const BHTargetStepState *state)
{
    BHPhysicsState physics;
    float x, y;
    double t, z;
    t = (double)rand() / 32768;
    bh_round_float(&x, (double)state->maximum.x * (1 - t) + (double)state->minimum.x * t);
    t = (double)rand() / 32768;
    bh_round_float(&y, (double)state->maximum.y * (1 - t) + (double)state->minimum.y * t);
    t = (double)rand() / 32768;
    z = (double)state->maximum.z * (1 - t) + (double)state->minimum.z * t;
    bh_physics_step_read_entity(&physics, entity);
    bh_round_float(&out->x, (((double)physics.pose.m[0][0] * x + (double)physics.pose.m[0][2] * z) +
                             (double)physics.pose.m[0][1] * y) +
                                entity->position.x);
    bh_round_float(&out->y, (((double)physics.pose.m[1][1] * y + (double)physics.pose.m[1][2] * z) +
                             (double)physics.pose.m[1][0] * x) +
                                entity->position.y);
    bh_round_float(&out->z, (((double)physics.pose.m[2][2] * z + (double)physics.pose.m[2][1] * y) +
                             (double)physics.pose.m[2][0] * x) +
                                entity->position.z);
}
void bh_target_step(BHEntity *entity)
{
    BHTargetStepContext *ctx = context(entity);
    BHDebrisBody *body = ctx->body;
    BHEntityPool *pool = body->context->pool;
    BHTargetStepState state;
    BHVec3 velocity, acceleration = {0, -.002f, 0}, point;
    BHEntity *owner;
    unsigned long generation = entity->generation, u;
    long event[2];
    int r, i;
    float angle, spin;
    double value, n;
    static const long amount[10] = {100, 50, 25, 2, 50, 1, 1, 1, 1, 1};
    if (*ctx->mode == 1)
        return;
    if (!bh_math_environment_supported())
    {
        ctx->error = 1;
        return;
    }
    ctx->error = 0;
    bh_target_step_read(entity, &state);
    if (state.counter > 0)
    {
        u = (unsigned long)state.counter + 1;
        memcpy(&state.counter, &u, 4);
        memcpy((char *)entity + 0x120, &state.counter, 4);
    }
    if (state.emission)
    {
        r = rand();
        if (r < state.emission)
        {
            r = rand();
            bh_round_float(&velocity.x, (double)(r - 16384) / 262144);
            r = rand();
            bh_round_float(&velocity.y, (double)(r - 16384) / 262144);
            r = rand();
            bh_round_float(&velocity.z, (double)(r - 16384) / 262144);
            r = rand();
            bh_round_float(&spin, (double)(r - 16384) / 16384);
            r = rand();
            bh_round_float(&angle, (double)r / 512);
            bh_script_particle_create(ctx->particles, ctx->smoke_image, 3, &entity->position, &velocity, &acceleration,
                                      .5f, angle, 12, spin, .9f, .95f, 2, .3f);
        }
    }
    if (!_isnan(state.countdown) && state.countdown > 0)
    {
        value = (double)state.countdown - 1.0 / 30.0;
        bh_round_float(&state.countdown, value);
        memcpy((char *)entity + 0xec, &state.countdown, 4);
        if (_isnan(value) || value <= 0)
        {
            owner = body->owner;
            if (owner)
            {
                if (!live(pool, owner, owner->generation))
                {
                    ctx->error = 1;
                    return;
                }
                event[0] = 12;
                event[1] = state.token;
                if (owner->query)
                    owner->query(owner, event);
                if (!live(pool, entity, generation))
                {
                    ctx->error = 1;
                    return;
                }
            }
            bh_target_step_read(entity, &state);
            if (state.loot < 0)
            {
                for (i = 0; i < 6; ++i)
                {
                    bh_target_step_read(entity, &state);
                    sample(&point, entity, &state);
                    if (ctx->scripted_prop)
                        bh_script_explosion_prop(ctx->explosions, &point, 1, 12, .95f, (unsigned long)body->owner);
                    else
                        bh_script_explosion_create(ctx->explosions, &point, 1, 12, .95f, (unsigned long)body->owner);
                    if (!live(pool, entity, generation))
                    {
                        ctx->error = 1;
                        return;
                    }
                }
            }
            else
            {
                if (state.loot < 10)
                    bh_pickup_spawn(ctx->pickups, &entity->position, state.loot, 0, amount[state.loot],
                                    amount[state.loot], 0, 40);
                else if ((state.loot >= 10 && state.loot <= 12) || state.loot == 14)
                    bh_pickup_spawn(ctx->pickups, &entity->position, state.loot - 10, 1, 1, 1, 0, 40);
                bh_script_explosion_create(ctx->explosions, &entity->position, 1.2f, 15, .95f,
                                           (unsigned long)body->owner);
                if (!live(pool, entity, generation))
                {
                    ctx->error = 1;
                    return;
                }
            }
            bh_debris_remove(entity);
            return;
        }
    }
    if (!_isnan(body->sleep_time) && body->sleep_time > 2)
        return;
    value = (double)body->sleep_time - 1.0 / 30.0;
    bh_round_float(&body->sleep_time, value);
    if (_isnan(value) || value < 0)
        body->sleep_time = 0;
    if (!bh_physics_step(ctx->physics, entity, .03333333507180214f))
    {
        ctx->error = 1;
        return;
    }
    if (!live(pool, entity, generation))
    {
        ctx->error = 1;
        return;
    }
    n = norm(&body->physics->velocity);
    if (!_isnan(n) && n >= .25)
        return;
    n = norm(&body->physics->angular_velocity);
    if (!_isnan(n) && n >= .01)
        return;
    bh_round_float(&body->sleep_time, (double)body->sleep_time - (-2.0 / 30.0));
}
