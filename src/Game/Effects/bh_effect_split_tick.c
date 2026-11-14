#include "../Entities/bh_field_access.h"
#include "bh_effect_split_tick.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0}, acceleration = {0, -.002f, 0};
static float stored(double x)
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
static int live(BHEffectProjectileContext *ctx, BHEntity *entity, unsigned long gen)
{
    return bh_entity_is_active(ctx->effects->pool, entity) && entity->generation == gen;
}
static int random_value(BHEffectProjectileContext *ctx)
{
    ++ctx->random_calls;
    return rand();
}
static BHEntity *receiver(BHEffectProjectileContext *ctx, const BHWorldSegmentHit *hit)
{
    unsigned long i;
    if (hit->object.kind != BH_WORLD_HIT_GEOMETRY)
        return hit->object.object.actor;
    {
        const BHHomingWorldResponse *binding = bh_homing_world_response(ctx->effects, hit->object.object.geometry);
        if (binding)
        {
            return binding->receiver;
        }
    }
    return NULL;
}
static int packet(BHEffectProjectileContext *ctx, BHEntity *entity, unsigned char kind, const BHVec3 *direction)
{
    if (ctx->network && !bh_effect_network_emit(ctx->network, entity, 0x138, 0x134, kind, direction))
    {
        ctx->error = 2;
        return 0;
    }
    return 1;
}
static void flash(BHEffectProjectileContext *ctx, const BHVec3 *p)
{
    BHEntity *entity = bh_entity_acquire(ctx->effects->pool);
    if (entity)
    {
        entity->position = *p;
        bh_flash_flare_bind(ctx->effects->flash, entity, .1f, 100, 4);
        ++ctx->flashes;
    }
}
int bh_effect_split_cascade(BHEntity *entity, const BHVec3 *normal)
{
    BHEffectProjectileContext *ctx = context(entity);
    BHEffectSplitContext *s;
    BHWorldGeometry *world;
    BHEntity *source;
    BHVec3 velocity, old;
    long count, remaining, groups, extra, children[32], i, j;
    unsigned long identity, generation;
    float lifetime, gain, normal_gain;
    double coefficient;
    if (!ctx || !ctx->split || !normal || !bh_math_environment_supported())
        return 0;
    s = ctx->split;
    count = (long)bh_field_u32(&entity->effect.split_count);
    if (count < 1 || count > 33)
    {
        s->error = 1;
        ctx->error = 1;
        return 0;
    }
    generation = entity->generation;
    identity = bh_field_u32(&entity->effect.origin_tag);
    source = (BHEntity *)bh_field_u32(&entity->effect.source);
    world = (BHWorldGeometry *)bh_field_u32(&entity->effect.world);
    bh_sound_stop(ctx->effects->sound, bh_field_u32(&entity->effect.sound));
    if (count >= 9)
    {
        ctx->debris->origin_tag = identity;
        bh_script_explosion_create(ctx->explosion, &entity->position, 1, 10, 1, (unsigned long)source);
    }
    else if (count >= 4)
    {
        ctx->debris->origin_tag = identity;
        bh_script_explosion_secondary(ctx->explosion, &entity->position, 1, 4, 1, s->split_sound,
                                      (unsigned long)source);
    }
    else if (count > 1)
        bh_sound_request(ctx->effects->sound, s->split_sound, 1, 1.2f, &entity->position, &zero);
    if (!live(ctx, entity, generation))
    {
        ctx->error = 3;
        return 0;
    }
    remaining = count - 1;
    bh_field_store_u32(&entity->effect.split_count, (unsigned long)remaining);
    if (remaining)
    {
        groups = remaining;
        if (remaining > 2)
            groups = (long)((unsigned long)random_value(ctx) * (remaining - 1) / 32768) + 2;
        for (i = 0; i < groups; i++)
            children[i] = 1;
        extra = remaining - groups;
        bh_field_store_u32(&entity->effect.split_count, (unsigned long)(extra - 1));
        for (i = 0; i < extra; i++)
        {
            j = (long)((unsigned long)random_value(ctx) * groups / 32768);
            ++children[j];
            bh_field_store_u32(&entity->effect.split_count, (unsigned long)(extra - i - 2));
        }
        gain = (float)(remaining * 5);
        memcpy(&old, &entity->effect.velocity, 12);
        for (i = 0; i < groups; i++)
        {
            for (j = 0; j < 3; j++)
                ((float *)&velocity)[j] = (float)(random_value(ctx) - 16384);
            bh_vec3_normalize(&velocity);
            for (j = 0; j < 3; j++)
                ((float *)&velocity)[j] =
                    stored((double)((float *)&velocity)[j] * gain - (double)((float *)&old)[j] * (-.5));
            normal_gain = stored((double)random_value(ctx) * (1.0 / 512));
            for (j = 0; j < 3; j++)
                ((float *)&velocity)[j] =
                    stored((double)((const float *)normal)[j] * normal_gain + ((float *)&velocity)[j]);
            coefficient = children[i] > 1 ? 1.1111111111111112e-5 : 1.0 / 32768;
            lifetime = stored((double)random_value(ctx) * coefficient);
            ctx->debris->origin_tag = identity;
            bh_effect_shard_create(ctx, world, source, &entity->position, &velocity, lifetime, children[i], 0, .9f);
        }
    }
    bh_entity_release(ctx->effects->pool, entity);
    ++s->cascades;
    return 1;
}
void bh_effect_split_tick(BHEntity *entity)
{
    BHEffectProjectileContext *ctx = context(entity);
    BHHomingProjectileContext *b;
    BHEffectSplitContext *s;
    BHVec3 movement, end, random_velocity, normal, point, drift, normalized;
    BHWorldGeometry *cached;
    BHEntity *ignore, *response, *contact, *marker;
    BHImpactState *impact;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    unsigned long identity, generation, sound;
    float speed, lifetime, spin, angle;
    double remaining, unignore, dot;
    int i, r, result;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } damage;
    if (!ctx || !ctx->split || !bh_math_environment_supported())
        return;
    b = ctx->effects;
    s = ctx->split;
    ctx->error = 0;
    s->error = 0;
    generation = entity->generation;
    unignore = (double)bh_field_f32(&entity->effect.grace) - 1.0 / 30;
    bh_field_store_f32(&entity->effect.grace, stored(unignore));
    if (unignore < 0)
        bh_field_store_u32(&entity->effect.ignore, 0);
    remaining = (double)bh_field_f32(&entity->effect.lifetime) - 1.0 / 30;
    bh_field_store_f32(&entity->effect.lifetime, stored(remaining));
    if (remaining <= 0)
    {
        if (!packet(ctx, entity, 1, &zero))
            return;
        if (bh_field_u32(&entity->effect.origin_tag))
            bh_field_store_u32(&entity->effect.origin_tag, 1);
        bh_effect_split_cascade(entity, &zero);
        return;
    }
    memcpy(&movement, &entity->effect.velocity, 12);
    for (i = 0; i < 3; i++)
    {
        ((float *)&movement)[i] = stored((double)((float *)&movement)[i] * bh_field_f32(&entity->effect.multiplier));
        ((float *)&end)[i] = stored((double)((float *)&entity->position)[i] + ((float *)&movement)[i]);
    }
    memcpy(&entity->effect.velocity, &movement, 12);
    r = random_value(ctx);
    speed = stored((double)r * (1.0 / 32768));
    r = random_value(ctx);
    lifetime = stored((double)r * (1.0 / 65536));
    bh_contact_spark_create(b->contacts, &entity->position, lifetime, speed);
    for (i = 0; i < 3; i++)
        ((float *)&random_velocity)[i] = stored((double)(random_value(ctx) - 16384) * .000002);
    r = random_value(ctx);
    spin = stored((double)(r - 16384) * (1.0 / 16384));
    r = random_value(ctx);
    angle = stored((double)r * (1.0 / 512));
    bh_script_particle_create(b->particles, b->first_image, 3, &entity->position, &random_velocity, &acceleration, .2f,
                              angle, 6, spin, .9f, .95f, .6f, .4f);
    ++ctx->particles;
    cached = (BHWorldGeometry *)bh_field_u32(&entity->effect.world);
    ignore = (BHEntity *)bh_field_u32(&entity->effect.ignore);
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = ignore;
    result = bh_world_segment_run(b->segment, ignore ? &self : NULL, &cached, &entity->position, &end, 4, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    memcpy(&entity->effect.world, &cached, 4);
    sound = bh_field_u32(&entity->effect.sound);
    if (result && hit.object.kind == BH_WORLD_HIT_GEOMETRY && (hit.material_flags & 4))
    {
        for (i = 0; i < 3; i++)
            ((float *)&point)[i] = stored((double)((float *)&hit.point)[i] - (double)((float *)&hit.normal)[i] * (-.5));
        contact = bh_contact_impact_create(b->contacts, &point, &hit.normal, 4);
        if (contact)
        {
            impact = bh_contact_impact_state(b->contacts, contact);
            impact->duration = 1;
            impact->size = 10;
            bh_sound_request(b->sound, b->liquid_sound, stored(10.0 / 6), 1, &point, &zero);
        }
        bh_sound_stop(b->sound, sound);
        bh_entity_release(b->pool, entity);
        ++s->liquid_hits;
        return;
    }
    identity = bh_field_u32(&entity->effect.origin_tag);
    if (result && identity != 1)
    {
        normal = hit.normal;
        response = receiver(ctx, &hit);
        if (hit.object.kind == BH_WORLD_HIT_GEOMETRY && !(hit.material_flags & 2))
        {
            dot = ((double)entity->origin.z * normal.z + (double)entity->origin.y * normal.y) +
                  (double)entity->origin.x * normal.x;
            if (dot > 0)
            {
                normal.x = -normal.x;
                normal.y = -normal.y;
                normal.z = -normal.z;
            }
            for (i = 0; i < 3; i++)
                ((float *)&point)[i] =
                    stored((double)((float *)&hit.point)[i] - (double)((float *)&normal)[i] * (-.15));
            if (!response)
            {
                ctx->error = 2;
                return;
            }
            marker = bh_hit_marker_create(b->markers, &point, &normal, 3, 1, response);
            drift.x = 0;
            drift.y = -1;
            drift.z = 0;
            bh_trail_create_pair(ctx->trails, ctx->trail_image, 3, 0, &point, marker, 6, 5, 6, .5f, .3f, .2f, &drift,
                                 .5f, NULL, NULL);
        }
        entity->position = hit.point;
        if (!packet(ctx, entity, 1, &zero))
            return;
        if (!identity)
        {
            if (!response)
            {
                ctx->error = 2;
                return;
            }
            damage.event = 0;
            damage.amount = 1;
            damage.source = (BHEntity *)bh_field_u32(&entity->effect.source);
            /* Original00404eb2 calls even the RET sentinel, then00404ec1 always
     cascades. NULL is the source representation of that no-op callback;
     it must not strand the projectile on the wall after emitting trails. */
            if (response->query)
                response->query(response, &damage);
            if (!live(ctx, entity, generation))
            {
                ctx->error = 3;
                return;
            }
        }
        ++s->collisions;
        bh_effect_split_cascade(entity, &normal);
        return;
    }
    entity->position = end;
    for (i = 0; i < 3; i++)
        ((float *)&drift)[i] = stored((double)((float *)&movement)[i] * 30);
    bh_sound_update(b->sound, sound, 1, 2, &entity->position, &drift);
    bh_field_store_f32(&entity->effect.velocity.y,
                       stored((double)bh_field_f32(&entity->effect.velocity.y) - (-.001091111111111111)));
    if (!identity && (long)bh_field_u32(&entity->effect.split_count) > 1 && random_value(ctx) < 1024)
    {
        for (i = 0; i < 3; i++)
            ((float *)&random_velocity)[i] = stored((double)(random_value(ctx) - 16384) * (1.0 / 262144));
        flash(ctx, &entity->position);
        bh_sound_request(b->sound, ctx->ricochet_sound, 1, 2, &entity->position, &drift);
        memcpy(&movement, &entity->effect.velocity, 12);
        for (i = 0; i < 3; i++)
            ((float *)&movement)[i] =
                stored((double)((float *)&movement)[i] - (double)(random_value(ctx) - 16384) * (-1.0 / 131072));
        memcpy(&entity->effect.velocity, &movement, 12);
        r = random_value(ctx);
        spin = stored((double)(r - 16384) * (1.0 / 16384));
        r = random_value(ctx);
        angle = stored((double)r * (1.0 / 512));
        bh_script_particle_create(b->particles, ctx->smoke_image, 3, &entity->position, &random_velocity, &acceleration,
                                  .1f, angle, 4, spin, .9f, .95f, .5f, 1);
        ++ctx->particles;
        bh_vec3_normalize_to(&movement, &normalized);
        if (!packet(ctx, entity, 4, &normalized))
            return;
        ++s->jitters;
    }
}
