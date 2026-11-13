#include "bh_field_access.h"
#include "bh_effect_orbit_tick.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static float rf(double n)
{
    float f;
    bh_round_float(&f, n);
    return f;
}
static int live(BHEffectOrbitTickContext *ctx, BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(ctx->core->effects->pool, entity) && entity->generation == generation;
}
int bh_effect_orbit_tick_bind(BHEffectOrbitTickContext *ctx, BHEntity *entity)
{
    if (!ctx || !ctx->core || !entity || !ctx->core->effects || !ctx->core->effects->segment ||
        !ctx->core->effects->contacts || !ctx->core->effects->markers || !ctx->core->trails || !ctx->core->explosion ||
        ctx->core->explosion->debris != ctx->core->debris)
        return 0;
    if (!bh_effect_projectile_bind(ctx->core, entity, 1))
        return 0;
    ctx->core->orbit = ctx;
    entity->step = bh_effect_orbit_tick;
    return 1;
}
static int announce(BHEffectOrbitTickContext *ctx, BHEntity *entity)
{
    return !ctx->core->network || bh_effect_network_emit(ctx->core->network, entity, 0x138, 0x134, 0, &zero);
}
static void explode(BHEffectOrbitTickContext *ctx, BHEntity *entity)
{
    BHEffectProjectileContext *b = ctx->core;
    b->debris->origin_tag = bh_field_u32(&entity->effect.origin_tag);
    bh_sound_stop(b->effects->sound, bh_field_u32(&entity->effect.sound));
    bh_script_explosion_create(b->explosion, &entity->position, 1.2f, 15, 1, bh_field_u32(&entity->effect.source));
}
static void direct(BHEntity *target, float amount, unsigned long source)
{
    struct
    {
        long event;
        float amount;
        unsigned long source;
    } packet;
    packet.event = 0;
    packet.amount = amount;
    packet.source = source;
    if (target && target->query)
        target->query(target, &packet);
}
static int motion(BHEffectOrbitTickContext *ctx, BHEntity *entity, BHEntity *target, unsigned long gen,
                  BHVec3 *position)
{
    BHEntityPositionPacket p;
    if (!target || !target->query)
    {
        ctx->error = 2;
        return 0;
    }
    p.event = 3;
    target->query(target, &p);
    if (!live(ctx, entity, gen))
    {
        ctx->error = 3;
        return 0;
    }
    *position = p.position;
    return 1;
}
static BHEntity *receiver(BHEffectOrbitTickContext *ctx, BHWorldSegmentHit *hit)
{
    BHHomingProjectileContext *b = ctx->core->effects;
    unsigned long i;
    if (hit->object.kind == BH_WORLD_HIT_ACTOR)
        return hit->object.object.actor;
    {
        const BHHomingWorldResponse *binding = bh_homing_world_response(b, hit->object.object.geometry);
        if (binding)
        {
            return binding->receiver;
        }
    }
    ctx->error = 2;
    return 0;
}
void bh_effect_orbit_tick(BHEntity *entity)
{
    BHEffectProjectileContext *core;
    BHEffectOrbitTickContext *ctx;
    BHHomingProjectileContext *b;
    BHEntity *target, *actor, *ignore, *contact, *response, *marker;
    BHWorldGeometry *cached;
    BHWorldSegmentRoom *room = 0;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHImpactState *impact;
    BHVec3 target_position, delta, velocity, acceleration, end, normal, point, drift = {0, -1, 0};
    unsigned long gen, identity, source, i, n;
    double grace, life, length, dot, power;
    int result, is_world;
    memcpy(&core, &entity->bindings.effect_projectile, 4);
    if (!core || !core->orbit)
        return;
    ctx = core->orbit;
    b = core->effects;
    memcpy(&gen, &entity->bindings.projectile_generation, 4);
    if (!live(ctx, entity, gen) || !bh_math_environment_supported())
    {
        ctx->error = 3;
        return;
    }
    ctx->error = 0;
    ++ctx->ticks;
    grace = (double)bh_field_f32(&entity->effect.grace) - 1.0 / 30;
    bh_field_store_f32(&entity->effect.grace, rf(grace));
    if (grace < 0)
    {
        ignore = 0;
        memcpy(&entity->effect.ignore, &ignore, 4);
    }
    life = (double)bh_field_f32(&entity->effect.lifetime) - 1.0 / 30;
    bh_field_store_f32(&entity->effect.lifetime, rf(life));
    identity = bh_field_u32(&entity->effect.origin_tag);
    source = bh_field_u32(&entity->effect.source);
    if (!(life > 0))
    {
        if (!announce(ctx, entity))
        {
            ctx->error = 2;
            return;
        }
        explode(ctx, entity);
        if (!live(ctx, entity, gen))
        {
            ctx->error = 3;
            return;
        }
        bh_entity_release(b->pool, entity);
        ++ctx->expired;
        return;
    }
    target = (BHEntity *)bh_field_u32(&entity->effect.target);
    memcpy(&velocity, &entity->effect.velocity, 12);
    memcpy(&acceleration, &entity->effect.acceleration, 12);
    if (target)
    {
        if (!motion(ctx, entity, target, gen, &target_position))
            return;
        for (i = 0; i < 3; ++i)
            ((float *)&delta)[i] = rf((double)((float *)&target_position)[i] - ((float *)&entity->position)[i]);
        length = sqrt(((double)delta.x * delta.x + (double)delta.y * delta.y) + (double)delta.z * delta.z);
        if (length < 3 && !identity)
        {
            if (!announce(ctx, entity))
            {
                ctx->error = 2;
                return;
            }
            explode(ctx, entity);
            if (!live(ctx, entity, gen))
            {
                ctx->error = 3;
                return;
            }
            direct(target, rf(1.2 - (double)rf(length) * .1), source);
            bh_entity_release(b->pool, entity);
            ++ctx->proximity;
            return;
        }
        power = .25 / rf(length);
        for (i = 0; i < 3; ++i)
        {
            ((float *)&delta)[i] = rf((double)((float *)&delta)[i] * power);
            ((float *)&velocity)[i] = rf((double)((float *)&velocity)[i] * .95);
            ((float *)&acceleration)[i] =
                rf(((double)((float *)&acceleration)[i] - ((float *)&delta)[i] * (-.25)) * .8);
        }
        memcpy(&entity->effect.velocity, &velocity, 12);
        memcpy(&entity->effect.acceleration, &acceleration, 12);
    }
    cached = (BHWorldGeometry *)bh_field_u32(&entity->effect.world);
    ignore = (BHEntity *)bh_field_u32(&entity->effect.ignore);
    if (cached && !identity)
    {
        room = bh_world_segment_room(b->segment, cached);
        if (!room)
        {
            ctx->error = 2;
            return;
        }
        for (actor = room->actors_owner ? *room->actors_owner : NULL, n = 0; actor;)
        {
            if (++n > b->segment->max_room_actors)
            {
                ctx->error = 2;
                return;
            }
            if (actor != ignore)
            {
                if (!motion(ctx, entity, actor, gen, &target_position))
                    return;
                for (i = 0; i < 3; ++i)
                    ((float *)&delta)[i] = rf((double)((float *)&target_position)[i] - ((float *)&entity->position)[i]);
                length = ((double)delta.x * delta.x + (double)delta.y * delta.y) + (double)delta.z * delta.z;
                if (length < 9)
                {
                    if (!announce(ctx, entity))
                    {
                        ctx->error = 2;
                        return;
                    }
                    explode(ctx, entity);
                    if (!live(ctx, entity, gen))
                    {
                        ctx->error = 3;
                        return;
                    }
                    bh_entity_release(b->pool, entity);
                    direct(actor, rf(1.2 - sqrt(length) * .1), source);
                    ++ctx->proximity;
                    return;
                }
            }
            actor = actor->actor.room_next;
        }
    }
    for (i = 0; i < 3; ++i)
    {
        ((float *)&velocity)[i] = rf((double)((float *)&acceleration)[i] + ((float *)&velocity)[i]);
        ((float *)&end)[i] = rf((double)((float *)&entity->position)[i] + ((float *)&velocity)[i]);
    }
    memcpy(&entity->effect.velocity, &velocity, 12);
    self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
    self.object.actor = ignore;
    result = bh_world_segment_run(b->segment, ignore ? &self : 0, &cached, &entity->position, &end, -1, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    memcpy(&entity->effect.world, &cached, 4);
    is_world = result && hit.object.kind == BH_WORLD_HIT_GEOMETRY;
    if (is_world && (hit.material_flags & 4))
    {
        for (i = 0; i < 3; ++i)
            ((float *)&point)[i] = rf((double)((float *)&hit.point)[i] - ((float *)&hit.normal)[i] * (-.5));
        contact = bh_contact_impact_create(b->contacts, &point, &hit.normal, 4);
        if (contact)
        {
            impact = bh_contact_impact_state(b->contacts, contact);
            impact->duration = 1.2f;
            impact->size = 12;
            bh_sound_request(b->sound, b->liquid_sound, rf((double)impact->size / 6), .8333333f, &point, &zero);
        }
        bh_sound_stop(b->sound, bh_field_u32(&entity->effect.sound));
        {
            struct
            {
                long event;
                BHEntity *actor;
            } p;
            p.event = 17;
            p.actor = entity;
            bh_entities_query(b->pool, 0, 0, 0, 0, &p);
        }
        if (!live(ctx, entity, gen))
        {
            ctx->error = 3;
            return;
        }
        bh_entity_release(b->pool, entity);
        ++ctx->liquid_hits;
        return;
    }
    if (!result || identity)
    {
        entity->position = end;
        for (i = 0; i < 3; ++i)
            ((float *)&velocity)[i] = rf((double)((float *)&velocity)[i] * 30);
        bh_sound_update(b->sound, bh_field_u32(&entity->effect.sound), 1, 1, &entity->position, &velocity);
        return;
    }
    response = receiver(ctx, &hit);
    if (ctx->error)
        return;
    normal = hit.normal;
    if (is_world && !(hit.material_flags & 2))
    {
        dot = ((double)entity->origin.z * normal.z + (double)entity->origin.x * normal.x) +
              (double)entity->origin.y * normal.y;
        if (dot > 0)
        {
            normal.x = -normal.x;
            normal.y = -normal.y;
            normal.z = -normal.z;
        }
        for (i = 0; i < 3; ++i)
            ((float *)&point)[i] = rf((double)((float *)&hit.point)[i] - ((float *)&normal)[i] * (-.15));
        if (!response)
        {
            ctx->error = 2;
            return;
        }
        marker = bh_hit_marker_create(b->markers, &point, &normal, 5, 1, response);
        bh_trail_create_pair(core->trails, core->trail_image, 3, 0, &point, marker, 10, 5, 6, .5f, .3f, .2f, &drift,
                             .5f, 0, 0);
    }
    entity->position = hit.point;
    if (!announce(ctx, entity))
    {
        ctx->error = 2;
        return;
    }
    explode(ctx, entity);
    if (!live(ctx, entity, gen))
    {
        ctx->error = 3;
        return;
    }
    direct(response, 1, source);
    bh_entity_release(b->pool, entity);
    ++ctx->world_hits;
}
