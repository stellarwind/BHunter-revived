#include "bh_actor_route_lifetime.h"
#include <string.h>
static int fail(BHActorRouteContext *c, BHActorRouteError e)
{
    if (c)
        c->error = e;
    return 0;
}
static BHEntity *link_value(const BHEntity *a, unsigned long offset)
{
    BHEntity *p;
    memcpy(&p, (const unsigned char *)a + offset, 4);
    return p;
}
#include "bh_actor_route_release_core.inc"
#include "bh_actor_route_due_core.inc"
int __cdecl bh_actor_route_due(BHActorRouteContext *c, BHEntity *a)
{
    const unsigned short *clock;
    if (!c || !a)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    clock = c->live_tick ? (const unsigned short *)c->live_tick : &c->tick;
    if (!bh_actor_route_due_core(a, c, clock))
        return 0;
    c->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
int __cdecl bh_actor_route_release(BHActorRouteContext *c, BHEntityPool *pool, BHEntity *a)
{
    BHActorRouteState s;
    BHPickupRoom *room = NULL;
    BHEntity *next, *previous, *at;
    BHActorRouteReleaseBinding binding;
    unsigned long i, n;
    if (!c || !pool || !a || !c->rooms || !c->room_count || !c->actor_limit)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_entity_is_active(pool, a))
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    bh_actor_route_read(a, &s);
    for (i = 0; i < c->room_count; ++i)
        if (c->rooms[i].world == s.current)
        {
            room = c->rooms + i;
            break;
        }
    if (!room)
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    next = link_value(a, 0xf4);
    previous = link_value(a, 0xf8);
    if ((previous ? link_value(previous, 0xf4) != a : room->actors != a) || (next && link_value(next, 0xf8) != a))
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    at = room->actors;
    n = 0;
    while (at && at != a && n < c->actor_limit)
    {
        at = link_value(at, 0xf4);
        ++n;
    }
    if (at != a || n >= c->actor_limit)
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    if (s.sound != BH_SOUND_INVALID && !c->sound)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    binding.sound = c->sound;
    binding.pool = pool;
    binding.room_head = &room->actors;
    if (!bh_actor_route_release_core(a, &binding))
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    c->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
