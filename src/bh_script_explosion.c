#include "bh_script_explosion.h"
#include "bh_callback_helpers.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

typedef struct BlastPacket
{
    long event;
    BHVec3 point;
    float energy, radius;
} BlastPacket;
typedef struct DamagePacket
{
    long event;
    float amount;
    unsigned long target;
} DamagePacket;
typedef char explosion_layout[sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && sizeof(BlastPacket) == 24 &&
                                      sizeof(DamagePacket) == 12
                                  ? 1
                                  : -1];
static void load(const BHEntity *entity, unsigned long offset, void *out, unsigned long n)
{
    memcpy(out, (const unsigned char *)entity + offset, n);
}
static void store(BHEntity *entity, unsigned long offset, const void *in, unsigned long n)
{
    memcpy((unsigned char *)entity + offset, in, n);
}
static BHScriptExplosionContext *context(const BHEntity *entity)
{
    BHScriptExplosionContext *ctx;
    memcpy(&ctx, entity->payload + 624, 4);
    return ctx;
}
static int live(BHScriptExplosionContext *ctx, BHEntity *entity, unsigned long generation)
{
    if (bh_entity_is_active(ctx->pool, entity) && entity->generation == generation)
        return 1;
    ctx->error = BH_EXPLOSION_MUTATION;
    return 0;
}
int __cdecl bh_script_explosion_read(const BHEntity *entity, BHScriptExplosionState *state)
{
    if (!entity || !state)
        return 0;
    load(entity, 0xe8, &state->age, 4);
    load(entity, 0xec, &state->duration, 4);
    load(entity, 0xf4, state->direction, 96);
    load(entity, 0x154, state->angle, 32);
    load(entity, 0x174, state->spin, 32);
    load(entity, 0x194, &state->radius, 4);
    load(entity, 0x198, &state->magnitude, 4);
    load(entity, 0x19c, &state->probability, 4);
    load(entity, 0x1ac, &state->sound_resource, 4);
    load(entity, 0x1b0, &state->origin_tag, 4);
    load(entity, 0x1b4, &state->target, 4);
    return 1;
}
int __cdecl bh_script_explosion_write(BHEntity *entity, const BHScriptExplosionState *state)
{
    if (!entity || !state)
        return 0;
    store(entity, 0xe8, &state->age, 4);
    store(entity, 0xec, &state->duration, 4);
    store(entity, 0xf4, state->direction, 96);
    store(entity, 0x154, state->angle, 32);
    store(entity, 0x174, state->spin, 32);
    store(entity, 0x194, &state->radius, 4);
    store(entity, 0x198, &state->magnitude, 4);
    store(entity, 0x19c, &state->probability, 4);
    store(entity, 0x1ac, &state->sound_resource, 4);
    store(entity, 0x1b0, &state->origin_tag, 4);
    store(entity, 0x1b4, &state->target, 4);
    return 1;
}
static int valid(BHScriptExplosionContext *ctx, const BHVec3 *p, float duration, float magnitude, float probability)
{
    return ctx && ctx->pool && ctx->queue && ctx->queue->commands && ctx->sound && ctx->debris && ctx->flash &&
           ctx->debris->pool == ctx->pool && ctx->debris->queue == ctx->queue && ctx->flash->pool == ctx->pool &&
           ctx->flash->queue == ctx->queue && p && _finite(p->x) && _finite(p->y) && _finite(p->z) &&
           _finite(duration) && _finite(magnitude) && _finite(probability) && magnitude >= 0 &&
           bh_math_environment_supported();
}
static BHEntity *create(BHScriptExplosionContext *ctx, const BHVec3 *p, float duration, float magnitude,
                        float probability, unsigned long sound, unsigned long target, int secondary)
{
    BHScriptExplosionState state;
    BHEntity *flash, *e;
    BHVec3 zero = {0, 0, 0};
    BlastPacket blast;
    DamagePacket damage;
    unsigned long i, generation;
    float speed, size, radius, square;
    double factor, count;
    int r;
    if (!valid(ctx, p, duration, magnitude, probability))
    {
        if (ctx)
            ctx->error = BH_EXPLOSION_ARGUMENT;
        return NULL;
    }
    ctx->error = BH_EXPLOSION_OK;
    bh_sound_request(ctx->sound, sound, 1, 1, p, &zero);
    flash = bh_entity_acquire(ctx->pool);
    if (flash)
    {
        flash->position = *p;
        bh_flash_flare_bind(ctx->flash, flash, .1f, 250, 4);
    }
    e = bh_entity_acquire(ctx->pool);
    if (!e)
    {
        ctx->error = BH_EXPLOSION_POOL;
        return NULL;
    }
    generation = e->generation;
    e->position = *p;
    state.age = 0;
    state.duration = duration;
    state.radius = 0;
    state.magnitude = magnitude;
    state.probability = probability;
    state.sound_resource = sound;
    state.target = target;
    state.origin_tag = ctx->debris->origin_tag;
    ctx->debris->origin_tag = 0;
    for (i = 0; i < 8; ++i)
    {
        state.direction[i].x = (float)(rand() - 16384);
        state.direction[i].y = (float)(rand() - 16384);
        state.direction[i].z = (float)(rand() - 16384);
        bh_vec3_normalize(state.direction + i);
        r = rand();
        factor = .5 - (double)r * (-1.0 / 65536.0);
        bh_round_float(&state.direction[i].x, factor * state.direction[i].x);
        bh_round_float(&state.direction[i].y, factor * state.direction[i].y);
        bh_round_float(&state.direction[i].z, factor * state.direction[i].z);
        r = rand();
        bh_round_float(state.angle + i, (double)r * (1.0 / 4096.0));
        r = rand();
        bh_round_float(state.spin + i, ((double)r - 16384.0) * .0000025);
    }
    bh_script_explosion_write(e, &state);
    memcpy(e->payload + 624, &ctx, 4);
    e->refresh = bh_script_explosion_refresh;
    e->step = bh_script_explosion_tick;
    bh_entity_set_query(ctx->pool, e, bh_actor_target_removed);
    blast.event = 1;
    blast.point = *p;
    bh_round_float(&square, (double)magnitude * magnitude);
    bh_round_float(&blast.energy, ((double)magnitude * magnitude) * 125.0);
    bh_round_float(&blast.radius, (double)magnitude * 5.0);
    if (!bh_entities_query(ctx->pool, 0, 0, NULL, 0, &blast))
        ctx->error = BH_EXPLOSION_QUERY;
    if (!live(ctx, e, generation))
        return NULL;
    bh_round_float(&size, (double)magnitude * .125);
    bh_round_float(&speed, (double)magnitude * 3.0);
    r = rand();
    count = 2.0 - ((double)r * magnitude) * (-1.0 / 32768.0);
    if (!_finite(count) || count < 0 || count >= 65536)
        ctx->error = BH_EXPLOSION_LIMIT;
    else
        bh_script_debris_create(ctx->debris, (long)count, p, 4, speed, size);
    if (!live(ctx, e, generation))
        return NULL;
    load(e, 0x1b0, &state.origin_tag, 4);
    if (!state.origin_tag)
    {
        damage.event = 0;
        load(e, 0x1b4, &damage.target, 4);
        damage.amount = secondary ? .4f : .6f;
        if (secondary)
            bh_round_float(&radius, (double)square * .05);
        else
            bh_round_float(&radius, (double)magnitude + magnitude);
        if (!bh_entities_query(ctx->pool, 0, 0, p, radius, &damage))
            ctx->error = BH_EXPLOSION_QUERY;
        if (!live(ctx, e, generation))
            return NULL;
        if (secondary)
        {
            damage.amount = .5f;
            bh_round_float(&radius, (double)magnitude * .5);
            if (!bh_entities_query(ctx->pool, 0, 0, p, radius, &damage))
                ctx->error = BH_EXPLOSION_QUERY;
            if (!live(ctx, e, generation))
                return NULL;
        }
    }
    return e;
}
BHEntity *__cdecl bh_script_explosion_create(BHScriptExplosionContext *ctx, const BHVec3 *p, float duration,
                                             float magnitude, float probability, unsigned long target)
{
    return create(ctx, p, duration, magnitude, probability, ctx ? ctx->sound_resource : 0, target, 0);
}
BHEntity *__cdecl bh_script_explosion_secondary(BHScriptExplosionContext *ctx, const BHVec3 *p, float duration,
                                                float magnitude, float probability, unsigned long sound,
                                                unsigned long target)
{
    return create(ctx, p, duration, magnitude, probability, sound, target, 1);
}
void __cdecl bh_script_explosion_tick(BHEntity *entity)
{
    BHScriptExplosionContext *ctx = context(entity);
    BHScriptExplosionState state;
    BHVec3 v, p;
    unsigned long generation = entity->generation, i;
    double age, threshold, old;
    float distance, duration, magnitude, probability;
    int r;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_EXPLOSION_ARGUMENT;
        return;
    }
    bh_script_explosion_read(entity, &state);
    age = (double)state.age - (-1.0 / 30.0);
    bh_round_float(&state.age, age);
    store(entity, 0xe8, &state.age, 4);
    if (!_isnan(age) && !_isnan(state.duration) && age > state.duration)
    {
        bh_entity_release(ctx->pool, entity);
        return;
    }
    r = rand();
    threshold = ((double)state.magnitude * state.probability) * 100.0;
    if (!_isnan(threshold) && threshold > r)
    {
        r = rand();
        bh_round_float(&distance, ((double)r * state.radius) * (1.0 / 32768.0));
        v.x = (float)(rand() - 16384);
        v.y = (float)(rand() - 16384);
        v.z = (float)(rand() - 16384);
        bh_vec3_normalize(&v);
        ctx->debris->origin_tag = state.origin_tag;
        bh_round_float(&v.x, (double)distance * v.x);
        bh_round_float(&v.y, (double)distance * v.y);
        bh_round_float(&v.z, (double)distance * v.z);
        bh_round_float(&p.x, (double)entity->position.x + v.x);
        bh_round_float(&p.y, (double)entity->position.y + v.y);
        bh_round_float(&p.z, (double)entity->position.z + v.z);
        bh_round_float(&duration, (double)state.duration - state.age);
        bh_round_float(&magnitude, (double)state.magnitude * 1.5);
        bh_round_float(&probability, (double)state.probability * .25);
        bh_script_explosion_secondary(ctx, &p, duration, magnitude, probability, state.sound_resource, state.target);
        if (!live(ctx, entity, generation))
            return;
        bh_script_explosion_read(entity, &state);
    }
    old = state.magnitude;
    bh_round_float(&state.magnitude, old * .95);
    bh_round_float(&state.radius, (double)state.radius - old * (-1.0 / 30.0));
    store(entity, 0x198, &state.magnitude, 4);
    store(entity, 0x194, &state.radius, 4);
    for (i = 0; i < 8; ++i)
    {
        bh_round_float(state.angle + i, (double)state.spin[i] + state.angle[i]);
        store(entity, 0x154 + i * 4, state.angle + i, 4);
    }
}
void __cdecl bh_script_explosion_refresh(BHEntity *entity, void *packet)
{
    BHScriptExplosionContext *ctx = context(entity);
    BHScriptExplosionState state;
    BHVec3 p;
    float size, alpha;
    unsigned long i;
    (void)packet;
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_EXPLOSION_ARGUMENT;
        return;
    }
    bh_script_explosion_read(entity, &state);
    bh_round_float(&alpha, 1.0 - (double)state.age / state.duration);
    bh_round_float(&size, (double)state.radius * .75 - ((double)state.radius * state.radius) * (-.25));
    for (i = 0; i < 8; ++i)
    {
        bh_round_float(&p.x, (double)state.direction[i].x * state.radius + entity->position.x);
        bh_round_float(&p.y, (double)state.direction[i].y * state.radius + entity->position.y);
        bh_round_float(&p.z, (double)state.direction[i].z * state.radius + entity->position.z);
        if (bh_draw_billboard(ctx->queue, 3, ctx->image, &p, size, size, state.angle[i], 0, alpha) < 0)
            ctx->error = BH_EXPLOSION_QUEUE;
    }
}
