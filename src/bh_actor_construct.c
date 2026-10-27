#include "bh_actor_construct.h"
#include "bh_round.h"
#include "bh_route_query.h"
#include "bh_fp.h"
#include <math.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static void word(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static void real(BHEntity *e, unsigned long o, float f)
{
    memcpy((char *)e + o, &f, 4);
}
static void short_word(BHEntity *e, unsigned long o, unsigned short v)
{
    memcpy((char *)e + o, &v, 2);
}
static unsigned long next_random(unsigned long r)
{
    return ((r * 0xe357UL >> 16) | (r * 0xe3570000UL)) ^ r;
}
static BHActorConstructContext *context(BHEntity *e)
{
    BHActorConstructContext *c;
    memcpy(&c, e->payload + 584, 4);
    return c;
}
static void route_tick(BHEntity *e)
{
    BHActorConstructContext *c = context(e);
    if (!bh_actor_route_due(c->response->damage->route, e))
        c->error = 2;
}
static void draw_actor(BHEntity *e)
{
    BHActorConstructContext *c = context(e);
    if (!bh_actor_vehicle_draw(c->draw, e))
        c->error = 2;
}
static BHEntity *construct(BHActorConstructContext *c, BHWorldGeometry *world, unsigned long type, long token,
                           float speed, unsigned long death_tag, unsigned long seed)
{
    BHActorResponseContext *response;
    BHActorRouteState s;
    BHVec3 center, zero = {0, 0, 0};
    const BHActorConstructRoom *binding = 0;
    BHEntity *e, *head;
    BHEntityStep draw = draw_actor;
    unsigned long i, index, r;
    float original_speed = speed;
    long bounty;
    unsigned short primary, secondary;
    double v;
    if (!c || !world || !c->response || !c->draw || !c->models || !c->sounds || c->resource_count < 25 || !c->rooms ||
        !c->cycle_models || c->cycle_count < 4 || !bh_math_environment_supported())
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->error = 0;
    response = c->response;
    for (i = 0; i < c->room_count; ++i)
        if (c->rooms[i].room && c->rooms[i].room->world == world)
        {
            binding = c->rooms + i;
            break;
        }
    if (!binding || !_finite(speed))
    {
        c->error = 1;
        return 0;
    }
    if (type == 4 || type == 5)
        type = 6;
    e = bh_entity_acquire(response->damage->pool);
    if (!e)
    {
        c->error = 2;
        return 0;
    }
    memcpy(e->payload + 584, &c, 4);
    e->position = zero;
    memcpy((char *)e + 0x3c, &zero, 12);
    e->flags |= 0x100;
    memcpy(e->opaque2b, &world, 4);
    memset(&s, 0, sizeof(s));
    s.current = s.next = world;
    center.x = rf(((double)world->minimum.x + world->maximum.x) * .5);
    center.y = rf(((double)world->minimum.y + world->maximum.y) * .5);
    center.z = rf(((double)world->minimum.z + world->maximum.z) * .5);
    for (i = 0; i < 4; ++i)
        s.controls[i] = center;
    s.portal_center = center;
    head = binding->room->actors;
    word(e, 0xf4, (unsigned long)head);
    word(e, 0xf8, 0);
    if (head)
        word(head, 0xf8, (unsigned long)e);
    binding->room->actors = e;
    memset((char *)e + 0x15c, 0, 24);
    real(e, 0x194, 0);
    s.duration = s.phase = 1;
    r = next_random(seed);
    s.u = rf(.25 - (r & 32767) * (-1.0 / 65536.0));
    r = next_random(r);
    s.v = rf(.25 - (r & 32767) * (-1.0 / 65536.0));
    s.tick = (unsigned short)(bh_actor_route_now(response->damage->route) - 30);
    s.duration_ticks = 30;
    if (speed == 0)
    {
        r = next_random(r);
        speed = rf(15.0 - (r & 32767) * (-1.0 / 1024.0));
    }
    s.speed = speed;
    s.jitter = .2f;
    s.speed_scale = 1;
    s.sound = BH_SOUND_INVALID;
    s.sound_frames = 0;
    word(e, 0x100, token ? (unsigned long)token : (unsigned long)-5000);
    word(e, 0x11c, death_tag);
    real(e, 0x19c, 1);
    word(e, 0x1ac, 0);
    real(e, 0x1b4, 0);
    word(e, 0x1b8, 0);
    word(e, 0x110, binding->bound_resource);
    word(e, 0xfc, type);
    word(e, 0x118, 0);
    memcpy(e->payload + 4, &draw, 4);
    r = next_random(r);
    index = ((r & 32767) * 23) >> 15;
    if (index > 7)
        ++index;
    if (index > 14)
        ++index;
    word(e, 0x114, c->models[index]);
    word(e, 0x10c, c->sounds[index]);
    if (type == 12)
    {
        type = 6;
        word(e, 0xfc, 6);
        word(e, 0x118, 1);
    }
    if (type == 7)
    {
        word(e, 0x114, c->models[8]);
        word(e, 0x10c, c->sounds[8]);
        type = 6;
        word(e, 0xfc, 6);
    }
    if (type == 10)
    {
        type = 2;
        word(e, 0xfc, 2);
        real(e, 0x1b4, .45f);
        word(e, 0x1b8, (unsigned long)response->player);
    }
    if (type == 11)
    {
        type = 6;
        word(e, 0xfc, 6);
        word(e, 0x1b8, (unsigned long)response->player);
    }
    if (type == 13)
    {
        type = 6;
        word(e, 0xfc, 6);
        word(e, 0x114, c->cycle_models[c->cycle & 3]);
        ++c->cycle;
        word(e, 0x10c, c->sounds[14]);
    }
    if (type == 14 || type == 15)
    {
        word(e, 0x114, type == 15 ? c->special_model : c->models[8]);
        word(e, 0x10c, c->sounds[8]);
        if (type == 15)
            real(e, 0x194, -10);
        type = 6;
        word(e, 0xfc, 6);
        word(e, 0x1b8, (unsigned long)response->player);
    }
    if (type == 3)
    {
        word(e, 0x114, c->models[8]);
        word(e, 0x10c, c->sounds[8]);
        if (!token)
            word(e, 0x100, (unsigned long)-30000);
    }
    if (type == 1 || type == 2 || type == 6)
    {
        head = response->vehicles->head;
        word(e, 0xf0, (unsigned long)head);
        word(e, 0xec, 0);
        if (head)
            word(head, 0xec, (unsigned long)e);
        response->vehicles->head = e;
        ++response->vehicles->count;
        if (type == 1)
        {
            if (original_speed == 0)
                s.speed = rf((double)s.speed - (-10.0));
            real(e, 0x1b4, 1);
            if (!token)
                word(e, 0x100, (unsigned long)-15000);
        }
        else
        {
            s.jitter = .3f;
            if (!token)
            {
                v = (double)s.speed - 20.0;
                v = v * v * (type == 2 ? -.0078125 : -.015625);
                bounty = (long)(__int64)v;
                word(e, 0x100, (1UL - (unsigned long)bounty) * 5000UL);
            }
            if (type == 2 && original_speed == 0)
                s.speed = rf((double)s.speed - (-5.0));
        }
        r = next_random(r);
        if (type == 2)
        {
            real(e, 0x1b4, rf((r & 32767) * (1.0 / 131072.0)));
            r = next_random(r);
        }
        else
            real(e, 0x1b4, 1);
        primary = (unsigned short)(__int64)((r & 32767) * (1.0 / 11000.0));
        r = next_random(r);
        secondary = (unsigned short)(__int64)((r & 32767) * (1.0 / 11000.0));
        short_word(e, 0x1dc, primary);
        short_word(e, 0x1de, secondary);
        short_word(e, 0x1e0, 0);
        short_word(e, 0x1e2, type == 6 ? 20 : 10);
        word(e, 0x1d8, (type == 1 ? 30 : type == 2 ? 40 : 60) / (primary == 1 ? 2 : 1));
        word(e, 0x1d4, 0);
    }
    s.random = r;
    bh_actor_route_write(e, &s);
    if (!bh_actor_damage_bind(response->damage, e))
    {
        c->error = 2;
        return e;
    }
    e->step = route_tick;
    if (type == 1 || type == 2 || type == 6)
    {
        if (!bh_actor_vehicle_tick_bind(response->attack, e) || !bh_actor_response_bind(response, e))
            c->error = 2;
    }
    return e;
}
BHEntity *__cdecl bh_actor_construct_room(BHActorConstructContext *c, BHWorldGeometry *w, unsigned long t, long token,
                                          float speed, unsigned long tag, unsigned long seed)
{
    return construct(c, w, t, token, speed, tag, seed);
}
BHEntity *__cdecl bh_actor_construct_position(BHActorConstructContext *c, const BHVec3 *p, unsigned long t, long token,
                                              float speed, unsigned long tag, unsigned long seed)
{
    BHWorldGeometry *w;
    if (!c || !p)
        return 0;
    w = bh_world_locate(c->first, c->world_count, 0, p, 0);
    if (!w)
        w = bh_world_locate(c->first, c->world_count, 0, p, 1);
    return construct(c, w, t, token, speed, tag, seed);
}
