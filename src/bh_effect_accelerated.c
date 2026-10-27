#include "bh_effect_accelerated.h"
#include "bh_delayed_expiry.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static void get(const BHEntity *e, unsigned long a, void *p, unsigned long n)
{
    memcpy(p, (const char *)e + a, n);
}
static void put(BHEntity *e, unsigned long a, const void *p, unsigned long n)
{
    memcpy((char *)e + a, p, n);
}
static float scalar(const BHEntity *e, unsigned long a)
{
    float f;
    get(e, a, &f, 4);
    return f;
}
static float rf(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static void value(BHEntity *e, unsigned long a, float f)
{
    put(e, a, &f, 4);
}
static unsigned long number(const BHEntity *e, unsigned long a)
{
    unsigned long n;
    get(e, a, &n, 4);
    return n;
}
static BHEffectProjectileContext *context(BHEntity *e)
{
    BHEffectProjectileContext *c;
    memcpy(&c, e->payload + 608, 4);
    return c;
}
static int random_value(BHEffectProjectileContext *c)
{
    ++c->random_calls;
    return rand();
}
static void offset(BHVec3 *p, const BHVec3 *a, const BHVec3 *n, double d)
{
    p->x = rf((double)a->x + (double)n->x * d);
    p->y = rf((double)a->y + (double)n->y * d);
    p->z = rf((double)a->z + (double)n->z * d);
}
static void retire(BHEffectProjectileContext *c, BHEntity *e)
{
    bh_sound_stop(c->effects->sound, number(e, 0x128));
    bh_entity_release(c->effects->pool, e);
}
static BHEntity *response(BHHomingProjectileContext *b, const BHWorldSegmentHit *h)
{
    unsigned long i;
    if (h->object.kind != BH_WORLD_HIT_GEOMETRY)
        return h->object.object.actor;
    for (i = 0; i < b->response_count; ++i)
        if (b->responses[i].world == h->object.object.geometry)
            return b->responses[i].receiver;
    return NULL;
}
static void puff(BHEffectProjectileContext *c, BHEntity *e, unsigned long image, float life, float delay, float size,
                 int ring)
{
    BHEntity *p = bh_entity_acquire(c->effects->pool);
    BHEffectAcceleratedContext *a = c->accelerated;
    unsigned long seed;
    if (!p)
        return;
    p->position = e->position;
    put(p, 0xf0, &image, 4);
    value(p, 0xec, life);
    value(p, 0x194, size);
    bh_delayed_expiry_bind(c->effects->pool, p, life, delay);
    if (!ring)
    {
        seed = (unsigned long)random_value(c);
        seed = seed * 32768UL + (unsigned long)random_value(c);
        seed = seed * 32768UL + (unsigned long)random_value(c);
        put(p, 0x1a8, &seed, 4);
    }
    bh_effect_puff_bind(a->puffs, p, ring);
    ++a->puff_entities;
}
void __cdecl bh_effect_accelerated_tick(BHEntity *e)
{
    BHEffectProjectileContext *c = context(e);
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
    if (!c || !c->accelerated || !bh_math_environment_supported())
        return;
    a = c->accelerated;
    b = c->effects;
    memcpy(&generation, e->payload + 612, 4);
    if (e->generation != generation)
    {
        a->error = 1;
        return;
    }
    next = (double)scalar(e, 0xec) - 1.0 / 30.0;
    value(e, 0xec, rf(next));
    if (next < 0)
    {
        ignore = NULL;
        put(e, 0x13c, &ignore, 4);
    }
    next = (double)scalar(e, 0xf4) + 1.0 / 30.0;
    value(e, 0xf4, rf(next));
    if (next > 20)
    {
        retire(c, e);
        return;
    }
    for (i = 0; i < 2; ++i)
    {
        r1 = rf((double)random_value(c) / 32768.0);
        r2 = rf((double)random_value(c) / 32768.0);
        bh_contact_spark_create(b->contacts, &e->position, r2, r1);
    }
    smoke.x = rf(((double)random_value(c) - 16384) * .000002);
    smoke.y = rf(((double)random_value(c) - 16384) * .000002);
    smoke.z = rf(((double)random_value(c) - 16384) * .000002);
    if (!(random_value(c) & 3))
    {
        pitch = rf(2.0 - (double)random_value(c) * (-1.0 / 16384.0));
        bh_sound_request(b->sound, c->ricochet_sound, .5f, pitch, &e->position, &e->origin);
        spin = rf(((double)random_value(c) - 16384) / 16384.0);
        angle = rf((double)random_value(c) / 512.0);
        bh_script_particle_create(b->particles, c->smoke_image, 3, &e->position, &smoke, &gravity, .2f, angle, 6, spin,
                                  .9f, .95f, .6f, .6f);
    }
    if (!number(e, 0x12c))
    {
        old = e->position;
        get(e, 0x60, &acc, 12);
        x = (double)e->origin.x + (double)acc.x / 30.0;
        y = (double)e->origin.y + (double)acc.y / 30.0;
        z = (double)e->origin.z + (double)acc.z / 30.0;
        e->origin.x = rf(x);
        e->origin.y = rf(y);
        e->origin.z = rf(z);
        value(e, 0x44, 0);
        value(e, 0x3c, rf(atan2(e->origin.x, e->origin.z)));
        value(e, 0x40, rf(atan2(-e->origin.y, sqrt(z * z + (double)e->origin.x * e->origin.x))));
        offset(&end, &old, &e->origin, 1.0 / 30.0);
        speed = sqrt(((double)e->origin.y * e->origin.y + (double)rf(z * z)) + (double)e->origin.x * e->origin.x);
        voice = number(e, 0x128);
        bh_sound_update(b->sound, voice, rf(speed * .02), .5f, &old, &e->origin);
        get(e, 0x138, &cached, 4);
        get(e, 0x13c, &ignore, 4);
        self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
        self.object.actor = ignore;
        memset(&hit, 0, sizeof(hit));
        result = bh_world_segment_run(b->segment, ignore ? &self : NULL, &cached, &old, &end, -1, &hit);
        put(e, 0x138, &cached, 4);
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
            retire(c, e);
            return;
        }
        suppress = number(e, 0x130);
        if (result && !suppress)
        {
            ++a->collisions;
            value(e, 0xe8, 3);
            n = hit.normal;
            receiver = response(b, &hit);
            if (world && !(hit.material_flags & 2))
            {
                if (((double)e->origin.x * n.x + (double)e->origin.y * n.y) + (double)e->origin.z * n.z > 0)
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
                bh_trail_create_pair(c->trails, c->trail_image, 3, 0, &p, marker, 15, 5, 6, .5f, .3f, .2f, &drift, .75f,
                                     NULL, NULL);
            }
            v = e->origin;
            bh_vec3_normalize(&v);
            offset(&e->position, &hit.point, &v, -1);
            e->origin = zero;
            put(e, 0x60, &zero, 12);
            bh_sound_stop(b->sound, voice);
            voice = BH_SOUND_INVALID;
            put(e, 0x128, &voice, 4);
            if (c->network)
                bh_effect_network_emit(c->network, e, 0x138, 0x134, 2, &zero);
            bh_effect_detonate(c, e);
            return;
        }
        e->position = end;
        return;
    }
    next = (double)scalar(e, 0xe8) - 1.0 / 30.0;
    value(e, 0xe8, rf(next));
    if (next < 2.0 && next >= 1.9666666666666666)
    {
        voice = bh_sound_request(b->sound, a->interval_sound, 1, 1, &e->position, &zero);
        put(e, 0x128, &voice, 4);
    }
    if (!(scalar(e, 0xe8) < 0))
        return;
    bh_sound_stop(b->sound, number(e, 0x128));
    bh_sound_request(b->sound, a->expiry_sound, 1, 1, &e->position, &zero);
    c->debris->origin_tag = number(e, 0x130);
    get(e, 0x144, &owner, 4);
    bh_script_explosion_create(c->explosion, &e->position, 1.2f, 30, 1, (unsigned long)owner);
    puff(c, e, a->ring_image, 1, 0, 800, 1);
    puff(c, e, a->second_ring_image, 1, .1f, 400, 1);
    puff(c, e, a->random_image, 1, .05f, 100, 0);
    puff(c, e, a->ring_image, .25f, .4f, 200, 1);
    puff(c, e, a->random_image, .5f, .5f, 50, 0);
    if (!number(e, 0x130))
    {
        damage.event = 0;
        damage.amount = 1;
        damage.source = owner;
        bh_entities_query(b->pool, 0, 0, &e->position, 100, &damage);
    }
    bh_entity_release(b->pool, e);
}
