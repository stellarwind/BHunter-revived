#include "bh_ballistic_projectile.h"
#include "bh_physics_response.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static BHHomingProjectileContext *context(BHEntity *entity)
{
    BHHomingProjectileContext *ctx;
    memcpy(&ctx, &entity->bindings.homing_projectile, 4);
    return ctx;
}
static int live(BHHomingProjectileContext *ctx, BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(ctx->pool, entity) && entity->generation == generation;
}
static void offset(BHVec3 *out, const BHVec3 *p, const BHVec3 *n, double factor)
{
    bh_round_float(&out->x, (double)p->x - (double)n->x * factor);
    bh_round_float(&out->y, (double)p->y - (double)n->y * factor);
    bh_round_float(&out->z, (double)p->z - (double)n->z * factor);
}
static double length2(const BHVec3 *v)
{
    return ((double)v->x * v->x + (double)v->y * v->y) + (double)v->z * v->z;
}
int bh_ballistic_projectile_bind(BHBallisticProjectileContext *owner, BHEntity *entity)
{
    BHHomingProjectileContext *ctx = owner ? &owner->bindings : NULL;
    if (!ctx || !entity || !ctx->pool || !ctx->segment || !ctx->sound || !ctx->particles || !ctx->contacts ||
        !ctx->flash || !ctx->markers || !live(ctx, entity, entity->generation))
        return 0;
    if (ctx->response_count > 65535 || (ctx->response_count && !ctx->responses))
        return 0;
    if (ctx->particles->pool != ctx->pool || ctx->contacts->pool != ctx->pool || ctx->flash->pool != ctx->pool ||
        ctx->markers->pool != ctx->pool)
        return 0;
    memcpy(&entity->bindings.homing_projectile, &ctx, 4);
    memcpy(&entity->bindings.projectile_generation, &entity->generation, 4);
    entity->step = bh_ballistic_projectile_tick;
    ctx->error = 0;
    return 1;
}
static int receiver(BHHomingProjectileContext *ctx, const BHWorldSegmentHit *h, BHEntity **out, int *world)
{
    unsigned long i;
    *world = h->object.kind == BH_WORLD_HIT_GEOMETRY;
    if (!*world)
    {
        *out = h->object.object.actor;
        return *out != NULL;
    }
    {
        const BHHomingWorldResponse *binding = bh_homing_world_response(ctx, h->object.object.geometry);
        if (binding)
        {
            *out = binding->receiver;
            return 1;
        }
    }
    ctx->error = 2;
    return 0;
}
static void particles(BHHomingProjectileContext *ctx, const BHVec3 *p)
{
    float spin, angle;
    int r;
    r = rand();
    bh_round_float(&spin, ((double)r - 16384.0) * (1.0 / 8192.0));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(ctx->particles, ctx->first_image, 3, p, NULL, NULL, .4f, angle, 10, spin, .8f, 1, .75f,
                              1);
    r = rand();
    bh_round_float(&spin, ((double)r - 16384.0) * (1.0 / 8192.0));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(ctx->particles, ctx->first_image, 3, p, NULL, NULL, .2f, angle, 16, spin, .8f, 1, .65f,
                              1);
    bh_script_particle_create(ctx->particles, ctx->second_image, 3, p, NULL, NULL, .2f, 0, 24, 0, .8f, 1, .5f, 1);
}
static void flash(BHHomingProjectileContext *ctx, const BHVec3 *p)
{
    BHEntity *entity = bh_entity_acquire(ctx->pool);
    if (entity)
    {
        entity->position = *p;
        bh_flash_flare_bind(ctx->flash, entity, .1f, 50, 2);
    }
}
static void hit_packets(BHHomingProjectileContext *ctx, BHEntity *target, const BHVec3 *p, const BHVec3 *n,
                        float impulse, float damage, unsigned long suppress, BHEntity *source)
{
    BHPhysicsContactPacket packet;
    struct
    {
        long event;
        float amount;
        BHEntity *source;
    } direct;
    unsigned long generation;
    if (!target || !target->query)
        return;
    generation = target->generation;
    packet.event = 2;
    packet.point = *p;
    packet.normal = *n;
    packet.impulse = impulse;
    packet.severity = 0;
    target->query(target, &packet);
    if (!suppress)
    {
        if (!live(ctx, target, generation))
        {
            ctx->error = 3;
            return;
        }
        direct.event = 0;
        direct.amount = damage;
        direct.source = source;
        if (target->query)
        {
            target->query(target, &direct);
            ++ctx->damage_packets;
        }
    }
}
static void retire(BHHomingProjectileContext *ctx, BHEntity *entity, unsigned long voice)
{
    bh_sound_stop(ctx->sound, voice);
    bh_entity_release(ctx->pool, entity);
}
void bh_ballistic_projectile_tick(BHEntity *entity)
{
    BHHomingProjectileContext *ctx = context(entity);
    BHBallisticProjectileContext *owner = (BHBallisticProjectileContext *)ctx;
    BHEntity *ignore, *source, *response, *actor, *contact, *none = NULL;
    BHWorldGeometry *cached;
    BHWorldSegmentRoom *room = NULL;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHEntityPositionPacket position;
    BHVec3 old, velocity, end, delta, normal, point, effect;
    BHImpactState *impact;
    unsigned long generation, suppress, voice, i, n;
    float life, duration, stored_dot, incidence, impulse, damage;
    double remaining, dot, speed;
    int result, is_world, decision;
    if (!ctx || !bh_math_environment_supported())
        return;
    memcpy(&generation, &entity->bindings.projectile_generation, 4);
    if (!live(ctx, entity, generation))
    {
        ctx->error = 3;
        return;
    }
    ctx->error = 0;
    memcpy(&life, &entity->projectile.countdown, 4);
    memcpy(&voice, &entity->projectile.sound, 4);
    remaining = (double)life - (1.0 / 30.0);
    bh_round_float(&life, remaining);
    memcpy(&entity->projectile.countdown, &life, 4);
    if (!(remaining > 0))
    {
        retire(ctx, entity, voice);
        return;
    }
    old = entity->position;
    velocity = entity->origin;
    offset(&end, &old, &velocity, -1.0 / 30.0);
    memcpy(&cached, &entity->projectile.world, 4);
    memcpy(&ignore, &entity->projectile.ignore, 4);
    memcpy(&suppress, &entity->projectile.identity, 4);
    memcpy(&source, &entity->projectile.secondary, 4);
    self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
    self.object.actor = ignore;
    memset(&hit, 0, sizeof(hit));
    result = bh_world_segment_run(ctx->segment, ignore ? &self : NULL, &cached, &old, &end, -1, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    memcpy(&entity->projectile.world, &cached, 4);
    if (!result)
    {
        if (cached)
        {
            room = bh_world_segment_room(ctx->segment, cached);
            if (!room)
            {
                ctx->error = 2;
                return;
            }
            for (actor = room->actors_owner ? *room->actors_owner : NULL, n = 0; actor;)
            {
                if (++n > ctx->segment->max_room_actors)
                {
                    ctx->error = 2;
                    return;
                }
                if (actor != ignore)
                {
                    if (!actor->query)
                    {
                        ctx->error = 2;
                        return;
                    }
                    position.event = 3;
                    position.position = actor->position;
                    actor->query(actor, &position);
                    if (!live(ctx, entity, generation))
                    {
                        ctx->error = 3;
                        return;
                    }
                    bh_round_float(&delta.x, (double)position.position.x - old.x);
                    bh_round_float(&delta.y, (double)position.position.y - old.y);
                    bh_round_float(&delta.z, (double)position.position.z - old.z);
                    if (length2(&delta) < 9)
                    {
                        bh_sound_stop(ctx->sound, voice);
                        bh_sound_request(ctx->sound, ctx->hit_sound, 1, 1, &old, &zero);
                        particles(ctx, &old);
                        flash(ctx, &old);
                        bh_entity_release(ctx->pool, entity);
                        if (!bh_vec3_normalize_to(&delta, &normal))
                        {
                            ctx->error = 4;
                            return;
                        }
                        hit_packets(ctx, actor, &old, &normal, 8, .25f, suppress, source);
                        ++ctx->proximity;
                        return;
                    }
                }
                actor = actor->actor.room_next;
            }
        }
        entity->position = end;
        bh_sound_update(ctx->sound, voice, 1, 2, &entity->position, &entity->origin);
        return;
    }
    if (!receiver(ctx, &hit, &response, &is_world))
        return;
    ++ctx->world_hits;
    point = hit.point;
    normal = hit.normal;
    dot = ((double)velocity.x * normal.x + (double)velocity.y * normal.y) + (double)velocity.z * normal.z;
    bh_round_float(&stored_dot, dot);
    speed = sqrt(length2(&velocity));
    if (speed == 0)
    {
        ctx->error = 4;
        return;
    }
    bh_round_float(&incidence, dot / speed);
    if (incidence < 0 || _isnan(incidence))
        incidence = -incidence;
    if (is_world && (hit.material_flags & 4))
    {
        offset(&effect, &point, &normal, -.5);
        bh_round_float(&duration, .6 - (double)rand() * (-1.52587890625e-6));
        contact = bh_contact_impact_create(ctx->contacts, &effect, &normal, 4);
        if (contact)
        {
            impact = bh_contact_impact_state(ctx->contacts, contact);
            impact->duration = duration;
            impact->size = 6;
            bh_sound_request(ctx->sound, ctx->liquid_sound, 1, (float)(1.0 / duration), &effect, &zero);
        }
        retire(ctx, entity, voice);
        return;
    }
    decision = rand();
    if (_isnan(incidence) || (double)incidence < (double)decision * (1.0 / 24000.0))
    {
        memcpy(&entity->projectile.ignore, &none, 4);
        entity->position = point;
        bh_round_float(&entity->origin.x, (double)normal.x * ((double)stored_dot * (-2.0)) + velocity.x);
        bh_round_float(&entity->origin.y, (double)normal.y * ((double)stored_dot * (-2.0)) + velocity.y);
        bh_round_float(&entity->origin.z, (double)normal.z * ((double)stored_dot * (-2.0)) + velocity.z);
        bh_sound_request(ctx->sound, owner->ricochet_sound, .3f, 2, &point, &zero);
        for (i = 0; i < 2; ++i)
        {
            contact = bh_contact_impact_create(ctx->contacts, &point, &normal, 2);
            if (contact)
            {
                impact = bh_contact_impact_state(ctx->contacts, contact);
                impact->duration = .5f;
                impact->size = i ? 10.0f : 6.0f;
            }
        }
        for (i = 0; i < 3; ++i)
            bh_contact_spark_create(ctx->contacts, &point, 1, 1);
        bh_round_float(&entity->origin.x, (double)entity->origin.x * .75);
        bh_round_float(&entity->origin.y, (double)entity->origin.y * .75);
        bh_round_float(&entity->origin.z, (double)entity->origin.z * .75);
        bh_round_float(&life, (double)life * .75);
        memcpy(&entity->projectile.countdown, &life, 4);
        bh_round_float(&impulse, (double)incidence * (-.1));
        bh_round_float(&damage, (double)incidence * .25);
        hit_packets(ctx, response, &point, &normal, impulse, damage, suppress, source);
        ++owner->bounces;
        if (!live(ctx, entity, generation))
        {
            ctx->error = 3;
            return;
        }
        bh_sound_update(ctx->sound, voice, 1, 2, &entity->position, &entity->origin);
        return;
    }
    if (is_world && !(hit.material_flags & 2))
    {
        if (dot > 0)
        {
            normal.x = -normal.x;
            normal.y = -normal.y;
            normal.z = -normal.z;
        }
        offset(&effect, &point, &normal, -.1);
        if (response)
            bh_hit_marker_create(ctx->markers, &effect, &normal, .7f, 0, response);
        else
        {
            ctx->error = 2;
            return;
        }
    }
    entity->position = point;
    offset(&effect, &point, &normal, -.5);
    bh_sound_stop(ctx->sound, voice);
    bh_sound_request(ctx->sound, ctx->hit_sound, 1, 1, &point, &zero);
    particles(ctx, &effect);
    flash(ctx, &effect);
    bh_entity_release(ctx->pool, entity);
    if (!bh_vec3_normalize_to(&velocity, &normal))
    {
        ctx->error = 4;
        return;
    }
    hit_packets(ctx, response, &point, &normal, -.1f, .3f, suppress, source);
}
