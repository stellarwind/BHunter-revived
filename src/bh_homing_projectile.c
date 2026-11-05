#include "bh_homing_projectile.h"
#include "bh_physics_response.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static void get(const BHEntity *entity, unsigned long a, void *p, unsigned long n)
{
    memcpy(p, (const unsigned char *)entity + a, n);
}
static void put(BHEntity *entity, unsigned long a, const void *p, unsigned long n)
{
    memcpy((unsigned char *)entity + a, p, n);
}
static BHHomingProjectileContext *context(BHEntity *entity)
{
    BHHomingProjectileContext *ctx;
    memcpy(&ctx, entity->payload + 608, 4);
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
int __cdecl bh_homing_projectile_bind(BHHomingProjectileContext *ctx, BHEntity *entity)
{
    if (!ctx || !entity || !ctx->pool || !ctx->segment || !ctx->sound || !ctx->particles || !ctx->contacts ||
        !ctx->flash || !ctx->markers || !live(ctx, entity, entity->generation))
        return 0;
    if (ctx->response_count > 65535 || (ctx->response_count && !ctx->responses))
        return 0;
    if (ctx->particles->pool != ctx->pool || ctx->contacts->pool != ctx->pool || ctx->flash->pool != ctx->pool ||
        ctx->markers->pool != ctx->pool)
        return 0;
    memcpy(entity->payload + 608, &ctx, 4);
    memcpy(entity->payload + 612, &entity->generation, 4);
    entity->step = bh_homing_projectile_tick;
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
    bh_script_particle_create(ctx->particles, ctx->first_image, 3, p, NULL, NULL, 1.6f, angle, 20, spin, .8f, 1, .75f,
                              1);
    r = rand();
    bh_round_float(&spin, ((double)r - 16384.0) * (1.0 / 8192.0));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(ctx->particles, ctx->second_image, 3, p, NULL, NULL, .8f, angle, 32, spin, .8f, 1, .65f,
                              1);
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(ctx->particles, ctx->first_image, 3, p, NULL, NULL, .8f, angle, 48, 0, .8f, 1, .5f, 1);
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
void __cdecl bh_homing_projectile_tick(BHEntity *entity)
{
    BHHomingProjectileContext *ctx = context(entity);
    BHEntity *target, *ignore, *damage_source, *response, *actor;
    BHWorldGeometry *cached;
    BHWorldSegmentRoom *room = NULL;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    BHEntityPositionPacket position;
    BHVec3 old, delta, direction, velocity, end, normal, point, effect;
    BHImpactState *impact;
    BHEntity *contact;
    unsigned long generation, suppress, i, n;
    float life, previous[3], *d, *v, *p, duration;
    double remaining, factor, dot, dx, dy, dz;
    int result, is_world = 0;
    if (!ctx || !bh_math_environment_supported())
        return;
    memcpy(&generation, entity->payload + 612, 4);
    if (!live(ctx, entity, generation))
    {
        ctx->error = 3;
        return;
    }
    ctx->error = 0;
    get(entity, 0xe8, &life, 4);
    remaining = (double)life - (1.0 / 30.0);
    bh_round_float(&life, remaining);
    put(entity, 0xe8, &life, 4);
    if (!(remaining > 0))
    {
        bh_entity_release(ctx->pool, entity);
        return;
    }
    get(entity, 0x12c, &target, 4);
    if (target)
    {
        if (!target->query)
        {
            ctx->error = 2;
            return;
        }
        position.event = 3;
        position.position = target->position;
        target->query(target, &position);
        if (!live(ctx, entity, generation))
        {
            ctx->error = 3;
            return;
        }
        get(entity, 0x100, previous, 12);
        put(entity, 0x100, &position.position, 12);
        old = entity->position;
        velocity = entity->origin;
        dx = (double)position.position.x - old.x;
        dy = (double)position.position.y - old.y;
        dz = (double)position.position.z - old.z;
        bh_round_float(&delta.x, dx);
        bh_round_float(&delta.y, dy);
        bh_round_float(&delta.z, dz);
        /* 0043f6fd..0043f729 stores each delta with FST, retaining its
           unrounded value for multiplication by the binary32 copy. */
        factor = sqrt((dx * delta.x + dy * delta.y) + dz * delta.z) * (1.0 / 15.0);
        d = &delta.x;
        p = &position.position.x;
        for (i = 0; i < 3; ++i)
            bh_round_float(d + i, ((double)p[i] - previous[i]) * factor + d[i]);
        if (!bh_vec3_normalize_to(&velocity, &direction))
        {
            ctx->error = 4;
            return;
        }
        bh_vec3_normalize(&delta);
        dot = ((double)direction.x * delta.x + (double)direction.y * delta.y) + (double)direction.z * delta.z;
        if (dot > -.4)
        {
            v = &entity->origin.x;
            d = &delta.x;
            p = &direction.x;
            for (i = 0; i < 3; ++i)
                bh_round_float(v + i, (double)v[i] - ((double)d[i] - dot * p[i]) * (-15.0));
        }
        ++ctx->guidance;
    }
    velocity = entity->origin;
    old = entity->position;
    offset(&end, &old, &velocity, -1.0 / 30.0);
    get(entity, 0x124, &cached, 4);
    get(entity, 0x128, &ignore, 4);
    get(entity, 0x11c, &suppress, 4);
    get(entity, 0x130, &damage_source, 4);
    self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
    self.object.actor = ignore;
    memset(&hit, 0, sizeof(hit));
    result = bh_world_segment_run(ctx->segment, ignore ? &self : NULL, &cached, &old, &end, -1, &hit);
    if (result < 0)
    {
        ctx->error = 2;
        return;
    }
    put(entity, 0x124, &cached, 4);
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
                    dx = (double)position.position.x - old.x;
                    dy = (double)position.position.y - old.y;
                    dz = (double)position.position.z - old.z;
                    bh_round_float(&delta.x, dx);
                    bh_round_float(&delta.y, dy);
                    bh_round_float(&delta.z, dz);
                    /* The proximity predicate also retains each unrounded
                       delta for multiplication by its stored copy. */
                    if ((dx * delta.x + dy * delta.y) + dz * delta.z < 9)
                    {
                        bh_sound_request(ctx->sound, ctx->hit_sound, .8f, .75f, &old, &zero);
                        particles(ctx, &old);
                        flash(ctx, &old);
                        bh_entity_release(ctx->pool, entity);
                        if (!bh_vec3_normalize_to(&delta, &normal))
                        {
                            ctx->error = 4;
                            return;
                        }
                        hit_packets(ctx, actor, &old, &normal, 8, .2f, suppress, damage_source);
                        ++ctx->proximity;
                        return;
                    }
                }
                get(actor, 0xf4, &actor, 4);
            }
        }
        entity->position = end;
        return;
    }
    if (!receiver(ctx, &hit, &response, &is_world))
        return;
    normal = hit.normal;
    point = hit.point;
    ++ctx->world_hits;
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
        bh_entity_release(ctx->pool, entity);
        return;
    }
    if (is_world && !(hit.material_flags & 2))
    {
        /* 0043f9df..0043fa02 adds X/Z before Y. */
        dot = ((double)velocity.x * normal.x + (double)velocity.z * normal.z) + (double)velocity.y * normal.y;
        if (dot > 0)
        {
            normal.x = -normal.x;
            normal.y = -normal.y;
            normal.z = -normal.z;
        }
        offset(&effect, &point, &normal, -.1);
        if (response)
            bh_hit_marker_create(ctx->markers, &effect, &normal, 1.5f, 2, response);
        else
        {
            ctx->error = 2;
            return;
        }
    }
    entity->position = point;
    offset(&effect, &point, &normal, -.5);
    bh_sound_request(ctx->sound, ctx->hit_sound, .8f, .75f, &point, &zero);
    particles(ctx, &point);
    flash(ctx, &effect);
    bh_entity_release(ctx->pool, entity);
    if (!bh_vec3_normalize_to(&velocity, &normal))
    {
        ctx->error = 4;
        return;
    }
    hit_packets(ctx, response, &point, &normal, -.1f, is_world ? .25f : .08f, suppress, damage_source);
}
