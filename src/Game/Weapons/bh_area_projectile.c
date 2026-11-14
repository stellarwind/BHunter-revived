#include "../Entities/bh_field_access.h"
#include "bh_area_projectile.h"
#include "../../Core/bh_callback_helpers.h"
#include "../../Physics/bh_physics_response.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static BHAreaProjectileContext *context(const BHEntity *entity)
{
    BHAreaProjectileContext *ctx;
    memcpy(&ctx, &entity->bindings.area_projectile, 4);
    return ctx;
}
static int live(BHAreaProjectileContext *ctx, BHEntity *entity, unsigned long gen)
{
    return bh_entity_is_active(ctx->effects->pool, entity) && entity->generation == gen;
}
static void draw(BHEntity *entity, void *packet)
{
    BHAreaProjectileContext *ctx = context(entity);
    (void)packet;
    bh_projectile_pulse_draw(ctx->draw, entity);
}
static BHEntity *receiver(BHAreaProjectileContext *ctx, const BHWorldSegmentHit *hit)
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
static int packets(BHAreaProjectileContext *ctx, BHEntity *entity, BHEntity *target, const BHVec3 *point, float impulse,
                   double damage)
{
    BHPhysicsContactPacket packet;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } direct;
    unsigned long generation, target_generation, suppress;
    if (!target || !target->query)
    {
        ctx->error = 2;
        return 0;
    }
    generation = entity->generation;
    target_generation = target->generation;
    memset(&packet, 0, sizeof(packet));
    packet.event = 2;
    packet.point = *point;
    bh_vec3_normalize_to(&entity->origin, &packet.normal);
    packet.impulse = impulse;
    packet.severity = 0;
    target->query(target, &packet);
    if (!live(ctx, entity, generation))
    {
        ctx->error = 3;
        return 0;
    }
    memcpy(&suppress, &entity->area_projectile.identity, 4);
    if (!suppress)
    {
        if (!live(ctx, target, target_generation) || !target->query)
        {
            ctx->error = 3;
            return 0;
        }
        direct.event = 0;
        direct.amount = stored((double)bh_field_f32(&entity->area_projectile.strength) * damage);
        memcpy(&direct.source, &entity->area_projectile.source, 4);
        target->query(target, &direct);
        if (!live(ctx, entity, generation))
        {
            ctx->error = 3;
            return 0;
        }
    }
    return 1;
}
void bh_area_projectile_tick(BHEntity *entity)
{
    BHAreaProjectileContext *ctx = context(entity);
    BHHomingProjectileContext *b;
    BHWorldGeometry *cached;
    BHEntity *ignore, *actor, *target;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    BHEntityPositionPacket sample;
    BHVec3 end, delta, point;
    float power, radius;
    double range, dot, raw_radius, raw_power;
    unsigned long sound, i, n, generation;
    int result;
    if (!ctx || !bh_math_environment_supported())
        return;
    b = ctx->effects;
    ctx->error = 0;
    generation = entity->generation;
    raw_radius = (double)bh_field_f32(&entity->area_projectile.radius) + 2.0 / 3.0;
    raw_power = (double)bh_field_f32(&entity->area_projectile.strength) * .99;
    radius = stored(raw_radius);
    power = stored(raw_power);
    memcpy(&entity->area_projectile.radius, &radius, 4);
    memcpy(&entity->area_projectile.strength, &power, 4);
    if (raw_radius > 25)
    {
        ignore = NULL;
        memcpy(&entity->area_projectile.ignore, &ignore, 4);
    }
    memcpy(&sound, &entity->area_projectile.sound, 4);
    if (raw_power < .1)
    {
        bh_sound_stop(b->sound, sound);
        bh_entity_release(b->pool, entity);
        return;
    }
    end.x = stored((double)entity->position.x + (double)entity->origin.x / 30);
    end.y = stored((double)entity->position.y + (double)entity->origin.y / 30);
    end.z = stored((double)entity->position.z + (double)entity->origin.z / 30);
    memcpy(&cached, &entity->area_projectile.world, 4);
    memcpy(&ignore, &entity->area_projectile.ignore, 4);
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = ignore;
    result = bh_world_segment_run(b->segment, ignore ? &self : NULL, &cached, &entity->position, &end, -1, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    memcpy(&entity->area_projectile.world, &cached, 4);
    if (!result)
        entity->position = end;
    else
    {
        dot = (((double)entity->origin.y * hit.normal.y + (double)entity->origin.z * hit.normal.z) +
               (double)entity->origin.x * hit.normal.x) *
              (-2);
        power = stored((double)power * .9);
        memcpy(&entity->area_projectile.strength, &power, 4);
        entity->origin.x = stored((double)hit.normal.x * dot + entity->origin.x);
        entity->origin.y = stored((double)hit.normal.y * dot + entity->origin.y);
        entity->origin.z = stored((double)hit.normal.z * dot + entity->origin.z);
        entity->position = hit.point;
        target = receiver(ctx, &hit);
        if (!packets(ctx, entity, target, &entity->position, stored((double)power * (-.4)), .2))
            return;
        ++ctx->collisions;
    }
    range = stored((double)radius * radius * .2);
    if (cached)
    {
        BHWorldSegmentRoom *room = bh_world_segment_room(b->segment, cached);
        if (!room)
        {
            ctx->error = 2;
            return;
        }
        actor = room->actors_owner ? *room->actors_owner : NULL;
        n = 0;
        while (actor)
        {
            if (++n > b->segment->max_room_actors || !actor->query)
            {
                ctx->error = 2;
                return;
            }
            sample.event = 3;
            actor->query(actor, &sample);
            if (!live(ctx, entity, generation))
            {
                ctx->error = 3;
                return;
            }
            delta.x = stored((double)sample.position.x - entity->position.x);
            delta.y = stored((double)sample.position.y - entity->position.y);
            delta.z = stored((double)sample.position.z - entity->position.z);
            if (((double)delta.z * delta.z + (double)delta.y * delta.y) + (double)delta.x * delta.x < range)
            {
                power = bh_field_f32(&entity->area_projectile.strength);
                bh_sound_request(b->sound, ctx->hit_sound, power, 3, &entity->position, &zero);
                power = stored((double)power * .9);
                memcpy(&entity->area_projectile.strength, &power, 4);
                if (!packets(ctx, entity, actor, &entity->position, 16, .15))
                    return;
                ++ctx->room_hits;
            }
            actor = actor->actor.room_next;
        }
    }
    for (i = 0; i < b->segment->actor_count; i++)
    {
        actor = b->segment->actors[i].entity;
        memcpy(&ignore, &entity->area_projectile.ignore, 4);
        if (actor == ignore)
            continue;
        delta.x = stored((double)actor->position.x - entity->position.x);
        delta.y = stored((double)actor->position.y - entity->position.y);
        delta.z = stored((double)actor->position.z - entity->position.z);
        if (((double)delta.z * delta.z + (double)delta.y * delta.y) + (double)delta.x * delta.x < range)
        {
            power = bh_field_f32(&entity->area_projectile.strength);
            bh_sound_request(b->sound, ctx->hit_sound, stored((double)power + power), 3, &entity->position, &zero);
            power = stored((double)power * .9);
            memcpy(&entity->area_projectile.strength, &power, 4);
            delta.x = -delta.x;
            delta.y = -delta.y;
            delta.z = -delta.z;
            bh_vec3_normalize(&delta);
            point.x = stored((double)actor->position.x + delta.x);
            point.y = stored((double)actor->position.y + delta.y);
            point.z = stored((double)actor->position.z + delta.z);
            if (!packets(ctx, entity, actor, &point, stored((double)power * (-.4)), .2))
                return;
            ++ctx->global_hits;
        }
    }
    bh_sound_update(b->sound, sound, bh_field_f32(&entity->area_projectile.strength), 1, &entity->position,
                    &entity->origin);
}
BHEntity *bh_area_projectile_create(BHAreaProjectileContext *ctx, BHWorldGeometry *world, const BHVec3 *position,
                                    const BHVec3 *velocity, BHEntity *source, float strength)
{
    BHEntity *entity;
    float radius = 20;
    unsigned long sound, identity;
    BHHomingProjectileContext *b;
    if (!ctx || !position || !velocity || !ctx->identity || !ctx->effects || !ctx->draw || !ctx->draw->queue ||
        !bh_math_environment_supported())
        return NULL;
    b = ctx->effects;
    if (!b->pool || !b->sound || !b->segment || (b->response_count && !b->responses))
        return NULL;
    entity = bh_entity_acquire(b->pool);
    if (!entity)
        return NULL;
    identity = *ctx->identity;
    *ctx->identity = 0;
    memcpy(&entity->area_projectile.strength, &strength, 4);
    memcpy(&entity->area_projectile.radius, &radius, 4);
    memcpy(&entity->area_projectile.identity, &identity, 4);
    entity->position = *position;
    entity->origin.x = stored((double)velocity->x * 300);
    entity->origin.y = stored((double)velocity->y * 300);
    entity->origin.z = stored((double)velocity->z * 300);
    memcpy(&entity->bindings.area_projectile, &ctx, 4);
    entity->step = bh_area_projectile_tick;
    entity->refresh = draw;
    bh_entity_set_query(b->pool, entity, bh_pickup_target_removed);
    sound = bh_sound_request(b->sound, ctx->loop_sound, 1, 1, position, &entity->origin);
    memcpy(&entity->area_projectile.sound, &sound, 4);
    memcpy(&entity->area_projectile.ignore, &source, 4);
    memcpy(&entity->area_projectile.world, &world, 4);
    memcpy(&entity->area_projectile.source, &source, 4);
    return entity;
}
