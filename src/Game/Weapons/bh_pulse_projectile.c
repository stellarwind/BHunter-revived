#include "../Entities/bh_field_access.h"
#include "bh_pulse_projectile.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0}, acceleration = {0, -.002f, 0};
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static BHPulseProjectileContext *context(BHEntity *entity)
{
    BHPulseProjectileContext *ctx;
    memcpy(&ctx, &entity->bindings.pulse_projectile, 4);
    return ctx;
}
static int live(BHPulseProjectileContext *ctx, BHEntity *entity, unsigned long gen)
{
    return bh_entity_is_active(ctx->effects->pool, entity) && entity->generation == gen;
}
static void draw(BHEntity *entity, void *unused)
{
    (void)unused;
    bh_pulse_visual_draw(context(entity)->draw, entity);
}
static void announce(BHPulseProjectileContext *ctx, BHEntity *entity)
{
    unsigned long sync, i;
    long found;
    unsigned char payload[2];
    BHWorldGeometry *world;
    BHPacketWriter writer;
    memcpy(&sync, &entity->projectile.sync_identity, 4);
    if (ctx->peers <= 1 || !sync)
        return;
    memcpy(&world, &entity->projectile.world, 4);
    if (!world)
        return;
    found = ctx->effects->response_index ? bh_binding_find(ctx->effects->response_index, world, NULL) : -1;
    if (found >= 0 && (unsigned long)found < ctx->world_count && ctx->world_indices[found].world == world)
        i = (unsigned long)found;
    else
        for (i = 0; i < ctx->world_count; i++)
            if (ctx->world_indices[i].world == world)
                break;
    if (i == ctx->world_count)
    {
        ctx->error = 2;
        return;
    }
    if (!bh_packet_begin(&writer, ctx->packet, sizeof(ctx->packet), 9, (unsigned short)sync) ||
        !bh_packet_write_position(&writer, world, ctx->world_indices[i].index, &entity->position))
    {
        ctx->error = 2;
        return;
    }
    payload[0] = (unsigned char)(sync >> 16);
    payload[1] = 3;
    if (!bh_packet_append(&writer, payload, 2) || !bh_packet_write_vector(&writer, &zero, 1) ||
        !bh_packet_finish(&writer, &ctx->packet_bytes))
    {
        ctx->error = 2;
        return;
    }
    bh_network_send(ctx->transport, 0, ctx->packet, ctx->packet_bytes, 0);
    ++ctx->packets;
}
void bh_pulse_projectile_tick(BHEntity *entity)
{
    BHPulseProjectileContext *ctx = context(entity);
    BHHomingProjectileContext *b;
    float life, phase, spin, angle, drift;
    BHVec3 velocity, base, random_velocity, end, point;
    BHWorldGeometry *cached;
    BHEntity *ignore, *source, *contact;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    BHImpactState *impact;
    unsigned long identity, sound, generation, i;
    double dot, power, offset, raw_life;
    int r, result;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } damage;
    if (!ctx || !bh_math_environment_supported())
        return;
    b = ctx->effects;
    ctx->error = 0;
    generation = entity->generation;
    raw_life = (double)bh_field_f32(&entity->projectile.countdown) - 1.0 / 30;
    life = stored(raw_life);
    phase = stored((double)bh_field_f32(&entity->pulse.phase) + bh_field_f32(&entity->pulse.phase_step));
    memcpy(&entity->pulse.phase, &phase, 4);
    memcpy(&entity->projectile.countdown, &life, 4);
    if (raw_life < 3)
    {
        drift = stored((double)bh_field_f32(&entity->pulse.phase_step) * .92);
        memcpy(&entity->pulse.phase_step, &drift, 4);
    }
    if (raw_life < .2 && raw_life >= .16666666666666669)
    {
        announce(ctx, entity);
        if (ctx->error)
            return;
        bh_script_debris_create(ctx->debris, 30, &entity->position, 4, 50, 1);
        memcpy(&identity, &entity->projectile.identity, 4);
        *ctx->identity = identity;
        ctx->debris->origin_tag = identity;
        memcpy(&source, &entity->projectile.secondary, 4);
        bh_script_explosion_create(ctx->explosion, &entity->position, 1.5f, 27, 1, (unsigned long)source);
        *ctx->identity = ctx->debris->origin_tag;
        if (!live(ctx, entity, generation))
        {
            ctx->error = 3;
            return;
        }
        if (!identity)
        {
            damage.event = 0;
            damage.amount = 1;
            damage.source = source;
            bh_entities_query(b->pool, 0, 0, &entity->position, 30, &damage);
            if (!live(ctx, entity, generation))
            {
                ctx->error = 3;
                return;
            }
        }
        ++ctx->explosions;
    }
    for (i = 0; i < 3; i++)
    {
        r = rand();
        ((float *)&random_velocity)[i] = stored(((double)r - 16384) * .000002);
    }
    if (life > 3)
    {
        r = rand();
        spin = stored(((double)r - 16384) * (1.0 / 16384));
        r = rand();
        angle = stored((double)r * (1.0 / 512));
        bh_script_particle_create(b->particles, b->first_image, 3, &entity->position, &random_velocity, &acceleration,
                                  .2f, angle, 5, spin, .9f, .95f, .5f, .15f);
    }
    if (life <= 0)
    {
        bh_entity_release(b->pool, entity);
        return;
    }
    memcpy(&velocity, &entity->pulse.velocity, 12);
    memcpy(&base, &entity->pulse.base, 12);
    for (i = 0; i < 3; i++)
    {
        ((float *)&entity->origin)[i] = stored((double)((float *)&base)[i] + ((float *)&velocity)[i]);
        ((float *)&velocity)[i] = stored((double)((float *)&velocity)[i] * .95);
    }
    memcpy(&entity->pulse.velocity, &velocity, 12);
    power = pow((double)life, .25);
    offset = (1 - power) * cos((double)life * 3) * (-1.0 / 6);
    end.x = stored((double)entity->position.x + (double)entity->origin.x / 30);
    end.y = stored((double)entity->position.y + ((double)entity->origin.y / 30 - offset));
    end.z = stored((double)entity->position.z + (double)entity->origin.z / 30);
    memcpy(&cached, &entity->projectile.world, 4);
    memcpy(&ignore, &entity->projectile.ignore, 4);
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = ignore;
    result = bh_world_segment_run(b->segment, ignore ? &self : NULL, &cached, &entity->position, &end, -1, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    memcpy(&entity->projectile.world, &cached, 4);
    memcpy(&sound, &entity->projectile.sound, 4);
    if (!result)
        entity->position = end;
    else if (hit.object.kind == BH_WORLD_HIT_GEOMETRY && (hit.material_flags & 4))
    {
        point.x = stored((double)hit.point.x + (double)hit.normal.x * .5);
        point.y = stored((double)hit.point.y + (double)hit.normal.y * .5);
        point.z = stored((double)hit.point.z + (double)hit.normal.z * .5);
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
        ++ctx->liquid_hits;
        return;
    }
    else
    {
        memcpy(&velocity, &entity->pulse.velocity, 12);
        dot = (((double)velocity.x * hit.normal.x + (double)velocity.y * hit.normal.y) +
               (double)velocity.z * hit.normal.z) *
              (-2);
        for (i = 0; i < 3; i++)
            ((float *)&velocity)[i] = stored((double)((float *)&hit.normal)[i] * dot + ((float *)&velocity)[i]);
        memcpy(&entity->pulse.velocity, &velocity, 12);
        dot = (((double)base.x * hit.normal.x + (double)base.y * hit.normal.y) + (double)base.z * hit.normal.z) * (-2);
        for (i = 0; i < 3; i++)
            ((float *)&base)[i] = stored((double)((float *)&hit.normal)[i] * dot + ((float *)&base)[i]);
        ignore = NULL;
        memcpy(&entity->projectile.ignore, &ignore, 4);
        entity->position = hit.point;
        for (i = 0; i < 3; i++)
        {
            ((float *)&velocity)[i] = stored((double)((float *)&velocity)[i] * .9);
            ((float *)&base)[i] = stored((double)((float *)&base)[i] * .9);
        }
        memcpy(&entity->pulse.velocity, &velocity, 12);
        memcpy(&entity->pulse.base, &base, 12);
        bh_sound_request(b->sound, ctx->bounce_sound, 1, 1, &entity->position, &zero);
        bh_contact_spark_create(b->contacts, &entity->position, .75f, 3);
        bh_contact_spark_create(b->contacts, &entity->position, 1, 2);
        ++ctx->bounces;
    }
    bh_sound_update(b->sound, sound, 1, 1, &entity->position, &entity->origin);
}
BHEntity *bh_pulse_projectile_create(BHPulseProjectileContext *ctx, BHWorldGeometry *world, BHEntity *source,
                                     const BHVec3 *position, const BHVec3 *base, const BHVec3 *velocity)
{
    BHEntity *entity;
    BHProjectileQueryState state;
    float f;
    unsigned long sync, i;
    BHHomingProjectileContext *b;
    if (!ctx || !position || !base || !velocity || !ctx->effects || !ctx->lifetime || !ctx->draw || !ctx->draw->queue ||
        !ctx->identity || !ctx->debris || !ctx->explosion || !bh_math_environment_supported())
        return NULL;
    b = ctx->effects;
    if (!b->pool || !b->sound || !b->segment || !b->particles || !b->contacts || ctx->lifetime->pool != b->pool ||
        ctx->lifetime->sound != b->sound || ctx->debris->pool != b->pool || ctx->explosion->pool != b->pool ||
        ctx->explosion->debris != ctx->debris)
        return NULL;
    entity = bh_entity_acquire(b->pool);
    if (!entity)
        return NULL;
    bh_projectile_query_state(entity, &state);
    state.countdown = 4;
    state.identity = *ctx->identity;
    *ctx->identity = 0;
    state.secondary = source;
    entity->position = *position;
    f = stored((double)rand() * (1.0 / 5120));
    memcpy(&entity->pulse.phase, &f, 4);
    f = 1.0f / 30;
    memcpy(&entity->pulse.phase_step, &f, 4);
    memcpy(&entity->pulse.velocity, velocity, 12);
    memcpy(&entity->pulse.base, base, 12);
    for (i = 0; i < 3; i++)
        ((float *)&entity->origin)[i] = stored((double)((const float *)velocity)[i] + ((const float *)base)[i]);
    entity->origin = *velocity;
    memcpy(&entity->bindings.pulse_projectile, &ctx, 4);
    entity->step = bh_pulse_projectile_tick;
    entity->refresh = draw;
    bh_projectile_lifetime_bind(ctx->lifetime, entity, &state);
    state.sound = bh_sound_request(b->sound, ctx->loop_sound, 1, 1, position, &entity->origin);
    memcpy(&entity->projectile.sound, &state.sound, 4);
    memcpy(&entity->projectile.world, &world, 4);
    memcpy(&entity->projectile.ignore, &source, 4);
    sync = 0;
    if (ctx->sync_identity)
    {
        sync = *ctx->sync_identity;
        *ctx->sync_identity = 0;
    }
    memcpy(&entity->projectile.sync_identity, &sync, 4);
    if (state.identity)
    {
        f = stored((double)state.countdown - (-.3));
        memcpy(&entity->projectile.countdown, &f, 4);
    }
    return entity;
}
