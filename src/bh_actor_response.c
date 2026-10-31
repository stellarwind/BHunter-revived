#include "bh_actor_response.h"
#include "bh_actor_vehicle_step.h"
#include "bh_camera_curve.h"
#include "bh_target_selection.h"
#include "bh_remote_prediction.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* Source-context adapters for the original one-argument dependency ABI.
   They live outside the matched owner and retain its checked caller's
   early-return behavior on a source dependency/lifetime guard failure. */
static BHActorAttackContext *vehicle_tick_context(BHEntity *e)
{
    BHActorAttackContext *c;
    memcpy(&c, e->payload + 596, 4);
    return c;
}
void __cdecl bh_actor_attack_bound(BHEntity *e)
{
    BHActorAttackContext *c = vehicle_tick_context(e);
    if (!bh_actor_attack(c, e) && c && c->step_abort)
        longjmp(*(jmp_buf *)c->step_abort, 1);
}
void __cdecl bh_actor_pursuit_bound(BHEntity *e)
{
    BHActorAttackContext *c = vehicle_tick_context(e);
    if (!bh_actor_pursuit_route(c->pursuit, e) && c->step_abort)
        longjmp(*(jmp_buf *)c->step_abort, 1);
}
/* These are source field views, not distinct original owners. Emit the direct
   loads/stores used throughout 0043a1d0/00439910. */
static __inline unsigned long word(BHEntity *e, unsigned long o)
{
    unsigned long v;
    memcpy(&v, (char *)e + o, 4);
    return v;
}
static __inline void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static __inline float number(BHEntity *e, unsigned long o)
{
    float v;
    memcpy(&v, (char *)e + o, 4);
    return v;
}
static __inline void real(BHEntity *e, unsigned long o, float v)
{
    memcpy((char *)e + o, &v, 4);
}
static __inline short short_value(BHEntity *e, unsigned long o)
{
    short v;
    memcpy(&v, (char *)e + o, 2);
    return v;
}
static __inline BHActorResponseContext *context(BHEntity *e)
{
    BHActorResponseContext *c;
    memcpy(&c, e->payload + 588, 4);
    return c;
}
static int live(BHActorResponseContext *c, BHEntity *e)
{
    return bh_entity_is_active(c->vehicles->pool, e);
}
static int unlink_actor(BHActorResponseContext *c, BHEntity *e)
{
    BHActorRouteState s;
    BHPickupRoom *r = 0;
    BHEntity *p, *n;
    unsigned long i;
    bh_actor_route_read(e, &s);
    for (i = 0; i < c->damage->route->room_count; ++i)
        if (c->damage->route->rooms[i].world == s.current)
        {
            r = c->damage->route->rooms + i;
            break;
        }
    if (!r || !c->vehicles->count)
    {
        c->error = 2;
        return 0;
    }
    p = (BHEntity *)word(e, 0xf8);
    n = (BHEntity *)word(e, 0xf4);
    if ((p ? (BHEntity *)word(p, 0xf4) : r->actors) != e || (n && (BHEntity *)word(n, 0xf8) != e))
    {
        c->error = 2;
        return 0;
    }
    if (p)
        setword(p, 0xf4, (unsigned long)n);
    else
        r->actors = n;
    if (n)
        setword(n, 0xf8, (unsigned long)p);
    p = (BHEntity *)word(e, 0xec);
    n = (BHEntity *)word(e, 0xf0);
    if ((p ? (BHEntity *)word(p, 0xf0) : c->vehicles->head) != e || (n && (BHEntity *)word(n, 0xec) != e))
    {
        c->error = 2;
        return 0;
    }
    if (p)
        setword(p, 0xf0, (unsigned long)n);
    else
        c->vehicles->head = n;
    if (n)
        setword(n, 0xec, (unsigned long)p);
    --c->vehicles->count;
    return 1;
}
static void release_tick(BHEntity *e)
{
    BHActorResponseContext *c = context(e);
    if (!bh_actor_vehicle_release(c->vehicles, e))
        c->error = 2;
}
int __cdecl bh_actor_response_bind(BHActorResponseContext *c, BHEntity *e)
{
    if (!c || !c->damage || !c->damage->route || !c->attack || !c->vehicles || !c->vehicles->pool || !e ||
        c->damage->pool != c->vehicles->pool || c->vehicles->route != c->damage->route || !live(c, e))
        return 0;
    memcpy(e->payload + 588, &c, 4);
    return bh_entity_set_query(c->vehicles->pool, e, bh_actor_response_query);
}
#include "bh_actor_response_query_core.inc"
#include "bh_actor_response_motion.inc"
#include "bh_actor_response_smoke.inc"
#include "bh_actor_response_finish.inc"
void __cdecl bh_actor_response_query(BHEntity *e, void *packet)
{
    BHActorResponseContext *c = context(e);
    unsigned long event;
    if (!c || !packet || !live(c, e))
        return;
    c->error = 0;
    event = *(unsigned long *)packet;
    if (event == 0 || event == 1 || event == 3 || event == 15 || event == 16)
    {
        if (!bh_math_environment_supported() || number(e, 0x184) <= 0 || !_finite(number(e, 0x184)))
        {
            c->error = 3;
            return;
        }
    }
    bh_actor_response_query_core(e, packet, c);
}
void __cdecl bh_actor_response_tick(BHEntity *e)
{
    BHActorResponseContext *c = context(e);
    BHActorDamageContext *d;
    BHVec3 death_position;
    int smoke;
    if (!c || !live(c, e))
        return;
    c->error = 0;
    d = c->damage;
    if (!d->particles || !d->fragments || !d->targets || !d->loot_rows || !c->mission ||
        !bh_math_environment_supported())
    {
        c->error = 1;
        return;
    }
    if (!bh_actor_attack(c->attack, e))
    {
        c->error = 2;
        return;
    }
    if (number(e, 0x184) <= 0 || !number(e, 0x188))
    {
        c->error = 3;
        return;
    }
    if (!bh_actor_response_motion(e, c))
        return;
    smoke = bh_actor_response_route_smoke(e, c, &death_position);
    if (smoke < 0)
        return;
    bh_actor_response_finish(e, c, &death_position, smoke);
}
