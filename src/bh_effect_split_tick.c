#include "bh_effect_split_tick.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0}, acceleration = {0, -.002f, 0};
static void get(const BHEntity *e, unsigned long a, void *p, unsigned long n)
{
    memcpy(p, (const unsigned char *)e + a, n);
}
static void put(BHEntity *e, unsigned long a, const void *p, unsigned long n)
{
    memcpy((unsigned char *)e + a, p, n);
}
static float scalar(const BHEntity *e, unsigned long a)
{
    float f;
    get(e, a, &f, 4);
    return f;
}
static unsigned long word(const BHEntity *e, unsigned long a)
{
    unsigned long n;
    get(e, a, &n, 4);
    return n;
}
static void value(BHEntity *e, unsigned long a, float f)
{
    put(e, a, &f, 4);
}
static void integer(BHEntity *e, unsigned long a, unsigned long n)
{
    put(e, a, &n, 4);
}
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static BHEffectProjectileContext *context(BHEntity *e)
{
    BHEffectProjectileContext *c;
    memcpy(&c, e->payload + 608, 4);
    return c;
}
static int live(BHEffectProjectileContext *c, BHEntity *e, unsigned long gen)
{
    BHEntity *p;
    unsigned long i = 0;
    for (p = c->effects->pool->active; p && i++ < c->effects->pool->capacity; p = p->next)
        if (p == e)
            return e->generation == gen;
    return 0;
}
static int random_value(BHEffectProjectileContext *c)
{
    ++c->random_calls;
    return rand();
}
static BHEntity *receiver(BHEffectProjectileContext *c, const BHWorldSegmentHit *hit)
{
    unsigned long i;
    if (hit->object.kind != BH_WORLD_HIT_GEOMETRY)
        return hit->object.object.actor;
    for (i = 0; i < c->effects->response_count; i++)
        if (c->effects->responses[i].world == hit->object.object.geometry)
            return c->effects->responses[i].receiver;
    return NULL;
}
static int packet(BHEffectProjectileContext *c, BHEntity *e, unsigned char kind, const BHVec3 *direction)
{
    if (!c->network || !bh_effect_network_emit(c->network, e, 0x138, 0x134, kind, direction))
    {
        c->error = 2;
        return 0;
    }
    return 1;
}
static void flash(BHEffectProjectileContext *c, const BHVec3 *p)
{
    BHEntity *e = bh_entity_acquire(c->effects->pool);
    if (e)
    {
        e->position = *p;
        bh_flash_flare_bind(c->effects->flash, e, .1f, 100, 4);
        ++c->flashes;
    }
}
int __cdecl bh_effect_split_cascade(BHEntity *e, const BHVec3 *normal)
{
    BHEffectProjectileContext *c = context(e);
    BHEffectSplitContext *s;
    BHWorldGeometry *world;
    BHEntity *source;
    BHVec3 velocity, old;
    long count, remaining, groups, extra, children[32], i, j;
    unsigned long identity, generation;
    float lifetime, gain, normal_gain;
    double coefficient;
    if (!c || !c->split || !normal || !bh_math_environment_supported())
        return 0;
    s = c->split;
    count = (long)word(e, 0x12c);
    if (count < 1 || count > 33)
    {
        s->error = 1;
        c->error = 1;
        return 0;
    }
    generation = e->generation;
    identity = word(e, 0x130);
    source = (BHEntity *)word(e, 0x144);
    world = (BHWorldGeometry *)word(e, 0x138);
    bh_sound_stop(c->effects->sound, word(e, 0x128));
    if (count >= 9)
    {
        c->debris->origin_tag = identity;
        bh_script_explosion_create(c->explosion, &e->position, 1, 10, 1, (unsigned long)source);
    }
    else if (count >= 4)
    {
        c->debris->origin_tag = identity;
        bh_script_explosion_secondary(c->explosion, &e->position, 1, 4, 1, s->split_sound, (unsigned long)source);
    }
    else if (count > 1)
        bh_sound_request(c->effects->sound, s->split_sound, 1, 1.2f, &e->position, &zero);
    if (!live(c, e, generation))
    {
        c->error = 3;
        return 0;
    }
    remaining = count - 1;
    integer(e, 0x12c, (unsigned long)remaining);
    if (remaining)
    {
        groups = remaining;
        if (remaining > 2)
            groups = (long)((unsigned long)random_value(c) * (remaining - 1) / 32768) + 2;
        for (i = 0; i < groups; i++)
            children[i] = 1;
        extra = remaining - groups;
        integer(e, 0x12c, (unsigned long)(extra - 1));
        for (i = 0; i < extra; i++)
        {
            j = (long)((unsigned long)random_value(c) * groups / 32768);
            ++children[j];
            integer(e, 0x12c, (unsigned long)(extra - i - 2));
        }
        gain = (float)(remaining * 5);
        get(e, 0xf8, &old, 12);
        for (i = 0; i < groups; i++)
        {
            for (j = 0; j < 3; j++)
                ((float *)&velocity)[j] = (float)(random_value(c) - 16384);
            bh_vec3_normalize(&velocity);
            for (j = 0; j < 3; j++)
                ((float *)&velocity)[j] =
                    stored((double)((float *)&velocity)[j] * gain - (double)((float *)&old)[j] * (-.5));
            normal_gain = stored((double)random_value(c) * (1.0 / 512));
            for (j = 0; j < 3; j++)
                ((float *)&velocity)[j] =
                    stored((double)((const float *)normal)[j] * normal_gain + ((float *)&velocity)[j]);
            coefficient = children[i] > 1 ? 1.1111111111111112e-5 : 1.0 / 32768;
            lifetime = stored((double)random_value(c) * coefficient);
            c->debris->origin_tag = identity;
            bh_effect_shard_create(c, world, source, &e->position, &velocity, lifetime, children[i], 0, .9f);
        }
    }
    bh_entity_release(c->effects->pool, e);
    ++s->cascades;
    return 1;
}
void __cdecl bh_effect_split_tick(BHEntity *e)
{
    BHEffectProjectileContext *c = context(e);
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
    if (!c || !c->split || !c->network || !bh_math_environment_supported())
        return;
    b = c->effects;
    s = c->split;
    c->error = 0;
    s->error = 0;
    generation = e->generation;
    unignore = (double)scalar(e, 0xec) - 1.0 / 30;
    value(e, 0xec, stored(unignore));
    if (unignore < 0)
        integer(e, 0x13c, 0);
    remaining = (double)scalar(e, 0xe8) - 1.0 / 30;
    value(e, 0xe8, stored(remaining));
    if (remaining <= 0)
    {
        if (!packet(c, e, 1, &zero))
            return;
        if (word(e, 0x130))
            integer(e, 0x130, 1);
        bh_effect_split_cascade(e, &zero);
        return;
    }
    get(e, 0xf8, &movement, 12);
    for (i = 0; i < 3; i++)
    {
        ((float *)&movement)[i] = stored((double)((float *)&movement)[i] * scalar(e, 0xf4));
        ((float *)&end)[i] = stored((double)((float *)&e->position)[i] + ((float *)&movement)[i]);
    }
    put(e, 0xf8, &movement, 12);
    r = random_value(c);
    speed = stored((double)r * (1.0 / 32768));
    r = random_value(c);
    lifetime = stored((double)r * (1.0 / 65536));
    bh_contact_spark_create(b->contacts, &e->position, lifetime, speed);
    for (i = 0; i < 3; i++)
        ((float *)&random_velocity)[i] = stored((double)(random_value(c) - 16384) * .000002);
    r = random_value(c);
    spin = stored((double)(r - 16384) * (1.0 / 16384));
    r = random_value(c);
    angle = stored((double)r * (1.0 / 512));
    bh_script_particle_create(b->particles, b->first_image, 3, &e->position, &random_velocity, &acceleration, .2f,
                              angle, 6, spin, .9f, .95f, .6f, .4f);
    ++c->particles;
    cached = (BHWorldGeometry *)word(e, 0x138);
    ignore = (BHEntity *)word(e, 0x13c);
    self.kind = BH_WORLD_HIT_ACTOR;
    self.object.actor = ignore;
    result = bh_world_segment_run(b->segment, ignore ? &self : NULL, &cached, &e->position, &end, 4, &hit);
    if (result < 0)
    {
        c->error = 2;
        return;
    }
    put(e, 0x138, &cached, 4);
    sound = word(e, 0x128);
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
        bh_entity_release(b->pool, e);
        ++s->liquid_hits;
        return;
    }
    identity = word(e, 0x130);
    if (result && identity != 1)
    {
        normal = hit.normal;
        response = receiver(c, &hit);
        if (hit.object.kind == BH_WORLD_HIT_GEOMETRY && !(hit.material_flags & 2))
        {
            dot = ((double)e->origin.z * normal.z + (double)e->origin.y * normal.y) + (double)e->origin.x * normal.x;
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
                c->error = 2;
                return;
            }
            marker = bh_hit_marker_create(b->markers, &point, &normal, 3, 1, response);
            drift.x = 0;
            drift.y = -1;
            drift.z = 0;
            bh_trail_create_pair(c->trails, c->trail_image, 3, 0, &point, marker, 6, 5, 6, .5f, .3f, .2f, &drift, .5f,
                                 NULL, NULL);
        }
        e->position = hit.point;
        if (!packet(c, e, 1, &zero))
            return;
        if (!identity)
        {
            if (!response)
            {
                c->error = 2;
                return;
            }
            damage.event = 0;
            damage.amount = 1;
            damage.source = (BHEntity *)word(e, 0x144);
            /* Original00404eb2 calls even the RET sentinel, then00404ec1 always
     cascades. NULL is the source representation of that no-op callback;
     it must not strand the projectile on the wall after emitting trails. */
            if (response->query)
                response->query(response, &damage);
            if (!live(c, e, generation))
            {
                c->error = 3;
                return;
            }
        }
        ++s->collisions;
        bh_effect_split_cascade(e, &normal);
        return;
    }
    e->position = end;
    for (i = 0; i < 3; i++)
        ((float *)&drift)[i] = stored((double)((float *)&movement)[i] * 30);
    bh_sound_update(b->sound, sound, 1, 2, &e->position, &drift);
    value(e, 0xfc, stored((double)scalar(e, 0xfc) - (-.001091111111111111)));
    if (!identity && (long)word(e, 0x12c) > 1 && random_value(c) < 1024)
    {
        for (i = 0; i < 3; i++)
            ((float *)&random_velocity)[i] = stored((double)(random_value(c) - 16384) * (1.0 / 262144));
        flash(c, &e->position);
        bh_sound_request(b->sound, c->ricochet_sound, 1, 2, &e->position, &drift);
        get(e, 0xf8, &movement, 12);
        for (i = 0; i < 3; i++)
            ((float *)&movement)[i] =
                stored((double)((float *)&movement)[i] - (double)(random_value(c) - 16384) * (-1.0 / 131072));
        put(e, 0xf8, &movement, 12);
        r = random_value(c);
        spin = stored((double)(r - 16384) * (1.0 / 16384));
        r = random_value(c);
        angle = stored((double)r * (1.0 / 512));
        bh_script_particle_create(b->particles, c->smoke_image, 3, &e->position, &random_velocity, &acceleration, .1f,
                                  angle, 4, spin, .9f, .95f, .5f, 1);
        ++c->particles;
        bh_vec3_normalize_to(&movement, &normalized);
        if (!packet(c, e, 4, &normalized))
            return;
        ++s->jitters;
    }
}
