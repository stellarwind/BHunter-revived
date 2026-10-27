#include "bh_contact_effects.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef char spark_state_size[sizeof(BHSparkState) == 48 ? 1 : -1];
typedef char spark_velocity_offset[offsetof(BHSparkState, velocity) == 0x24 ? 1 : -1];
typedef char impact_age_offset[offsetof(BHImpactState, age) == 0x18 ? 1 : -1];
typedef char impact_normal_offset[offsetof(BHImpactState, normal) == 0x2c ? 1 : -1];
typedef char state_payload_offset[offsetof(BHEntity, payload) + 4 == 0xe8 ? 1 : -1];
static void __cdecl spark_tick(BHEntity *entity);
static void __cdecl spark_refresh(BHEntity *entity, void *packet);
static void __cdecl impact_tick(BHEntity *entity);
static void __cdecl quad_refresh(BHEntity *entity, void *packet);
static void __cdecl rays_refresh(BHEntity *entity, void *packet);
static BHContactEffects *context(BHEntity *entity)
{
    BHContactEffects *ctx;
    memcpy(&ctx, entity->payload + 64, sizeof(ctx));
    return ctx;
}
static void bind(BHEntity *entity, BHContactEffects *ctx)
{
    memcpy(entity->payload + 64, &ctx, sizeof(ctx));
}
static BHSparkState *spark(BHEntity *entity)
{
    return (BHSparkState *)(entity->payload + 4);
}
static BHImpactState *impact(BHEntity *entity)
{
    return (BHImpactState *)(entity->payload + 4);
}
static int finite_vector(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int live(BHContactEffects *ctx, BHEntity *entity)
{
    BHEntity *p;
    unsigned long steps = 0;
    if (!ctx || !ctx->pool)
        return 0;
    for (p = ctx->pool->active; p && steps++ < ctx->pool->capacity; p = p->next)
        if (p == entity)
            return 1;
    return 0;
}
int __cdecl bh_contact_effects_init(BHContactEffects *ctx, BHEntityPool *pool, BHDrawQueue *queue,
                                    unsigned long spark_id, unsigned long quad_id, unsigned long rays_id,
                                    unsigned long line_id)
{
    BHContactEffects next;
    if (!ctx || !pool || !queue || !queue->commands || !bh_entities_valid(pool))
        return 0;
    memset(&next, 0, sizeof(next));
    next.pool = pool;
    next.queue = queue;
    next.spark_texture = spark_id;
    next.quad_texture = quad_id;
    next.rays_texture = rays_id;
    next.line_texture = line_id;
    *ctx = next;
    return 1;
}
int __cdecl bh_contact_effects_dispose(BHContactEffects *ctx)
{
    BHEntity *p;
    unsigned long count = 0;
    if (!ctx || !ctx->pool)
        return 0;
    for (p = ctx->pool->active; p && count++ < ctx->pool->capacity; p = p->next)
        if ((p->step == spark_tick || p->step == impact_tick) && context(p) == ctx)
        {
            ctx->error = BH_EFFECT_CONTEXT;
            return 0;
        }
    memset(ctx, 0, sizeof(*ctx));
    return 1;
}
BHSparkState *__cdecl bh_contact_spark_state(BHContactEffects *ctx, BHEntity *entity)
{
    if (!entity || !live(ctx, entity) || entity->step != spark_tick || entity->refresh != spark_refresh ||
        context(entity) != ctx)
        return NULL;
    return spark(entity);
}
BHImpactState *__cdecl bh_contact_impact_state(BHContactEffects *ctx, BHEntity *entity)
{
    if (!entity || !live(ctx, entity) || entity->step != impact_tick ||
        (entity->refresh != quad_refresh && entity->refresh != rays_refresh) || context(entity) != ctx)
        return NULL;
    return impact(entity);
}
BHEntity *__cdecl bh_contact_spark_create(BHContactEffects *ctx, const BHVec3 *point, float lifetime, float speed)
{
    BHEntity *entity;
    BHSparkState *state;
    int value;
    if (!ctx || !ctx->pool || !ctx->queue || !finite_vector(point) || !_finite(lifetime) || !_finite(speed) ||
        !bh_math_environment_supported())
    {
        if (ctx)
            ctx->error = BH_EFFECT_ARGUMENT;
        return NULL;
    }
    entity = bh_entity_acquire(ctx->pool);
    if (!entity)
    {
        ctx->error = BH_EFFECT_POOL;
        return NULL;
    }
    state = spark(entity);
    state->lifetime = lifetime;
    entity->position = *point;
    value = rand();
    bh_round_float(&state->velocity.x, ((double)value - 16384.0) * (1.0 / 6000.0));
    value = rand();
    bh_round_float(&state->velocity.x, (double)speed * state->velocity.x);
    bh_round_float(&state->velocity.y, ((double)value - 16384.0) * (1.0 / 6000.0));
    bh_round_float(&state->velocity.y, (double)speed * state->velocity.y);
    value = rand();
    bh_round_float(&state->velocity.z, (((double)value - 16384.0) * (1.0 / 6000.0)) * speed);
    value = rand();
    bh_round_float(&state->angle, (double)value * (1.0 / 8192.0));
    value = rand();
    bh_round_float(&state->spin, ((double)value - 16384.0) * (1.0 / 16384.0));
    value = rand();
    state->frame = (unsigned long)value & 7;
    value = rand();
    bh_round_float(&state->width, .1 - (double)value * (-1.0 / 32768.0));
    value = rand();
    bh_round_float(&state->height, .1 - (double)value * (-1.0 / 32768.0));
    bind(entity, ctx);
    entity->step = spark_tick;
    entity->refresh = spark_refresh;
    ctx->error = BH_EFFECT_OK;
    return entity;
}
BHEntity *__cdecl bh_contact_impact_create(BHContactEffects *ctx, const BHVec3 *point, const BHVec3 *normal,
                                           unsigned long flags)
{
    BHEntity *entity;
    BHImpactState *state;
    if (!ctx || !ctx->pool || !ctx->queue || !finite_vector(point) || !finite_vector(normal) || !(flags & 6))
    {
        if (ctx)
            ctx->error = BH_EFFECT_ARGUMENT;
        return NULL;
    }
    entity = bh_entity_acquire(ctx->pool);
    if (!entity)
    {
        ctx->error = BH_EFFECT_POOL;
        return NULL;
    }
    state = impact(entity);
    state->age = 0;
    state->duration = 1;
    state->size = (flags & 2) ? 16.0f : 12.0f;
    state->normal = *normal;
    entity->position = *point;
    bind(entity, ctx);
    entity->step = impact_tick;
    entity->refresh = (flags & 2) ? quad_refresh : rays_refresh;
    ctx->error = BH_EFFECT_OK;
    return entity;
}
static void __cdecl spark_tick(BHEntity *entity)
{
    BHContactEffects *ctx = context(entity);
    BHSparkState *state = spark(entity);
    if (!ctx || !bh_contact_spark_state(ctx, entity) || !bh_math_environment_supported())
        return;
    bh_round_float(&state->lifetime, (double)state->lifetime - (1.0 / 60.0));
    if (state->lifetime <= 0)
    {
        if (!bh_entity_release(ctx->pool, entity))
            ctx->error = BH_EFFECT_POOL;
        return;
    }
    bh_round_float(&entity->position.y, (double)state->velocity.y * (1.0 / 30.0) + entity->position.y);
    bh_round_float(&entity->position.z, (double)state->velocity.z * (1.0 / 30.0) + entity->position.z);
    bh_round_float(&entity->position.x, (double)state->velocity.x * (1.0 / 30.0) + entity->position.x);
    bh_round_float(&state->velocity.x, (double)state->velocity.x * .96);
    bh_round_float(&state->velocity.z, (double)state->velocity.z * .96);
    bh_round_float(&state->velocity.y, (double)state->velocity.y * .96 - (-.16366666666666668));
    bh_round_float(&state->width, (double)state->width - (-1.0 / 30.0));
    bh_round_float(&state->angle, (double)state->angle - (double)state->spin * (-1.0 / 30.0));
    bh_round_float(&state->height, (double)state->height - (-1.0 / 30.0));
}
static void __cdecl spark_refresh(BHEntity *entity, void *packet)
{
    BHContactEffects *ctx = context(entity);
    BHSparkState *state = spark(entity);
    if (!ctx || !bh_contact_spark_state(ctx, entity))
        return;
    if (bh_draw_billboard(ctx->queue, 3, ctx->spark_texture, &entity->position, state->width, state->height,
                          state->angle, state->frame, state->lifetime) < 0)
        ctx->error = BH_EFFECT_QUEUE;
}
static void __cdecl impact_tick(BHEntity *entity)
{
    BHContactEffects *ctx = context(entity);
    BHImpactState *state = impact(entity);
    if (!ctx || !bh_contact_impact_state(ctx, entity) || !bh_math_environment_supported())
        return;
    bh_round_float(&state->age, (double)state->age - (-1.0 / 30.0));
    if (state->age >= state->duration && !bh_entity_release(ctx->pool, entity))
        ctx->error = BH_EFFECT_POOL;
}
static int impact_valid(BHContactEffects *ctx, BHEntity *entity)
{
    BHImpactState *state;
    if (!ctx || !(state = bh_contact_impact_state(ctx, entity)) || !bh_math_environment_supported())
        return 0;
    if (!_finite(state->age) || !_finite(state->duration) || state->duration <= 0 || !_finite(state->size) ||
        !finite_vector(&state->normal))
    {
        ctx->error = BH_EFFECT_NUMERICAL;
        return 0;
    }
    return 1;
}
static void __cdecl quad_refresh(BHEntity *entity, void *packet)
{
    BHContactEffects *ctx = context(entity);
    BHImpactState *state = impact(entity);
    const BHVec3 up = {0, 0, 1};
    float alpha, size, angle;
    int value;
    if (!impact_valid(ctx, entity))
        return;
    bh_round_float(&alpha, 1.0 - (double)state->age / state->duration);
    bh_round_float(&size, (double)state->size * state->age);
    value = rand();
    bh_round_float(&angle, (double)value * (1.0 / 4096.0));
    if (bh_draw_oriented(ctx->queue, 3, ctx->quad_texture, &entity->position, &up, &state->normal, size, size, angle, 0,
                         alpha) < 0)
        ctx->error = BH_EFFECT_QUEUE;
}
static void __cdecl rays_refresh(BHEntity *entity, void *packet)
{
    BHContactEffects *ctx = context(entity);
    BHImpactState *state = impact(entity);
    BHVec3 up = {0, 0, 1}, end;
    float size, alpha, width, saved_growth;
    double ratio, growth, extent;
    int result;
    if (!impact_valid(ctx, entity))
        return;
    if (ctx->has_camera)
    {
        if (!_finite(ctx->camera_yaw))
        {
            ctx->error = BH_EFFECT_NUMERICAL;
            return;
        }
        bh_round_float(&up.x, sin((double)ctx->camera_yaw));
        bh_round_float(&up.z, cos((double)ctx->camera_yaw));
    }
    bh_round_float(&size, (double)state->size * state->age);
    bh_round_float(&alpha, (1.0 - (double)state->age / state->duration) * .7);
    result = bh_draw_oriented(ctx->queue, 3, ctx->rays_texture, &entity->position, &up, &state->normal, size, size, 0,
                              0, alpha);
    if (result < 0)
        ctx->error = BH_EFFECT_QUEUE;
    ratio = (double)state->age / state->duration;
    growth = (ratio + ratio) * (1.0 - ratio);
    bh_round_float(&saved_growth, growth);
    extent = growth * state->size;
    bh_round_float(&width, ((double)state->size * (1.0 - ratio)) * .25);
    bh_round_float(&end.x, (double)state->normal.x * extent + entity->position.x);
    bh_round_float(&end.y, (double)entity->position.y + (double)state->normal.y * extent);
    bh_round_float(&end.z, (double)entity->position.z + (double)state->normal.z * extent);
    result = bh_draw_segment(ctx->queue, 3, ctx->line_texture, &entity->position, &end, width, width, 1, 1, 0);
    if (result < 0)
        ctx->error = BH_EFFECT_QUEUE;
    extent = ((double)saved_growth * state->size) * .5;
    bh_round_float(&end.x, (double)entity->position.x + (double)state->normal.x * extent);
    bh_round_float(&end.y, (double)state->normal.y * extent + entity->position.y);
    bh_round_float(&end.z, (double)entity->position.z + (double)state->normal.z * extent);
    result = bh_draw_segment(ctx->queue, 3, ctx->line_texture, &end, &entity->position, width, width, 1, 1, 0);
    if (result < 0)
        ctx->error = BH_EFFECT_QUEUE;
}
