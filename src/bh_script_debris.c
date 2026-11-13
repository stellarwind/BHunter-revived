#include "bh_script_debris.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && offsetof(BHEntity, origin) == 0x48,
              "debris layout");
static BHScriptDebrisContext *context(BHEntity *entity)
{
    BHScriptDebrisContext *ctx;
    memcpy(&ctx, &entity->bindings.script_debris, 4);
    return ctx;
}
int bh_script_debris_read(const BHEntity *entity, BHScriptDebrisState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->lifetime, &entity->fragment.lifetime, 4);
    memcpy(&state->angle, &entity->fragment.angle, 4);
    memcpy(&state->spin, &entity->fragment.spin, 4);
    memcpy(&state->width, &entity->fragment.width, 4);
    memcpy(&state->height, &entity->fragment.height, 4);
    memcpy(&state->frame, &entity->fragment.frame, 4);
    memcpy(&state->origin_tag, &entity->fragment.origin_tag, 4);
    return 1;
}
int bh_script_debris_write(BHEntity *entity, const BHScriptDebrisState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&entity->fragment.lifetime, &state->lifetime, 4);
    memcpy(&entity->fragment.angle, &state->angle, 4);
    memcpy(&entity->fragment.spin, &state->spin, 4);
    memcpy(&entity->fragment.width, &state->width, 4);
    memcpy(&entity->fragment.height, &state->height, 4);
    memcpy(&entity->fragment.frame, &state->frame, 4);
    memcpy(&entity->fragment.origin_tag, &state->origin_tag, 4);
    return 1;
}
unsigned long bh_script_debris_create(BHScriptDebrisContext *ctx, long count, const BHVec3 *p, float lifetime,
                                      float speed, float size)
{
    BHEntity *entity;
    BHScriptDebrisState state;
    unsigned long created = 0;
    float length;
    double factor;
    int r;
    if (!ctx || !ctx->pool || !ctx->queue || !ctx->queue->commands || count < 0 || count > 65535 ||
        !bh_math_environment_supported())
    {
        if (ctx)
            ctx->error = BH_DEBRIS_ARGUMENT;
        return 0;
    }
    if (!count)
    {
        ctx->origin_tag = 0;
        ctx->error = BH_DEBRIS_OK;
        return 0;
    }
    if (!p)
    {
        ctx->error = BH_DEBRIS_ARGUMENT;
        return 0;
    }
    while (count--)
    {
        entity = bh_entity_acquire(ctx->pool);
        if (!entity)
        {
            ctx->error = BH_DEBRIS_POOL;
            return created;
        }
        entity->position = *p;
        state.lifetime = lifetime;
        state.origin_tag = ctx->origin_tag;
        state.frame = (unsigned long)rand() & 4;
        r = rand();
        entity->origin.x = (float)(r - 16384);
        r = rand();
        entity->origin.y = (float)(r - 16384);
        r = rand();
        entity->origin.z = (float)(r - 16384);
        bh_round_float(
            &length, sqrt(((double)entity->origin.x * entity->origin.x + (double)entity->origin.y * entity->origin.y) +
                          (double)entity->origin.z * entity->origin.z));
        r = rand();
        factor = (((.9 - (double)r * (-.000006103515625)) * speed) * (1.0 / 30.0)) / length;
        bh_round_float(&entity->origin.x, factor * entity->origin.x);
        bh_round_float(&entity->origin.y, factor * entity->origin.y);
        bh_round_float(&entity->origin.z, factor * entity->origin.z);
        r = rand();
        bh_round_float(&state.width, ((double)r * size) * (1.0 / 16384.0));
        r = rand();
        bh_round_float(&state.height, ((double)r * size) * (1.0 / 16384.0));
        r = rand();
        bh_round_float(&state.angle, (double)r * .0001953125);
        r = rand();
        bh_round_float(&state.spin, ((double)r - 16384.0) * (1.0 / 32768.0));
        bh_script_debris_write(entity, &state);
        memcpy(&entity->bindings.script_debris, &ctx, 4);
        entity->step = bh_script_debris_tick;
        entity->refresh = bh_script_debris_refresh;
        ++created;
    }
    ctx->origin_tag = 0;
    ctx->error = BH_DEBRIS_OK;
    return created;
}
void bh_script_debris_tick(BHEntity *entity)
{
    BHScriptDebrisContext *ctx = context(entity);
    BHScriptDebrisState state;
    BHVec3 old = entity->origin;
    double life, y, factor;
    float ry;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_DEBRIS_ARGUMENT;
        return;
    }
    bh_script_debris_read(entity, &state);
    life = (double)state.lifetime - (1.0 / 30.0);
    bh_round_float(&state.lifetime, life);
    memcpy(&entity->fragment.lifetime, &state.lifetime, 4);
    if (_isnan(life) || life < 0)
    {
        bh_entity_release(ctx->pool, entity);
        return;
    }
    y = (double)old.y - (-.010911111111111113);
    bh_round_float(&ry, y);
    factor = 1.0 - sqrt(((double)old.x * old.x + y * ry) + (double)old.z * old.z) * .0025;
    bh_round_float(&entity->position.x, (double)old.x + entity->position.x);
    bh_round_float(&entity->position.y, (double)old.y + entity->position.y);
    bh_round_float(&entity->position.z, (double)old.z + entity->position.z);
    entity->origin.y = ry;
    if (_isnan(factor) || factor < .5)
        factor = .5;
    bh_round_float(&entity->origin.x, factor * old.x);
    bh_round_float(&entity->origin.y, factor * y);
    bh_round_float(&entity->origin.z, factor * old.z);
    bh_round_float(&state.angle, (double)state.spin + state.angle);
    memcpy(&entity->fragment.angle, &state.angle, 4);
}
void bh_script_debris_refresh(BHEntity *entity, void *packet)
{
    BHScriptDebrisContext *ctx = context(entity);
    BHScriptDebrisState state;
    int r;
    (void)packet;
    bh_script_debris_read(entity, &state);
    r = bh_draw_billboard(ctx->queue, 0, ctx->image, &entity->position, state.width, state.height, state.angle,
                          state.frame, 1);
    if (r < 0)
        ctx->error = BH_DEBRIS_QUEUE;
}
