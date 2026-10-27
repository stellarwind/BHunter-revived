#include "bh_script_actor_construct.h"
#include "bh_script_actor_tick.h"
#include "bh_actor_route_lifetime.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
typedef char constructor_abi[sizeof(void *) == 4 && sizeof(unsigned long) == 4 ? 1 : -1];
static void put(BHEntity *e, unsigned long off, const void *v, unsigned long n)
{
    memcpy((char *)e + off, v, n);
}
static void word(BHEntity *e, unsigned long off, unsigned long v)
{
    put(e, off, &v, 4);
}
static void real_clock(BHEntity *e)
{
    BHScriptActorContext *c = bh_script_actor_context(e);
    bh_script_actor_clock_tick(c->route, e);
}
static void real_phase(BHEntity *e)
{
    BHScriptActorContext *c = bh_script_actor_context(e);
    bh_script_actor_phase_tick(c->route, e, c->pulse_resource);
}
static void real_draw(BHEntity *e)
{
    BHScriptActorContext *c = bh_script_actor_context(e);
    bh_script_actor_draw(c->draw, e);
}
static void real_vehicle_draw(BHEntity *e)
{
    BHScriptActorContext *c = bh_script_actor_context(e);
    bh_script_actor_vehicle_draw(c->draw, e);
}
void __cdecl bh_script_actor_present(BHEntity *e)
{
    BHEntityStep draw;
    memcpy(&draw, (char *)e + 0xe8, 4);
    if (draw)
        draw(e);
}
BHEntity *__cdecl bh_script_actor_construct(BHScriptActorConstructorContext *c, const BHVec3 *position,
                                            unsigned long type)
{
    BHScriptActorContext *callbacks;
    BHActorRouteContext *route;
    BHWorldGeometry *world;
    BHPickupRoom *room = NULL;
    BHEntity *e, *head;
    BHActorRouteState s;
    BHVec3 zero = {0, 0, 0}, center, offset;
    BHEntityStep draw;
    unsigned long i, first, second;
    if (!c || !position || type > 3)
        return NULL;
    callbacks = &c->callbacks;
    route = callbacks->route;
    if (!callbacks->pool || !route || !callbacks->draw || !callbacks->draw->queue || !callbacks->draw->particles ||
        !route->rooms || !route->room_count || !bh_math_environment_supported())
        return NULL;
    world = bh_world_locate(c->first_world, c->world_count, NULL, position, 0);
    if (!world)
        return NULL;
    for (i = 0; i < route->room_count; ++i)
        if (route->rooms[i].world == world)
        {
            room = route->rooms + i;
            break;
        }
    if (!room)
        return NULL;
    e = bh_entity_acquire(callbacks->pool);
    if (!e)
        return NULL;
    memset(&s, 0, sizeof(s));
    e->position = zero;
    e->origin = zero;
    e->flags |= 0x100;
    s.current = s.next = world;
    put(e, 0xc4, &world, 4);
    head = room->actors;
    put(e, 0xf4, &head, 4);
    room->actors = e;
    word(e, 0xf8, 0);
    if (head)
        put(head, 0xf8, &e, 4);
    first = (unsigned long)rand();
    second = (unsigned long)rand();
    s.random = first + second * 65536UL;
    bh_round_float(&center.x, ((double)world->minimum.x + world->maximum.x) * .5);
    bh_round_float(&center.y, ((double)world->minimum.y + world->maximum.y) * .5);
    bh_round_float(&center.z, ((double)world->minimum.z + world->maximum.z) * .5);
    for (i = 0; i < 4; ++i)
        s.controls[i] = center;
    s.portal_center = center;
    s.speed = 80;
    s.u = .5f;
    s.v = .5f;
    s.speed_scale = 1;
    s.duration = 1;
    s.duration_ticks = 30;
    s.tick = (unsigned short)(bh_actor_route_now(route) - 30);
    s.sound = BH_SOUND_INVALID;
    bh_actor_route_write(e, &s);
    put(e, 0x15c, &zero, 12);
    put(e, 0x168, &zero, 12);
    word(e, 0x19c, 0x3f800000);
    word(e, 0x1b0, 0x3e4ccccd);
    word(e, 0x100, 0);
    word(e, 0x118, 0);
    word(e, 0x11c, 0);
    word(e, 0x1ac, 0);
    word(e, 0x114, c->model_resources[type]);
    word(e, 0x10c, c->sound_resource);
    word(e, 0xfc, type < 2 ? 4 : 5);
    if (type < 2)
    {
        offset.x = 5;
        offset.y = 5;
        offset.z = 10;
        draw = real_draw;
    }
    else
    {
        offset = zero;
        draw = real_vehicle_draw;
    }
    put(e, 0x174, &offset, 12);
    put(e, 0xe8, &draw, 4);
    e->step = type == 2 ? real_phase : real_clock;
    if (!bh_script_actor_bind(callbacks, e) || !bh_script_actor_route_advance(route, e))
    {
        bh_actor_route_release(route, callbacks->pool, e);
        return NULL;
    }
    return e;
}
