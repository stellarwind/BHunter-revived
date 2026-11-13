#include "bh_field_access.h"
#include "bh_effect_accelerated.h"
#include "bh_delayed_expiry.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static float rf(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static BHEffectProjectileContext *context(BHEntity *entity)
{
    BHEffectProjectileContext *ctx;
    memcpy(&ctx, &entity->bindings.effect_projectile, 4);
    return ctx;
}
static int random_value(BHEffectProjectileContext *ctx)
{
    ++ctx->random_calls;
    return rand();
}
static void offset(BHVec3 *p, const BHVec3 *a, const BHVec3 *n, double d)
{
    p->x = rf((double)a->x + (double)n->x * d);
    p->y = rf((double)a->y + (double)n->y * d);
    p->z = rf((double)a->z + (double)n->z * d);
}
static void retire(BHEffectProjectileContext *ctx, BHEntity *entity)
{
    bh_sound_stop(ctx->effects->sound, bh_field_u32(&entity->effect.sound));
    bh_entity_release(ctx->effects->pool, entity);
}
static BHEntity *response(BHHomingProjectileContext *ctx, const BHWorldSegmentHit *h)
{
    unsigned long i;
    if (h->object.kind != BH_WORLD_HIT_GEOMETRY)
        return h->object.object.actor;
    {
        const BHHomingWorldResponse *binding = bh_homing_world_response(ctx, h->object.object.geometry);
        if (binding)
        {
            return binding->receiver;
        }
    }
    return NULL;
}
static void puff(BHEffectProjectileContext *ctx, BHEntity *entity, unsigned long image, float life, float delay,
                 float size, int ring)
{
    BHEntity *p = bh_entity_acquire(ctx->effects->pool);
    BHEffectAcceleratedContext *a = ctx->accelerated;
    unsigned long seed;
    if (!p)
        return;
    p->position = entity->position;
    memcpy(&p->puff.image, &image, 4);
    bh_field_store_f32(&p->puff.duration, life);
    bh_field_store_f32(&p->puff.size, size);
    bh_delayed_expiry_bind(ctx->effects->pool, p, life, delay);
    if (!ring)
    {
        seed = (unsigned long)random_value(ctx);
        seed = seed * 32768UL + (unsigned long)random_value(ctx);
        seed = seed * 32768UL + (unsigned long)random_value(ctx);
        memcpy(&p->puff.seed, &seed, 4);
    }
    bh_effect_puff_bind(a->puffs, p, ring);
    ++a->puff_entities;
}
void bh_effect_accelerated_tick(BHEntity *entity)
{
    BHEffectProjectileContext *ctx = context(entity);
    BHEffectAcceleratedContext *a;
    BHHomingProjectileContext *b;
    BHVec3 v, acc, end, old, p, n, smoke, gravity = {0, -.002f, 0}, drift = {0, -1, 0};
    BHWorldGeometry *cached;
    BHEntity *ignore, *owner, *impact, *receiver, *marker;
    BHImpactState *state;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    unsigned long generation, i, voice, suppress;
    double next, x, y, z, speed;
    float r1, r2, spin, angle, pitch;
    int result, world;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } damage;
    if (!ctx || !ctx->accelerated || !bh_math_environment_supported())
        return;
    a = ctx->accelerated;
    b = ctx->effects;
    memcpy(&generation, &entity->bindings.projectile_generation, 4);
    if (entity->generation != generation)
    {
        a->error = 1;
        return;
    }
    next = (double)bh_field_f32(&entity->effect.grace) - 1.0 / 30.0;
    bh_field_store_f32(&entity->effect.grace, rf(next));
    if (next < 0)
    {
        ignore = NULL;
        memcpy(&entity->effect.ignore, &ignore, 4);
    }
    next = (double)bh_field_f32(&entity->effect.multiplier) + 1.0 / 30.0;
    bh_field_store_f32(&entity->effect.multiplier, rf(next));
    if (next > 20)
    {
        retire(ctx, entity);
        return;
    }
    for (i = 0; i < 2; ++i)
    {
        r1 = rf((double)random_value(ctx) / 32768.0);
        r2 = rf((double)random_value(ctx) / 32768.0);
        bh_contact_spark_create(b->contacts, &entity->position, r2, r1);
    }
    smoke.x = rf(((double)random_value(ctx) - 16384) * .000002);
    smoke.y = rf(((double)random_value(ctx) - 16384) * .000002);
    smoke.z = rf(((double)random_value(ctx) - 16384) * .000002);
    if (!(random_value(ctx) & 3))
    {
        pitch = rf(2.0 - (double)random_value(ctx) * (-1.0 / 16384.0));
        bh_sound_request(b->sound, ctx->ricochet_sound, .5f, pitch, &entity->position, &entity->origin);
        spin = rf(((double)random_value(ctx) - 16384) / 16384.0);
        angle = rf((double)random_value(ctx) / 512.0);
        bh_script_particle_create(b->particles, ctx->smoke_image, 3, &entity->position, &smoke, &gravity, .2f, angle, 6,
                                  spin, .9f, .95f, .6f, .6f);
    }
    if (!bh_field_u32(&entity->effect.split_count))
    {
        old = entity->position;
        memcpy(&acc, &entity->acceleration, 12);
        x = (double)entity->origin.x + (double)acc.x / 30.0;
        y = (double)entity->origin.y + (double)acc.y / 30.0;
        z = (double)entity->origin.z + (double)acc.z / 30.0;
        entity->origin.x = rf(x);
        entity->origin.y = rf(y);
        entity->origin.z = rf(z);
        bh_field_store_f32(&entity->euler.z, 0);
        bh_field_store_f32(&entity->euler.x, rf(atan2(entity->origin.x, entity->origin.z)));
        bh_field_store_f32(&entity->euler.y,
                           rf(atan2(-entity->origin.y, sqrt(z * z + (double)entity->origin.x * entity->origin.x))));
        offset(&end, &old, &entity->origin, 1.0 / 30.0);
        speed = sqrt(((double)entity->origin.y * entity->origin.y + (double)rf(z * z)) +
                     (double)entity->origin.x * entity->origin.x);
        voice = bh_field_u32(&entity->effect.sound);
        bh_sound_update(b->sound, voice, rf(speed * .02), .5f, &old, &entity->origin);
        memcpy(&cached, &entity->effect.world, 4);
        memcpy(&ignore, &entity->effect.ignore, 4);
        self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
        self.object.actor = ignore;
        memset(&hit, 0, sizeof(hit));
        result = bh_world_segment_run(b->segment, ignore ? &self : NULL, &cached, &old, &end, -1, &hit);
        memcpy(&entity->effect.world, &cached, 4);
        if (result < 0)
        {
            a->error = 2;
            return;
        }
        world = result && hit.object.kind == BH_WORLD_HIT_GEOMETRY;
        if (world && (hit.material_flags & 4))
        {
            offset(&p, &hit.point, &hit.normal, .5);
            impact = bh_contact_impact_create(b->contacts, &p, &hit.normal, 4);
            if (impact)
            {
                state = bh_contact_impact_state(b->contacts, impact);
                state->age = 0;
                state->duration = 1.5f;
                state->size = 15;
                bh_sound_request(b->sound, b->liquid_sound, 2.5f, .6666667f, &p, &zero);
            }
            ++a->liquid_hits;
            retire(ctx, entity);
            return;
        }
        suppress = bh_field_u32(&entity->effect.origin_tag);
        if (result && !suppress)
        {
            ++a->collisions;
            bh_field_store_f32(&entity->effect.lifetime, 3);
            n = hit.normal;
            receiver = response(b, &hit);
            if (world && !(hit.material_flags & 2))
            {
                if (((double)entity->origin.x * n.x + (double)entity->origin.y * n.y) + (double)entity->origin.z * n.z >
                    0)
                {
                    n.x = -n.x;
                    n.y = -n.y;
                    n.z = -n.z;
                }
                offset(&p, &hit.point, &n, .15);
                if (!receiver)
                {
                    a->error = 3;
                    return;
                }
                marker = bh_hit_marker_create(b->markers, &p, &n, 6, 0, receiver);
                bh_trail_create_pair(ctx->trails, ctx->trail_image, 3, 0, &p, marker, 15, 5, 6, .5f, .3f, .2f, &drift,
                                     .75f, NULL, NULL);
            }
            v = entity->origin;
            bh_vec3_normalize(&v);
            offset(&entity->position, &hit.point, &v, -1);
            entity->origin = zero;
            memcpy(&entity->acceleration, &zero, 12);
            bh_sound_stop(b->sound, voice);
            voice = BH_SOUND_INVALID;
            memcpy(&entity->effect.sound, &voice, 4);
            if (ctx->network)
                bh_effect_network_emit(ctx->network, entity, 0x138, 0x134, 2, &zero);
            bh_effect_detonate(ctx, entity);
            return;
        }
        entity->position = end;
        return;
    }
    next = (double)bh_field_f32(&entity->effect.lifetime) - 1.0 / 30.0;
    bh_field_store_f32(&entity->effect.lifetime, rf(next));
    if (next < 2.0 && next >= 1.9666666666666666)
    {
        voice = bh_sound_request(b->sound, a->interval_sound, 1, 1, &entity->position, &zero);
        memcpy(&entity->effect.sound, &voice, 4);
    }
    if (!(bh_field_f32(&entity->effect.lifetime) < 0))
        return;
    bh_sound_stop(b->sound, bh_field_u32(&entity->effect.sound));
    bh_sound_request(b->sound, a->expiry_sound, 1, 1, &entity->position, &zero);
    ctx->debris->origin_tag = bh_field_u32(&entity->effect.origin_tag);
    memcpy(&owner, &entity->effect.source, 4);
    bh_script_explosion_create(ctx->explosion, &entity->position, 1.2f, 30, 1, (unsigned long)owner);
    puff(ctx, entity, a->ring_image, 1, 0, 800, 1);
    puff(ctx, entity, a->second_ring_image, 1, .1f, 400, 1);
    puff(ctx, entity, a->random_image, 1, .05f, 100, 0);
    puff(ctx, entity, a->ring_image, .25f, .4f, 200, 1);
    puff(ctx, entity, a->random_image, .5f, .5f, 50, 0);
    if (!bh_field_u32(&entity->effect.origin_tag))
    {
        damage.event = 0;
        damage.amount = 1;
        damage.source = owner;
        bh_entities_query(b->pool, 0, 0, &entity->position, 100, &damage);
    }
    bh_entity_release(b->pool, entity);
}
