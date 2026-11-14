#include "../Entities/bh_field_access.h"
#include "bh_effect_projectile.h"
#include "../../Math/bh_round.h"
#include "bh_effect_orbit_tick.h"
#include "bh_effect_split_tick.h"
#include "bh_effect_accelerated.h"
#include "../../Math/bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static float rf(double d)
{
    float f;
    bh_round_float(&f, d);
    return f;
}
static BHEffectProjectileContext *context(BHEntity *entity)
{
    BHEffectProjectileContext *ctx;
    memcpy(&ctx, &entity->bindings.effect_projectile, 4);
    return ctx;
}
static int live(BHEffectProjectileContext *ctx, BHEntity *entity)
{
    unsigned long g;
    memcpy(&g, &entity->bindings.projectile_generation, 4);
    return bh_entity_is_active(ctx->effects->pool, entity) && entity->generation == g;
}
int bh_effect_projectile_bind(BHEffectProjectileContext *ctx, BHEntity *entity, int orbit)
{
    BHEntityStep step;
    unsigned long handle;
    if (!ctx || !ctx->effects || !ctx->effects->pool || !ctx->effects->sound || !ctx->effects->particles ||
        !ctx->effects->flash || !ctx->debris || !entity)
        return 0;
    if (ctx->effects->particles->pool != ctx->effects->pool || ctx->effects->flash->pool != ctx->effects->pool ||
        ctx->debris->pool != ctx->effects->pool)
        return 0;
    ctx->cleanup.pool = ctx->effects->pool;
    ctx->cleanup.sound = ctx->effects->sound;
    step = entity->step;
    memcpy(&handle, &entity->effect.sound, 4);
    if (!bh_effect_cleanup_bind(&ctx->cleanup, entity, handle))
        return 0;
    entity->step = step;
    memcpy(&entity->bindings.effect_projectile, &ctx, 4);
    memcpy(&entity->bindings.projectile_generation, &entity->generation, 4);
    return bh_entity_set_query(ctx->effects->pool, entity, orbit ? bh_effect_orbit_query : bh_effect_linear_query);
}
static int random_value(BHEffectProjectileContext *ctx)
{
    ++ctx->random_calls;
    return rand();
}
static void flash(BHEffectProjectileContext *ctx, BHEntity *entity, float strength, float size)
{
    BHEntity *f = bh_entity_acquire(ctx->effects->pool);
    if (f)
    {
        f->position = entity->position;
        bh_flash_flare_bind(ctx->effects->flash, f, .1f, strength, size);
        ++ctx->flashes;
    }
}
static void particle(BHEffectProjectileContext *ctx, BHEntity *entity, const BHVec3 *v, float size, float size_rate,
                     float lifetime, float alpha)
{
    BHVec3 gravity = {0, -.002f, 0};
    float spin, angle;
    int r;
    r = random_value(ctx);
    spin = rf(((double)r - 16384) * (1.0 / 16384.0));
    r = random_value(ctx);
    angle = rf((double)r * (1.0 / 512.0));
    if (bh_script_particle_create(ctx->effects->particles, ctx->smoke_image, 3, &entity->position, v, &gravity, size,
                                  angle, size_rate, spin, .9f, .95f, lifetime, alpha))
        ++ctx->particles;
}
static void terminate_effects(BHEffectProjectileContext *ctx, BHEntity *entity)
{
    unsigned long handle;
    BHVec3 v;
    int i, j;
    float *o = &v.x;
    bh_field_store_f32(&entity->effect.lifetime, 3);
    entity->origin = zero;
    memcpy(&entity->acceleration, &zero, 12);
    memcpy(&handle, &entity->effect.sound, 4);
    bh_sound_stop(ctx->effects->sound, handle);
    bh_field_store_u32(&entity->effect.sound, BH_SOUND_INVALID);
    bh_sound_request(ctx->effects->sound, ctx->blast_sound, 4, 1, &entity->position, &zero);
    bh_sound_request(ctx->effects->sound, ctx->impact_sound, 1, 1, &entity->position, &zero);
    bh_field_store_u32(&entity->effect.split_count, 1);
    flash(ctx, entity, 500, 8);
    bh_script_debris_create(ctx->debris, 10, &entity->position, 2, 20, .5f);
    for (i = 0; i < 6; ++i)
    {
        for (j = 0; j < 3; ++j)
            o[j] = rf(((double)random_value(ctx) - 16384) * (1.0 / 262144.0));
        particle(ctx, entity, &v, .5f, 20, 1, .8f);
    }
}
static void detach(BHEffectProjectileContext *ctx, BHEntity *entity, BHEffectPacket *p)
{
    unsigned long n;
    memcpy(&n, &entity->effect.target, 4);
    if (n == (unsigned long)p->words[1])
        bh_field_store_u32(&entity->effect.target, 0);
    memcpy(&n, &entity->effect.source, 4);
    if (n == (unsigned long)p->words[1])
        bh_field_store_u32(&entity->effect.source, 0);
    (void)ctx;
}
static void retire(BHEffectProjectileContext *ctx, BHEntity *entity)
{
    entity->step = bh_effect_sound_cleanup;
    bh_entity_set_query(ctx->effects->pool, entity, NULL);
    entity->refresh = NULL;
}
static void synchronize(BHEffectProjectileContext *ctx, BHEntity *entity, BHEffectPacket *p, int orbit)
{
    unsigned long id;
    long kind;
    BHVec3 v;
    double speed;
    int i;
    float *o = &v.x;
    memcpy(&id, &entity->effect.origin_tag, 4);
    if ((unsigned long)p->words[1] != id)
        return;
    p->words[1] = -1;
    kind = p->words[2];
    if (kind == 0 || kind == 1)
    {
        bh_field_store_f32(&entity->effect.lifetime, 0);
        if (!kind)
        {
            ++entity->generation;
            memcpy(&entity->bindings.projectile_generation, &entity->generation, 4);
        }
        memcpy(&entity->position, p->floats + 4, 12);
        return;
    }
    if (kind == 2)
    {
        memcpy(&entity->position, p->floats + 4, 12);
        terminate_effects(ctx, entity);
        return;
    }
    if (kind != 4 || orbit)
        return;
    memcpy(&entity->position, p->floats + 4, 12);
    memcpy(&v, &entity->effect.velocity, 12);
    speed = sqrt(((double)v.x * v.x + (double)v.y * v.y) + (double)v.z * v.z);
    for (i = 0; i < 3; ++i)
        bh_field_store_f32(&((float *)&entity->effect.velocity)[i], rf((double)p->floats[7 + i] * speed));
    for (i = 0; i < 3; ++i)
        o[i] = rf(((double)random_value(ctx) - 16384) * (1.0 / 262144.0));
    flash(ctx, entity, 100, 4);
    bh_sound_request(ctx->effects->sound, ctx->ricochet_sound, 1, 2, &entity->position, &zero);
    particle(ctx, entity, &v, .1f, 4, .5f, 1);
}
void bh_effect_orbit_query(BHEntity *entity, void *packet)
{
    BHEffectProjectileContext *ctx = context(entity);
    BHEffectPacket *p = (BHEffectPacket *)packet;
    float sn, cs;
    double phase;
    BHVec3 a, b;
    if (!ctx || !p || !live(ctx, entity))
        return;
    switch (p->words[0])
    {
    case 3:
        phase = (double)bh_field_f32(&entity->effect.lifetime) * 50;
        sn = rf(sin(phase));
        cs = rf(cos(phase));
        memcpy(&a, &entity->effect.orbit_first, 12);
        memcpy(&b, &entity->effect.orbit_second, 12);
        p->floats[1] = rf(((double)entity->position.x + (double)a.x * cs) + (double)b.x * sn);
        p->floats[2] = rf(((double)entity->position.y + (double)a.y * cs) + (double)b.y * sn);
        p->floats[3] = rf(((double)entity->position.z + (double)a.z * cs) + (double)b.z * sn);
        break;
    case 17:
        detach(ctx, entity, p);
        break;
    case 18:
        synchronize(ctx, entity, p, 1);
        break;
    case 20:
        retire(ctx, entity);
        break;
    default:
        break;
    }
}
void bh_effect_linear_query(BHEntity *entity, void *packet)
{
    BHEffectProjectileContext *ctx = context(entity);
    BHEffectPacket *p = (BHEffectPacket *)packet;
    if (!ctx || !p || !live(ctx, entity))
        return;
    if (p->words[0] == 17)
        detach(ctx, entity, p);
    else if (p->words[0] == 18)
        synchronize(ctx, entity, p, 0);
    else if (p->words[0] == 20)
        retire(ctx, entity);
}

static void directional(BHEntity *entity, void *p)
{
    BHEffectProjectileContext *ctx = context(entity);
    (void)p;
    if (ctx && ctx->draw)
        bh_effect_directional_draw(ctx->draw, entity);
}
static void variable(BHEntity *entity, void *p)
{
    BHEffectProjectileContext *ctx = context(entity);
    (void)p;
    if (ctx && ctx->draw)
        bh_effect_variable_draw(ctx->draw, entity);
}
static void fixed(BHEntity *entity, void *p)
{
    BHEffectProjectileContext *ctx = context(entity);
    (void)p;
    if (ctx && ctx->draw)
        bh_effect_fixed_draw(ctx->draw, entity);
}
static void scaled(BHVec3 *out, const BHVec3 *v, double n)
{
    out->x = rf((double)v->x * n);
    out->y = rf((double)v->y * n);
    out->z = rf((double)v->z * n);
}
static int constructor_context(BHEffectProjectileContext *ctx)
{
    return ctx && ctx->effects && ctx->effects->pool && ctx->effects->sound && ctx->draw && ctx->draw->queue &&
           ctx->debris;
}
static BHEntity *acquire(BHEffectProjectileContext *ctx, BHWorldGeometry *world, BHEntity *source, const BHVec3 *p,
                         float life, float grace)
{
    BHEntity *entity = bh_entity_acquire(ctx->effects->pool);
    if (!entity)
        return NULL;
    entity->position = *p;
    bh_field_store_f32(&entity->effect.lifetime, life);
    bh_field_store_f32(&entity->effect.grace, grace);
    bh_field_store_u32(&entity->effect.origin_tag, ctx->debris->origin_tag);
    ctx->debris->origin_tag = 0;
    memcpy(&entity->effect.source, &source, 4);
    memcpy(&entity->effect.world, &world, 4);
    return entity;
}
static void identity(BHEffectProjectileContext *ctx, BHEntity *entity)
{
    if (ctx->pending_identity)
    {
        bh_field_store_u32(&entity->effect.sync_identity, *ctx->pending_identity);
        *ctx->pending_identity = 0;
    }
}
BHEntity *bh_effect_orbit_create(BHEffectProjectileContext *ctx, BHWorldGeometry *world, BHEntity *source,
                                 const BHVec3 *p, const BHVec3 *v, const BHVec3 *a, const BHVec3 *b, const BHVec3 *acc,
                                 float life, BHEntity *target)
{
    BHEntity *entity;
    unsigned long voice;
    if (!constructor_context(ctx) || !ctx->orbit || !ctx->trails || !p || !v || !a || !b || !acc)
        return NULL;
    entity = acquire(ctx, world, source, p, life, 1);
    if (!entity)
        return NULL;
    scaled(&entity->effect.velocity, v, 1.0 / 30.0);
    scaled(&entity->effect.acceleration, acc, 1.0 / 15.0);
    scaled(&entity->effect.orbit_first, a, .1);
    scaled(&entity->effect.orbit_second, b, .1);
    bh_field_store_u32(&entity->effect.sound, BH_SOUND_INVALID);
    ctx->orbit->core = ctx;
    if (!bh_effect_orbit_tick_bind(ctx->orbit, entity))
    {
        ctx->error = 1;
        bh_entity_release(ctx->effects->pool, entity);
        return NULL;
    }
    entity->refresh = directional;
    bh_trail_create_pair(ctx->trails, ctx->trail_image, 3, 0, p, entity, life, 2, 30, 1, 0, 1, &zero, 16, NULL, NULL);
    voice = bh_sound_request(ctx->effects->sound, ctx->loop_sound, 1, 1, p, v);
    bh_field_store_u32(&entity->effect.sound, voice);
    memcpy(&entity->effect.ignore, &source, 4);
    memcpy(&entity->effect.target, &target, 4);
    identity(ctx, entity);
    return entity;
}
static BHEntity *split_create(BHEffectProjectileContext *ctx, BHWorldGeometry *world, BHEntity *source, const BHVec3 *p,
                              const BHVec3 *v, float life, int count, unsigned long model, float multiplier, int shard)
{
    BHEntity *entity;
    BHEntity *ignore;
    unsigned long voice;
    if (!constructor_context(ctx) || !ctx->split || !ctx->effects->segment || !ctx->effects->contacts ||
        !ctx->effects->markers || !ctx->explosion || !ctx->trails || !p || !v)
        return NULL;
    entity = acquire(ctx, world, source, p, life, 1);
    if (!entity)
        return NULL;
    bh_field_store_u32(&entity->effect.model_resource, model);
    scaled(&entity->effect.velocity, v, 1.0 / 30.0);
    bh_field_store_u32(&entity->effect.sound, BH_SOUND_INVALID);
    if (!bh_effect_projectile_bind(ctx, entity, 0))
    {
        ctx->error = 1;
        bh_entity_release(ctx->effects->pool, entity);
        return NULL;
    }
    entity->step = bh_effect_split_tick;
    entity->refresh = variable;
    voice = shard ? BH_SOUND_INVALID : bh_sound_request(ctx->effects->sound, ctx->loop_sound, 1, .5f, p, v);
    bh_field_store_u32(&entity->effect.sound, voice);
    ignore = shard ? NULL : source;
    memcpy(&entity->effect.ignore, &ignore, 4);
    bh_field_store_u32(&entity->effect.split_count, (unsigned long)count);
    bh_field_store_f32(&entity->effect.multiplier, multiplier);
    bh_field_store_f32(&entity->effect.roll, rf((double)random_value(ctx) * .0001953125));
    identity(ctx, entity);
    return entity;
}
BHEntity *bh_effect_split_create(BHEffectProjectileContext *ctx, BHWorldGeometry *world, BHEntity *entity,
                                 const BHVec3 *p, const BHVec3 *v, float life, int count, unsigned long model,
                                 float multiplier)
{
    return split_create(ctx, world, entity, p, v, life, count, model, multiplier, 0);
}
BHEntity *bh_effect_shard_create(BHEffectProjectileContext *ctx, BHWorldGeometry *world, BHEntity *entity,
                                 const BHVec3 *p, const BHVec3 *v, float life, int count, unsigned long model,
                                 float multiplier)
{
    return split_create(ctx, world, entity, p, v, life, count, model, multiplier, 1);
}
BHEntity *bh_effect_accelerated_create(BHEffectProjectileContext *ctx, BHWorldGeometry *world, BHEntity *source,
                                       const BHVec3 *p, const BHVec3 *v, const BHVec3 *acc)
{
    BHEntity *entity;
    unsigned long voice;
    if (!constructor_context(ctx) || !ctx->accelerated || !ctx->accelerated->puffs || !ctx->effects->segment ||
        !ctx->effects->contacts || !ctx->effects->markers || !ctx->explosion || !ctx->trails || !p || !v || !acc)
        return NULL;
    entity = acquire(ctx, world, source, p, 0, 2);
    if (!entity)
        return NULL;
    entity->origin = *v;
    memcpy(&entity->acceleration, acc, 12);
    bh_field_store_u32(&entity->effect.sound, BH_SOUND_INVALID);
    if (!bh_effect_projectile_bind(ctx, entity, 0))
    {
        ctx->error = 1;
        bh_entity_release(ctx->effects->pool, entity);
        return NULL;
    }
    entity->step = bh_effect_accelerated_tick;
    entity->refresh = fixed;
    memcpy(&entity->effect.ignore, &source, 4);
    bh_field_store_f32(&entity->effect.roll, rf((double)random_value(ctx) * .0001953125));
    bh_field_store_u32(&entity->effect.split_count, 0);
    bh_field_store_f32(&entity->effect.multiplier, 0);
    voice = bh_sound_request(ctx->effects->sound, ctx->second_loop_sound, 0, .5f, p, v);
    bh_field_store_u32(&entity->effect.sound, voice);
    identity(ctx, entity);
    return entity;
}

void bh_effect_detonate(BHEffectProjectileContext *ctx, BHEntity *entity)
{
    terminate_effects(ctx, entity);
}
