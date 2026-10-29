#include "bh_actor_route.h"
#include "bh_axis_snap.h"
#include "bh_route_choice.h"
#include "bh_route_guided.h"
#include "bh_fp.h"
#include <stddef.h>
#include <string.h>
#include <float.h>

typedef char route_actor_links[offsetof(BHEntity, payload) == 0xe4 && sizeof(void *) == 4 ? 1 : -1];
typedef char route_actor_owner[offsetof(BHEntity, opaque2b) == 0xc4 ? 1 : -1];
static void read_field(const BHEntity *a, unsigned long offset, void *out, unsigned long n)
{
    memcpy(out, (const unsigned char *)a + offset, n);
}
static void write_field(BHEntity *a, unsigned long offset, const void *in, unsigned long n)
{
    memcpy((unsigned char *)a + offset, in, n);
}
#define READ(member, offset) read_field(a, offset, &s->member, sizeof(s->member))
#define WRITE(member, offset) write_field(a, offset, &s->member, sizeof(s->member))
unsigned short __cdecl bh_actor_route_now(const BHActorRouteContext *c)
{
    return c->live_tick ? (unsigned short)*c->live_tick : c->tick;
}
void __cdecl bh_actor_route_set_tick(BHActorRouteContext *c, unsigned short tick)
{
    if (c->live_tick)
        *c->live_tick = (*c->live_tick & 0xffff0000UL) | tick;
    else
        c->tick = tick;
}
int __cdecl bh_actor_route_read(const BHEntity *a, BHActorRouteState *s)
{
    if (!a || !s)
        return 0;
    READ(current, 0x1a4);
    READ(next, 0x1a8);
    READ(controls[0], 0x120);
    READ(controls[1], 0x144);
    READ(controls[2], 0x150);
    READ(controls[3], 0x12c);
    READ(portal_center, 0x138);
    READ(phase, 0x180);
    READ(duration, 0x184);
    READ(speed, 0x188);
    READ(u, 0x18c);
    READ(v, 0x190);
    READ(speed_scale, 0x198);
    READ(jitter, 0x1b0);
    READ(sound, 0x104);
    READ(sound_frames, 0x108);
    READ(random, 0x1e8);
    READ(tick, 0x1e4);
    READ(duration_ticks, 0x1e6);
    return 1;
}
int __cdecl bh_actor_route_write(BHEntity *a, const BHActorRouteState *s)
{
    if (!a || !s)
        return 0;
    WRITE(current, 0x1a4);
    WRITE(next, 0x1a8);
    WRITE(controls[0], 0x120);
    WRITE(controls[1], 0x144);
    WRITE(controls[2], 0x150);
    WRITE(controls[3], 0x12c);
    WRITE(portal_center, 0x138);
    WRITE(phase, 0x180);
    WRITE(duration, 0x184);
    WRITE(speed, 0x188);
    WRITE(u, 0x18c);
    WRITE(v, 0x190);
    WRITE(speed_scale, 0x198);
    WRITE(jitter, 0x1b0);
    WRITE(sound, 0x104);
    WRITE(sound_frames, 0x108);
    WRITE(random, 0x1e8);
    WRITE(tick, 0x1e4);
    WRITE(duration_ticks, 0x1e6);
    return 1;
}
#undef READ
#undef WRITE
static int fail(BHActorRouteContext *c, BHActorRouteError e)
{
    if (c)
        c->error = e;
    return 0;
}
static BHPickupRoom *room_binding(BHActorRouteContext *c, const BHWorldGeometry *world)
{
    unsigned long i;
    for (i = 0; i < c->room_count; ++i)
        if (c->rooms[i].world == world)
            return c->rooms + i;
    return NULL;
}
static BHEntity *actor_link(const BHEntity *a, unsigned long offset)
{
    BHEntity *out;
    read_field(a, offset, &out, 4);
    return out;
}
static void set_link(BHEntity *a, unsigned long offset, BHEntity *value)
{
    write_field(a, offset, &value, 4);
}
static int vector_finite(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int portal_valid(const BHPortal *p)
{
    unsigned long i;
    if (p->direction > 6 || p->direction == 3)
        return 0;
    for (i = 0; i < 4; ++i)
        if (!_finite(p->scratch[i]))
            return 0;
    return p->scratch[0] <= p->scratch[1] && p->scratch[2] <= p->scratch[3];
}
static int duration_ticks(double duration, unsigned short *out)
{
    double ticks = duration * 30.0;
    if (!_finite(ticks) || ticks < -9223372036854775808.0 || ticks >= 9223372036854775808.0)
        return 0;
    *out = (unsigned short)(__int64)ticks;
    return 1;
}
#include "bh_actor_route_promote_core.inc"
int __cdecl bh_actor_route_promote(BHActorRouteContext *c, BHEntity *a, long incoming, long outgoing)
{
    BHActorRouteState s;
    BHPickupRoom *old, *current;
    BHWorldGeometry *world;
    const BHPortal *in, *out;
    BHEntity *next, *previous, *at;
    unsigned long n;
    BHActorRoutePromotionBinding binding;
    BHEntity saved;
    BHEntity *old_head, *new_head, *new_head_previous;
    if (!c || !a || !c->rooms || !c->room_count || !c->actor_limit)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(c, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(a, &s);
    old = room_binding(c, s.current);
    current = room_binding(c, s.next);
    world = s.next;
    if (!old || !current || !world->model || world->model->portal_count > 6 ||
        world->navigation.count != world->model->portal_count || incoming < 0 || outgoing < 0 ||
        (unsigned long)incoming >= world->navigation.count || (unsigned long)outgoing >= world->navigation.count)
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    in = world->navigation.portals + incoming;
    out = world->navigation.portals + outgoing;
    if (!out->neighbor || !room_binding(c, (BHWorldGeometry *)out->neighbor) || !portal_valid(in) || !portal_valid(out))
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    next = actor_link(a, 0xf4);
    previous = actor_link(a, 0xf8);
    if ((previous ? actor_link(previous, 0xf4) != a : old->actors != a) || (next && actor_link(next, 0xf8) != a))
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    at = old->actors;
    n = 0;
    while (at && at != a && n < c->actor_limit)
    {
        at = actor_link(at, 0xf4);
        ++n;
    }
    if (at != a || n >= c->actor_limit)
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    if (!vector_finite(&s.controls[3]) || !vector_finite(&world->minimum) || !vector_finite(&world->maximum) ||
        !_finite(s.u) || !_finite(s.v) || !_finite(s.speed) || s.speed <= 0 || !_finite(s.speed_scale) ||
        s.speed_scale <= 0)
        return fail(c, BH_ACTOR_ROUTE_NUMERICAL);
    /* Invalid numerical input remains transactional in this checked adapter.
       The complete valid core follows the original early list/ownership stores. */
    saved = *a;
    old_head = old->actors;
    new_head = current->actors;
    new_head_previous = new_head ? actor_link(new_head, 0xf8) : NULL;
    binding.context = c;
    binding.old_room = old;
    binding.room = current;
    binding.world = world;
    binding.incoming = in;
    binding.outgoing = out;
    if (!bh_actor_route_promote_core(a, &binding, incoming, outgoing))
    {
        *a = saved;
        old->actors = old_head;
        current->actors = new_head;
        if (previous) set_link(previous, 0xf4, a);
        if (next) set_link(next, 0xf8, a);
        if (new_head) set_link(new_head, 0xf8, new_head_previous);
        return fail(c, BH_ACTOR_ROUTE_NUMERICAL);
    }
#ifdef DEBUGGING
    if (c->promotion_capture)
    {
        bh_actor_route_read(a, &s);
        /* Passive diagnostic consumer reads the original input image. */
        c->promotion_capture(&saved, &s, world, incoming, outgoing);
    }
#endif
    c->error = BH_ACTOR_ROUTE_OK;
    return 1;
}
static __inline unsigned long random_next(unsigned long state)
{
    return ((state * 0xe3570000UL) | ((state * 0xe357UL) >> 16)) ^ state;
}
#include "bh_actor_route_advance_core.inc"

int __cdecl bh_actor_route_advance(BHActorRouteContext *c, BHEntity *a)
{
    BHActorRouteState s;
    BHActorRouteAdvanceBinding binding;
    BHActorExitRoomImage image;
    BHActorExitNeighborImage neighbors[6];
    const BHWorldGeometry *room, *neighbor;
    unsigned long i, j, count;
    long incoming = -1;
    if (!c || !a || !c->rooms || !c->room_count)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    if (!bh_math_environment_supported())
        return fail(c, BH_ACTOR_ROUTE_NUMERICAL);
    bh_actor_route_read(a, &s);
    if (s.sound_frames == 1 && s.sound != BH_SOUND_INVALID && !c->sound)
        return fail(c, BH_ACTOR_ROUTE_ARGUMENT);
    room = s.next;
    if (!room || !room->model || room->model->portal_count > 6 || room->navigation.count != room->model->portal_count)
    {
        route_advance_expire_sound(c, a);
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    count = room->navigation.count;
    /* The guided owner has no valid incoming portal when count is zero. */
    if (!count && (room->query_flags & 8))
    {
        route_advance_expire_sound(c, a);
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    for (i = 0; i < count; ++i)
        if ((const BHWorldGeometry *)room->navigation.portals[i].neighbor == s.current || incoming < 0)
            incoming = (long)i;
    if ((room->query_flags & 8) && (room->navigation.portals[incoming].direction > 6 ||
                                  room->navigation.portals[incoming].direction == 3))
    {
        route_advance_expire_sound(c, a);
        return fail(c, BH_ACTOR_ROUTE_TOPOLOGY);
    }
    image.count_record = (int)count;
    *(unsigned long *)((char *)&image + 0x114) = room->query_flags;
    binding.context = c;
    binding.room = &image;
    binding.current_identity = s.current ? (void *)&image : NULL;
    for (i = 0; i < count; ++i)
    {
        image.portals[i] = room->navigation.portals[i];
        neighbor = (const BHWorldGeometry *)image.portals[i].neighbor;
        if (neighbor)
        {
            for (j = 0; j < i; ++j)
                if (room->navigation.portals[j].neighbor == room->navigation.portals[i].neighbor)
                    break;
            if (j == i)
            {
                neighbors[i].flags = neighbor->query_flags;
                image.portals[i].neighbor = (BHPortalNode *)(neighbors + i);
            }
            else
                image.portals[i].neighbor = image.portals[j].neighbor;
            if (neighbor == s.current)
                binding.current_identity = image.portals[i].neighbor;
        }
    }
    return bh_actor_route_advance_core(a, &binding);
}
