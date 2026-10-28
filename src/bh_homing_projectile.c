#include "bh_homing_projectile.h"
#include "bh_physics_response.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static void get(const BHEntity *e, unsigned long a, void *p, unsigned long n)
{
    memcpy(p, (const unsigned char *)e + a, n);
}
static void put(BHEntity *e, unsigned long a, const void *p, unsigned long n)
{
    memcpy((unsigned char *)e + a, p, n);
}
static BHHomingProjectileContext *context(BHEntity *e)
{
    BHHomingProjectileContext *c;
    memcpy(&c, e->payload + 608, 4);
    return c;
}
static int live(BHHomingProjectileContext *c, BHEntity *e, unsigned long generation)
{
    return bh_entity_is_active(c->pool, e) && e->generation == generation;
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
int __cdecl bh_homing_projectile_bind(BHHomingProjectileContext *c, BHEntity *e)
{
    if (!c || !e || !c->pool || !c->segment || !c->sound || !c->particles || !c->contacts || !c->flash || !c->markers ||
        !live(c, e, e->generation))
        return 0;
    if (c->response_count > 65535 || (c->response_count && !c->responses))
        return 0;
    if (c->particles->pool != c->pool || c->contacts->pool != c->pool || c->flash->pool != c->pool ||
        c->markers->pool != c->pool)
        return 0;
    memcpy(e->payload + 608, &c, 4);
    memcpy(e->payload + 612, &e->generation, 4);
    e->step = bh_homing_projectile_tick;
    c->error = 0;
    return 1;
}
static int receiver(BHHomingProjectileContext *c, const BHWorldSegmentHit *h, BHEntity **out, int *world)
{
    unsigned long i;
    *world = h->object.kind == BH_WORLD_HIT_GEOMETRY;
    if (!*world)
    {
        *out = h->object.object.actor;
        return *out != NULL;
    }
    for (i = 0; i < c->response_count; ++i)
        if (c->responses[i].world == h->object.object.geometry)
        {
            *out = c->responses[i].receiver;
            return 1;
        }
    c->error = 2;
    return 0;
}
static void particles(BHHomingProjectileContext *c, const BHVec3 *p)
{
    float spin, angle;
    int r;
    r = rand();
    bh_round_float(&spin, ((double)r - 16384.0) * (1.0 / 8192.0));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(c->particles, c->first_image, 3, p, NULL, NULL, 1.6f, angle, 20, spin, .8f, 1, .75f, 1);
    r = rand();
    bh_round_float(&spin, ((double)r - 16384.0) * (1.0 / 8192.0));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(c->particles, c->second_image, 3, p, NULL, NULL, .8f, angle, 32, spin, .8f, 1, .65f, 1);
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(c->particles, c->first_image, 3, p, NULL, NULL, .8f, angle, 48, 0, .8f, 1, .5f, 1);
}
static void flash(BHHomingProjectileContext *c, const BHVec3 *p)
{
    BHEntity *e = bh_entity_acquire(c->pool);
    if (e)
    {
        e->position = *p;
        bh_flash_flare_bind(c->flash, e, .1f, 50, 2);
    }
}
static void hit_packets(BHHomingProjectileContext *c, BHEntity *target, const BHVec3 *p, const BHVec3 *n, float impulse,
                        float damage, unsigned long suppress, BHEntity *source)
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
        if (!live(c, target, generation))
        {
            c->error = 3;
            return;
        }
        direct.event = 0;
        direct.amount = damage;
        direct.source = source;
        if (target->query)
        {
            target->query(target, &direct);
            ++c->damage_packets;
        }
    }
}
void __cdecl bh_homing_projectile_tick(BHEntity *e)
{
    BHHomingProjectileContext *c = context(e);
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
    double remaining, factor, dot;
    int result, is_world = 0;
    if (!c || !bh_math_environment_supported())
        return;
    memcpy(&generation, e->payload + 612, 4);
    if (!live(c, e, generation))
    {
        c->error = 3;
        return;
    }
    c->error = 0;
    get(e, 0xe8, &life, 4);
    remaining = (double)life - (1.0 / 30.0);
    bh_round_float(&life, remaining);
    put(e, 0xe8, &life, 4);
    if (!(remaining > 0))
    {
        bh_entity_release(c->pool, e);
        return;
    }
    get(e, 0x12c, &target, 4);
    if (target)
    {
        if (!target->query)
        {
            c->error = 2;
            return;
        }
        position.event = 3;
        position.position = target->position;
        target->query(target, &position);
        if (!live(c, e, generation))
        {
            c->error = 3;
            return;
        }
        get(e, 0x100, previous, 12);
        put(e, 0x100, &position.position, 12);
        old = e->position;
        velocity = e->origin;
        bh_round_float(&delta.x, (double)position.position.x - old.x);
        bh_round_float(&delta.y, (double)position.position.y - old.y);
        bh_round_float(&delta.z, (double)position.position.z - old.z);
        factor = sqrt(length2(&delta)) * (1.0 / 15.0);
        d = &delta.x;
        p = &position.position.x;
        for (i = 0; i < 3; ++i)
            bh_round_float(d + i, ((double)p[i] - previous[i]) * factor + d[i]);
        if (!bh_vec3_normalize_to(&velocity, &direction))
        {
            c->error = 4;
            return;
        }
        bh_vec3_normalize(&delta);
        dot = ((double)direction.x * delta.x + (double)direction.y * delta.y) + (double)direction.z * delta.z;
        if (dot > -.4)
        {
            v = &e->origin.x;
            d = &delta.x;
            p = &direction.x;
            for (i = 0; i < 3; ++i)
                bh_round_float(v + i, (double)v[i] - ((double)d[i] - dot * p[i]) * (-15.0));
        }
        ++c->guidance;
    }
    velocity = e->origin;
    old = e->position;
    offset(&end, &old, &velocity, -1.0 / 30.0);
    get(e, 0x124, &cached, 4);
    get(e, 0x128, &ignore, 4);
    get(e, 0x11c, &suppress, 4);
    get(e, 0x130, &damage_source, 4);
    self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
    self.object.actor = ignore;
    memset(&hit, 0, sizeof(hit));
    result = bh_world_segment_run(c->segment, ignore ? &self : NULL, &cached, &old, &end, -1, &hit);
    if (result < 0)
    {
        c->error = 2;
        return;
    }
    put(e, 0x124, &cached, 4);
    if (!result)
    {
        if (cached)
        {
            for (i = 0; i < c->segment->room_count; ++i)
                if (c->segment->rooms[i].world == cached)
                {
                    room = c->segment->rooms + i;
                    break;
                }
            if (!room)
            {
                c->error = 2;
                return;
            }
            for (actor = room->actors_owner ? *room->actors_owner : room->actors, n = 0; actor;)
            {
                if (++n > c->segment->max_room_actors)
                {
                    c->error = 2;
                    return;
                }
                if (actor != ignore)
                {
                    if (!actor->query)
                    {
                        c->error = 2;
                        return;
                    }
                    position.event = 3;
                    position.position = actor->position;
                    actor->query(actor, &position);
                    if (!live(c, e, generation))
                    {
                        c->error = 3;
                        return;
                    }
                    bh_round_float(&delta.x, (double)position.position.x - old.x);
                    bh_round_float(&delta.y, (double)position.position.y - old.y);
                    bh_round_float(&delta.z, (double)position.position.z - old.z);
                    if (length2(&delta) < 9)
                    {
                        bh_sound_request(c->sound, c->hit_sound, .8f, .75f, &old, &zero);
                        particles(c, &old);
                        flash(c, &old);
                        bh_entity_release(c->pool, e);
                        if (!bh_vec3_normalize_to(&delta, &normal))
                        {
                            c->error = 4;
                            return;
                        }
                        hit_packets(c, actor, &old, &normal, 8, .2f, suppress, damage_source);
                        ++c->proximity;
                        return;
                    }
                }
                get(actor, 0xf4, &actor, 4);
            }
        }
        e->position = end;
        return;
    }
    if (!receiver(c, &hit, &response, &is_world))
        return;
    normal = hit.normal;
    point = hit.point;
    ++c->world_hits;
    if (is_world && (hit.material_flags & 4))
    {
        offset(&effect, &point, &normal, -.5);
        bh_round_float(&duration, .6 - (double)rand() * (-1.52587890625e-6));
        contact = bh_contact_impact_create(c->contacts, &effect, &normal, 4);
        if (contact)
        {
            impact = bh_contact_impact_state(c->contacts, contact);
            impact->duration = duration;
            impact->size = 6;
            bh_sound_request(c->sound, c->liquid_sound, 1, (float)(1.0 / duration), &effect, &zero);
        }
        bh_entity_release(c->pool, e);
        return;
    }
    if (is_world && !(hit.material_flags & 2))
    {
        dot = ((double)velocity.x * normal.x + (double)velocity.y * normal.y) + (double)velocity.z * normal.z;
        if (dot > 0)
        {
            normal.x = -normal.x;
            normal.y = -normal.y;
            normal.z = -normal.z;
        }
        offset(&effect, &point, &normal, -.1);
        if (response)
            bh_hit_marker_create(c->markers, &effect, &normal, 1.5f, 2, response);
        else
        {
            c->error = 2;
            return;
        }
    }
    e->position = point;
    offset(&effect, &point, &normal, -.5);
    bh_sound_request(c->sound, c->hit_sound, .8f, .75f, &point, &zero);
    particles(c, &point);
    flash(c, &effect);
    bh_entity_release(c->pool, e);
    if (!bh_vec3_normalize_to(&velocity, &normal))
    {
        c->error = 4;
        return;
    }
    hit_packets(c, response, &point, &normal, -.1f, is_world ? .25f : .08f, suppress, damage_source);
}
