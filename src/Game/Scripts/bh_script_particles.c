#include "bh_script_particles.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
static_assert(sizeof(BHScriptParticleState) == 84 && offsetof(BHScriptParticleState, velocity) == 60 &&
                  offsetof(BHEntity, payload) == 0xe4,
              "particle layout");
static BHScriptParticleState *state(BHEntity *entity)
{
    return (BHScriptParticleState *)(entity->payload + 4);
}
static BHScriptParticleContext *context(BHEntity *entity)
{
    BHScriptParticleContext *ctx;
    memcpy(&ctx, &entity->bindings.script_particles, 4);
    return ctx;
}
static int live(BHEntityPool *pool, BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
int bh_script_particles_init(BHScriptParticleContext *ctx, BHEntityPool *pool, BHDrawQueue *queue)
{
    if (!ctx || !pool || !queue || !queue->commands || !bh_entities_valid(pool))
        return 0;
    ctx->pool = pool;
    ctx->queue = queue;
    ctx->error = BH_PARTICLE_OK;
    return 1;
}
BHScriptParticleState *bh_script_particle_state(BHScriptParticleContext *ctx, BHEntity *entity)
{
    if (!ctx || !entity || !live(ctx->pool, entity) || entity->step != bh_script_particle_tick ||
        entity->refresh != bh_script_particle_refresh || context(entity) != ctx)
        return NULL;
    return state(entity);
}
BHEntity *bh_script_particle_create(BHScriptParticleContext *ctx, unsigned long resource, unsigned long bucket,
                                    const BHVec3 *p, const BHVec3 *v, const BHVec3 *a, float size, float angle,
                                    float size_rate, float angular_rate, float size_decay, float angular_decay,
                                    float lifetime, float alpha)
{
    BHEntity *entity;
    BHScriptParticleState *script_particle_state;
    BHVec3 zero = {0, 0, 0};
    if (!ctx || !ctx->pool || !ctx->queue || !p || !bh_math_environment_supported())
    {
        if (ctx)
            ctx->error = BH_PARTICLE_ARGUMENT;
        return NULL;
    }
    entity = bh_entity_acquire(ctx->pool);
    if (!entity)
    {
        ctx->error = BH_PARTICLE_POOL;
        return NULL;
    }
    script_particle_state = state(entity);
    entity->position = *p;
    script_particle_state->lifetime = lifetime;
    script_particle_state->velocity = v ? *v : zero;
    script_particle_state->acceleration = a ? *a : zero;
    script_particle_state->size = size;
    script_particle_state->angle = angle;
    bh_round_float(&script_particle_state->size_step, (double)size_rate * (1.0 / 30.0));
    bh_round_float(&script_particle_state->angular_step, (double)angular_rate * (1.0 / 30.0));
    script_particle_state->size_decay = size_decay;
    script_particle_state->angular_decay = angular_decay;
    script_particle_state->alpha = alpha;
    bh_round_float(&script_particle_state->alpha_step, ((double)alpha * (-1.0 / 30.0)) / lifetime);
    script_particle_state->frame = (unsigned long)rand() & 7;
    script_particle_state->resource = resource;
    script_particle_state->bucket = bucket;
    memcpy(&entity->bindings.script_particles, &ctx, 4);
    entity->step = bh_script_particle_tick;
    entity->refresh = bh_script_particle_refresh;
    ctx->error = BH_PARTICLE_OK;
    return entity;
}
void bh_script_particle_tick(BHEntity *entity)
{
    BHScriptParticleContext *ctx = context(entity);
    BHScriptParticleState *script_particle_state = state(entity);
    double next, x;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    next = (double)script_particle_state->lifetime - (1.0 / 30.0);
    bh_round_float(&script_particle_state->lifetime, next);
    if (!(next > 0))
    {
        bh_entity_release(ctx->pool, entity);
        return;
    }
    /* X's velocity sum survives its binary32 store on the original x87 stack.
    Y/Z position additions reload their rounded stored velocity instead. */
    x = (double)script_particle_state->acceleration.x + script_particle_state->velocity.x;
    bh_round_float(&script_particle_state->velocity.x, x);
    bh_round_float(&script_particle_state->velocity.y, (double)script_particle_state->acceleration.y + script_particle_state->velocity.y);
    bh_round_float(&script_particle_state->velocity.z, (double)script_particle_state->acceleration.z + script_particle_state->velocity.z);
    bh_round_float(&entity->position.x, (double)entity->position.x + x);
    bh_round_float(&entity->position.y, (double)script_particle_state->velocity.y + entity->position.y);
    bh_round_float(&entity->position.z, (double)script_particle_state->velocity.z + entity->position.z);
    bh_round_float(&script_particle_state->angle, (double)script_particle_state->angular_step + script_particle_state->angle);
    bh_round_float(&script_particle_state->size, (double)script_particle_state->size_step + script_particle_state->size);
    bh_round_float(&script_particle_state->angular_step, (double)script_particle_state->angular_decay * script_particle_state->angular_step);
    bh_round_float(&script_particle_state->size_step, (double)script_particle_state->size_decay * script_particle_state->size_step);
    bh_round_float(&script_particle_state->alpha, (double)script_particle_state->alpha_step + script_particle_state->alpha);
}
void bh_script_particle_refresh(BHEntity *entity, void *packet)
{
    BHScriptParticleContext *ctx = context(entity);
    BHScriptParticleState *script_particle_state = state(entity);
    int r;
    (void)packet;
    r = bh_draw_billboard(ctx->queue, script_particle_state->bucket, script_particle_state->resource, &entity->position, script_particle_state->size, script_particle_state->size, script_particle_state->angle, script_particle_state->frame,
                          script_particle_state->alpha);
    if (r < 0)
        ctx->error = BH_PARTICLE_QUEUE;
}
static BHScriptEmitterContext *emitter_context(BHEntity *entity)
{
    BHScriptEmitterContext *ctx;
    memcpy(&ctx, &entity->bindings.script_emitter, 4);
    return ctx;
}
static const BHScriptTimerActor *actor_record(BHScriptEmitterContext *ctx, BHEntity *entity)
{
    unsigned long i;
    for (i = 0; i < ctx->lifetime->timers.actor_count; ++i)
        if (ctx->lifetime->timers.actors[i].entity == entity)
            return ctx->lifetime->timers.actors + i;
    return NULL;
}
int bh_script_emitter_read(BHEntity *entity, BHScriptEmitterState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->actor, &entity->emitter.actor, 4);
    memcpy(&state->script_loaded, &entity->emitter.script_loaded, 4);
    memcpy(&state->countdown, &entity->emitter.countdown, 4);
    memcpy(&state->spread, &entity->emitter.spread, 4);
    memcpy(&state->velocity, &entity->emitter.velocity, 12);
    memcpy(&state->acceleration, &entity->emitter.acceleration, 12);
    memcpy(&state->size, &entity->emitter.size, 4);
    return 1;
}
int bh_script_emitter_bind(BHScriptEmitterContext *ctx, BHEntity *entity, const BHScriptEmitterState *state, int steam)
{
    BHScriptLifetimeState base;
    unsigned char retained[12];
    if (!ctx || !ctx->lifetime || !ctx->particles || !ctx->particles->pool || !ctx->lifetime->timers.pool ||
        ctx->particles->pool != ctx->lifetime->timers.pool || !entity || !state || !actor_record(ctx, state->actor))
        return 0;
    memcpy(retained, &entity->script_lifetime.sound, 4);
    memcpy(retained + 4, &entity->script_lifetime.mission_resource, 4);
    memcpy(retained + 8, &entity->script_lifetime.mission_countdown, 4);
    memset(&base, 0, sizeof(base));
    base.actor = state->actor;
    base.script_loaded = state->script_loaded;
    base.countdown = state->countdown;
    base.delta = state->velocity;
    if (!bh_script_lifetime_bind(ctx->lifetime, entity, &base, bh_script_sensor_query))
        return 0;
    memcpy(&entity->script_lifetime.sound, retained, 4);
    memcpy(&entity->script_lifetime.mission_resource, retained + 4, 4);
    memcpy(&entity->script_lifetime.mission_countdown, retained + 8, 4);
    memcpy(&entity->emitter.spread, &state->spread, 4);
    memcpy(&entity->emitter.acceleration, &state->acceleration, 12);
    memcpy(&entity->emitter.size, &state->size, 4);
    memcpy(&entity->bindings.script_emitter, &ctx, 4);
    entity->step = steam ? bh_script_steam_tick : bh_script_fountain_tick;
    entity->refresh = steam ? bh_script_steam_refresh : NULL;
    ctx->error = BH_PARTICLE_OK;
    return 1;
}
static int active(BHScriptEmitterContext *ctx, BHEntity *entity)
{
    const BHScriptTimerActor *script_timer_actor = actor_record(ctx, entity);
    if (!script_timer_actor)
    {
        ctx->error = BH_PARTICLE_ACTOR;
        return 0;
    }
    if (script_timer_actor->owner)
        script_timer_actor = script_timer_actor->owner;
    return script_timer_actor->active != 0;
}
static int countdown(BHEntity *entity, float value)
{
    double n;
    if (!(value > 0))
        return 0;
    n = (double)value - (1.0 / 30.0);
    bh_round_float(&value, n);
    memcpy(&entity->emitter.countdown, &value, 4);
    return !(n > 0);
}
void bh_script_fountain_tick(BHEntity *entity)
{
    BHScriptEmitterContext *ctx = emitter_context(entity);
    BHScriptEmitterState state;
    int expires, r;
    float spin, rate, angle;
    bh_script_emitter_read(entity, &state);
    if (!active(ctx, state.actor))
        return;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    expires = countdown(entity, state.countdown);
    if (expires || rand() < 11000)
    {
        r = rand();
        bh_round_float(&spin, (double)(r - 16384) * (1.0 / 131072.0));
        r = rand();
        bh_round_float(&rate, 10.0 - (double)r * (-1.0 / 2048.0));
        r = rand();
        bh_round_float(&angle, (double)r * (1.0 / 4096.0));
        bh_script_particle_create(ctx->particles, ctx->fountain_texture, 3, &state.actor->position, &state.velocity,
                                  &state.acceleration, .1f, angle, rate, spin, .98f, .95f, 1, 1);
    }
    if (expires)
        bh_entity_release(ctx->lifetime->timers.pool, entity);
}
void bh_script_steam_tick(BHEntity *entity)
{
    BHScriptEmitterContext *ctx = emitter_context(entity);
    BHScriptEmitterState state;
    BHVec3 v;
    int expires, r;
    float spin, angle;
    bh_script_emitter_read(entity, &state);
    if (!active(ctx, state.actor))
        return;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    expires = countdown(entity, state.countdown);
    if (expires || rand() < 10000)
    {
        v = state.velocity;
        r = rand();
        bh_round_float(&v.x, (double)(r - 16384) * state.spread + v.x);
        r = rand();
        bh_round_float(&v.y, (double)(r - 16384) * state.spread + v.y);
        r = rand();
        bh_round_float(&v.z, (double)(r - 16384) * state.spread + v.z);
        r = rand();
        bh_round_float(&spin, (double)(r - 16384) * (1.0 / 131072.0));
        r = rand();
        bh_round_float(&angle, (double)r * (1.0 / 4096.0));
        bh_script_particle_create(ctx->particles, ctx->steam_texture, 3, &state.actor->position, &v,
                                  &state.acceleration, .2f, angle, 4, spin, 1, .95f, 3, 1);
    }
    if (expires)
        bh_entity_release(ctx->lifetime->timers.pool, entity);
}
void bh_script_steam_refresh(BHEntity *entity, void *packet)
{
    BHScriptEmitterContext *ctx = emitter_context(entity);
    BHScriptEmitterState state;
    BHVec3 p, up = {0, 0, 1}, normal = {0, -1, 0};
    unsigned long frame;
    float size, angle;
    int r;
    (void)packet;
    bh_script_emitter_read(entity, &state);
    if (!active(ctx, state.actor))
        return;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    bh_round_float(&size, (double)state.size);
    frame = (unsigned long)rand() & 7;
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    p = state.actor->position;
    bh_round_float(&p.y, (double)p.y - 1.6);
    if (bh_draw_oriented(ctx->particles->queue, 3, ctx->steam_texture, &p, &up, &normal, size, size, angle, frame, 1) <
        0)
        ctx->error = BH_PARTICLE_QUEUE;
}
