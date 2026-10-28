#include "bh_effect_orbit_tick.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static void get(const BHEntity *e, unsigned long at, void *out, unsigned long n)
{
    memcpy(out, (char *)e + at, n);
}
static void put(BHEntity *e, unsigned long at, const void *in, unsigned long n)
{
    memcpy((char *)e + at, in, n);
}
static unsigned long word(BHEntity *e, unsigned long at)
{
    unsigned long n;
    get(e, at, &n, 4);
    return n;
}
static float scalar(BHEntity *e, unsigned long at)
{
    float f;
    get(e, at, &f, 4);
    return f;
}
static float rf(double n)
{
    float f;
    bh_round_float(&f, n);
    return f;
}
static void value(BHEntity *e, unsigned long at, float f)
{
    put(e, at, &f, 4);
}
static int live(BHEffectOrbitTickContext *c, BHEntity *e, unsigned long generation)
{
    return bh_entity_is_active(c->core->effects->pool, e) && e->generation == generation;
}
int __cdecl bh_effect_orbit_tick_bind(BHEffectOrbitTickContext *c, BHEntity *e)
{
    if (!c || !c->core || !e || !c->core->effects || !c->core->effects->segment || !c->core->effects->contacts ||
        !c->core->effects->markers || !c->core->trails || !c->core->explosion || !c->core->network ||
        c->core->explosion->debris != c->core->debris)
        return 0;
    if (!bh_effect_projectile_bind(c->core, e, 1))
        return 0;
    c->core->orbit = c;
    e->step = bh_effect_orbit_tick;
    return 1;
}
static int announce(BHEffectOrbitTickContext *c, BHEntity *e)
{
    return bh_effect_network_emit(c->core->network, e, 0x138, 0x134, 0, &zero);
}
static void explode(BHEffectOrbitTickContext *c, BHEntity *e)
{
    BHEffectProjectileContext *b = c->core;
    b->debris->origin_tag = word(e, 0x130);
    bh_sound_stop(b->effects->sound, word(e, 0x128));
    bh_script_explosion_create(b->explosion, &e->position, 1.2f, 15, 1, word(e, 0x144));
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
static int motion(BHEffectOrbitTickContext *c, BHEntity *e, BHEntity *target, unsigned long gen, BHVec3 *position)
{
    BHEntityPositionPacket p;
    if (!target || !target->query)
    {
        c->error = 2;
        return 0;
    }
    p.event = 3;
    target->query(target, &p);
    if (!live(c, e, gen))
    {
        c->error = 3;
        return 0;
    }
    *position = p.position;
    return 1;
}
static BHEntity *receiver(BHEffectOrbitTickContext *c, BHWorldSegmentHit *hit)
{
    BHHomingProjectileContext *b = c->core->effects;
    unsigned long i;
    if (hit->object.kind == BH_WORLD_HIT_ACTOR)
        return hit->object.object.actor;
    for (i = 0; i < b->response_count; ++i)
        if (b->responses[i].world == hit->object.object.geometry)
            return b->responses[i].receiver;
    c->error = 2;
    return 0;
}
void __cdecl bh_effect_orbit_tick(BHEntity *e)
{
    BHEffectProjectileContext *core;
    BHEffectOrbitTickContext *c;
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
    memcpy(&core, e->payload + 608, 4);
    if (!core || !core->orbit)
        return;
    c = core->orbit;
    b = core->effects;
    memcpy(&gen, e->payload + 612, 4);
    if (!live(c, e, gen) || !bh_math_environment_supported())
    {
        c->error = 3;
        return;
    }
    c->error = 0;
    ++c->ticks;
    grace = (double)scalar(e, 0xec) - 1.0 / 30;
    value(e, 0xec, rf(grace));
    if (grace < 0)
    {
        ignore = 0;
        put(e, 0x13c, &ignore, 4);
    }
    life = (double)scalar(e, 0xe8) - 1.0 / 30;
    value(e, 0xe8, rf(life));
    identity = word(e, 0x130);
    source = word(e, 0x144);
    if (!(life > 0))
    {
        if (!announce(c, e))
        {
            c->error = 2;
            return;
        }
        explode(c, e);
        if (!live(c, e, gen))
        {
            c->error = 3;
            return;
        }
        bh_entity_release(b->pool, e);
        ++c->expired;
        return;
    }
    target = (BHEntity *)word(e, 0x140);
    get(e, 0xf8, &velocity, 12);
    get(e, 0x104, &acceleration, 12);
    if (target)
    {
        if (!motion(c, e, target, gen, &target_position))
            return;
        for (i = 0; i < 3; ++i)
            ((float *)&delta)[i] = rf((double)((float *)&target_position)[i] - ((float *)&e->position)[i]);
        length = sqrt(((double)delta.x * delta.x + (double)delta.y * delta.y) + (double)delta.z * delta.z);
        if (length < 3 && !identity)
        {
            if (!announce(c, e))
            {
                c->error = 2;
                return;
            }
            explode(c, e);
            if (!live(c, e, gen))
            {
                c->error = 3;
                return;
            }
            direct(target, rf(1.2 - (double)rf(length) * .1), source);
            bh_entity_release(b->pool, e);
            ++c->proximity;
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
        put(e, 0xf8, &velocity, 12);
        put(e, 0x104, &acceleration, 12);
    }
    cached = (BHWorldGeometry *)word(e, 0x138);
    ignore = (BHEntity *)word(e, 0x13c);
    if (cached && !identity)
    {
        for (i = 0; i < b->segment->room_count; ++i)
            if (b->segment->rooms[i].world == cached)
            {
                room = b->segment->rooms + i;
                break;
            }
        if (!room)
        {
            c->error = 2;
            return;
        }
        for (actor = room->actors_owner ? *room->actors_owner : room->actors, n = 0; actor;)
        {
            if (++n > b->segment->max_room_actors)
            {
                c->error = 2;
                return;
            }
            if (actor != ignore)
            {
                if (!motion(c, e, actor, gen, &target_position))
                    return;
                for (i = 0; i < 3; ++i)
                    ((float *)&delta)[i] = rf((double)((float *)&target_position)[i] - ((float *)&e->position)[i]);
                length = ((double)delta.x * delta.x + (double)delta.y * delta.y) + (double)delta.z * delta.z;
                if (length < 9)
                {
                    if (!announce(c, e))
                    {
                        c->error = 2;
                        return;
                    }
                    explode(c, e);
                    if (!live(c, e, gen))
                    {
                        c->error = 3;
                        return;
                    }
                    bh_entity_release(b->pool, e);
                    direct(actor, rf(1.2 - sqrt(length) * .1), source);
                    ++c->proximity;
                    return;
                }
            }
            get(actor, 0xf4, &actor, 4);
        }
    }
    for (i = 0; i < 3; ++i)
    {
        ((float *)&velocity)[i] = rf((double)((float *)&acceleration)[i] + ((float *)&velocity)[i]);
        ((float *)&end)[i] = rf((double)((float *)&e->position)[i] + ((float *)&velocity)[i]);
    }
    put(e, 0xf8, &velocity, 12);
    self.kind = ignore ? BH_WORLD_HIT_ACTOR : BH_WORLD_HIT_END;
    self.object.actor = ignore;
    result = bh_world_segment_run(b->segment, ignore ? &self : 0, &cached, &e->position, &end, -1, &hit);
    if (result < 0)
    {
        c->error = 2;
        return;
    }
    put(e, 0x138, &cached, 4);
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
        bh_sound_stop(b->sound, word(e, 0x128));
        {
            struct
            {
                long event;
                BHEntity *actor;
            } p;
            p.event = 17;
            p.actor = e;
            bh_entities_query(b->pool, 0, 0, 0, 0, &p);
        }
        if (!live(c, e, gen))
        {
            c->error = 3;
            return;
        }
        bh_entity_release(b->pool, e);
        ++c->liquid_hits;
        return;
    }
    if (!result || identity)
    {
        e->position = end;
        for (i = 0; i < 3; ++i)
            ((float *)&velocity)[i] = rf((double)((float *)&velocity)[i] * 30);
        bh_sound_update(b->sound, word(e, 0x128), 1, 1, &e->position, &velocity);
        return;
    }
    response = receiver(c, &hit);
    if (c->error)
        return;
    normal = hit.normal;
    if (is_world && !(hit.material_flags & 2))
    {
        dot = ((double)e->origin.z * normal.z + (double)e->origin.x * normal.x) + (double)e->origin.y * normal.y;
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
            c->error = 2;
            return;
        }
        marker = bh_hit_marker_create(b->markers, &point, &normal, 5, 1, response);
        bh_trail_create_pair(core->trails, core->trail_image, 3, 0, &point, marker, 10, 5, 6, .5f, .3f, .2f, &drift,
                             .5f, 0, 0);
    }
    e->position = hit.point;
    if (!announce(c, e))
    {
        c->error = 2;
        return;
    }
    explode(c, e);
    if (!live(c, e, gen))
    {
        c->error = 3;
        return;
    }
    direct(response, 1, source);
    bh_entity_release(b->pool, e);
    ++c->world_hits;
}
