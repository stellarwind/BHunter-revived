#include "bh_ballistic_projectile.h"
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
    BHEntity *p;
    unsigned long i = 0;
    for (p = c->pool->active; p && i++ < c->pool->capacity; p = p->next)
        if (p == e)
            return e->generation == generation;
    return 0;
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
int __cdecl bh_ballistic_projectile_bind(BHBallisticProjectileContext *owner, BHEntity *e)
{
    BHHomingProjectileContext *c = owner ? &owner->bindings : NULL;
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
    e->step = bh_ballistic_projectile_tick;
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
    bh_script_particle_create(c->particles, c->first_image, 3, p, NULL, NULL, .4f, angle, 10, spin, .8f, 1, .75f, 1);
    r = rand();
    bh_round_float(&spin, ((double)r - 16384.0) * (1.0 / 8192.0));
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    bh_script_particle_create(c->particles, c->first_image, 3, p, NULL, NULL, .2f, angle, 16, spin, .8f, 1, .65f, 1);
    bh_script_particle_create(c->particles, c->second_image, 3, p, NULL, NULL, .2f, 0, 24, 0, .8f, 1, .5f, 1);
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
static void retire(BHHomingProjectileContext *c, BHEntity *e, unsigned long voice)
{
    bh_sound_stop(c->sound, voice);
    bh_entity_release(c->pool, e);
}
void __cdecl bh_ballistic_projectile_tick(BHEntity *e)
{
    BHHomingProjectileContext *c = context(e);
    BHBallisticProjectileContext *owner = (BHBallisticProjectileContext *)c;
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
    get(e, 0x118, &voice, 4);
    remaining = (double)life - (1.0 / 30.0);
    bh_round_float(&life, remaining);
    put(e, 0xe8, &life, 4);
    if (!(remaining > 0))
    {
        retire(c, e, voice);
        return;
    }
    old = e->position;
    velocity = e->origin;
    offset(&end, &old, &velocity, -1.0 / 30.0);
    get(e, 0x124, &cached, 4);
    get(e, 0x128, &ignore, 4);
    get(e, 0x11c, &suppress, 4);
    get(e, 0x130, &source, 4);
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
                        bh_sound_stop(c->sound, voice);
                        bh_sound_request(c->sound, c->hit_sound, 1, 1, &old, &zero);
                        particles(c, &old);
                        flash(c, &old);
                        bh_entity_release(c->pool, e);
                        if (!bh_vec3_normalize_to(&delta, &normal))
                        {
                            c->error = 4;
                            return;
                        }
                        hit_packets(c, actor, &old, &normal, 8, .25f, suppress, source);
                        ++c->proximity;
                        return;
                    }
                }
                get(actor, 0xf4, &actor, 4);
            }
        }
        e->position = end;
        bh_sound_update(c->sound, voice, 1, 2, &e->position, &e->origin);
        return;
    }
    if (!receiver(c, &hit, &response, &is_world))
        return;
    ++c->world_hits;
    point = hit.point;
    normal = hit.normal;
    dot = ((double)velocity.x * normal.x + (double)velocity.y * normal.y) + (double)velocity.z * normal.z;
    bh_round_float(&stored_dot, dot);
    speed = sqrt(length2(&velocity));
    if (speed == 0)
    {
        c->error = 4;
        return;
    }
    bh_round_float(&incidence, dot / speed);
    if (incidence < 0 || _isnan(incidence))
        incidence = -incidence;
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
        retire(c, e, voice);
        return;
    }
    decision = rand();
    if (_isnan(incidence) || (double)incidence < (double)decision * (1.0 / 24000.0))
    {
        put(e, 0x128, &none, 4);
        e->position = point;
        bh_round_float(&e->origin.x, (double)normal.x * ((double)stored_dot * (-2.0)) + velocity.x);
        bh_round_float(&e->origin.y, (double)normal.y * ((double)stored_dot * (-2.0)) + velocity.y);
        bh_round_float(&e->origin.z, (double)normal.z * ((double)stored_dot * (-2.0)) + velocity.z);
        bh_sound_request(c->sound, owner->ricochet_sound, .3f, 2, &point, &zero);
        for (i = 0; i < 2; ++i)
        {
            contact = bh_contact_impact_create(c->contacts, &point, &normal, 2);
            if (contact)
            {
                impact = bh_contact_impact_state(c->contacts, contact);
                impact->duration = .5f;
                impact->size = i ? 10.0f : 6.0f;
            }
        }
        for (i = 0; i < 3; ++i)
            bh_contact_spark_create(c->contacts, &point, 1, 1);
        bh_round_float(&e->origin.x, (double)e->origin.x * .75);
        bh_round_float(&e->origin.y, (double)e->origin.y * .75);
        bh_round_float(&e->origin.z, (double)e->origin.z * .75);
        bh_round_float(&life, (double)life * .75);
        put(e, 0xe8, &life, 4);
        bh_round_float(&impulse, (double)incidence * (-.1));
        bh_round_float(&damage, (double)incidence * .25);
        hit_packets(c, response, &point, &normal, impulse, damage, suppress, source);
        ++owner->bounces;
        if (!live(c, e, generation))
        {
            c->error = 3;
            return;
        }
        bh_sound_update(c->sound, voice, 1, 2, &e->position, &e->origin);
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
            bh_hit_marker_create(c->markers, &effect, &normal, .7f, 0, response);
        else
        {
            c->error = 2;
            return;
        }
    }
    e->position = point;
    offset(&effect, &point, &normal, -.5);
    bh_sound_stop(c->sound, voice);
    bh_sound_request(c->sound, c->hit_sound, 1, 1, &point, &zero);
    particles(c, &effect);
    flash(c, &effect);
    bh_entity_release(c->pool, e);
    if (!bh_vec3_normalize_to(&velocity, &normal))
    {
        c->error = 4;
        return;
    }
    hit_packets(c, response, &point, &normal, -.1f, .3f, suppress, source);
}
