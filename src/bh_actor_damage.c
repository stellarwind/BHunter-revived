#include "bh_actor_damage.h"
#include "bh_camera_curve.h"
#include "bh_remote_prediction.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static __inline float number(BHEntity *e, unsigned long off)
{
    float f;
    memcpy(&f, (char *)e + off, 4);
    return f;
}
static __inline unsigned long word(BHEntity *e, unsigned long off)
{
    unsigned long v;
    memcpy(&v, (char *)e + off, 4);
    return v;
}
static __inline void set_word(BHEntity *e, unsigned long off, unsigned long v)
{
    memcpy((char *)e + off, &v, 4);
}
static BHActorDamageContext *context(BHEntity *e)
{
    BHActorDamageContext *c;
    memcpy(&c, e->payload + 600, 4);
    return c;
}
static int live(BHActorDamageContext *c, BHEntity *e)
{
    return bh_entity_is_active(c->pool, e);
}
static void normal_tick(BHEntity *e)
{
    BHActorDamageContext *c = context(e);
    bh_actor_route_due(c->route, e);
}
static void release_tick(BHEntity *e)
{
    BHActorDamageContext *c = context(e);
    bh_actor_route_release(c->route, c->pool, e);
}
int __cdecl bh_actor_damage_bind(BHActorDamageContext *c, BHEntity *e)
{
    if (!c || !c->pool || !c->route || !c->route->sound || !e || !live(c, e))
        return 0;
    memcpy(e->payload + 600, &c, 4);
    return bh_entity_set_query(c->pool, e, bh_actor_damage_query);
}
static unsigned long random_step(unsigned long r)
{
    return ((r * 0xe357UL >> 16) | (r * 0xe3570000UL)) ^ r;
}
void __cdecl bh_actor_damage_death_velocity(const BHVec3 *after, const BHVec3 *at, BHVec3 *velocity)
{
    /* 00435bf9..00435c52 retains X, but spills Y/Z differences before
       reloading them for the common scale. Do not symmetrize these stores. */
    double x = (double)after->x - at->x;
    float y = stored((double)after->y - at->y);
    float z = stored((double)after->z - at->z);
    velocity->x = stored(x * 15);
    velocity->y = stored((double)y * 15);
    velocity->z = stored((double)z * 15);
}
#include "bh_actor_damage_query_core.inc"
#include "bh_actor_damage_tick_core.inc"
void __cdecl bh_actor_damage_tick(BHEntity *e)
{
    BHActorDamageContext *c = context(e);
    if (!c || !live(c, e) || !c->particles || !c->fragments || !c->targets || !c->loot_rows ||
        !bh_math_environment_supported())
    {
        if (c)
            c->error = BH_ACTOR_ROUTE_ARGUMENT;
        return;
    }
    if (!*(unsigned short *)((char *)e + 0x1e6))
    {
        c->error = BH_ACTOR_ROUTE_NUMERICAL;
        return;
    }
    bh_actor_damage_tick_core(e, c);
}
void __cdecl bh_actor_damage_query(BHEntity *e, void *packet)
{
    BHActorDamageContext *c = context(e);
    unsigned long event;
    unsigned short tick = 0;
    if (!c || !packet || !live(c, e))
        return;
    event = *(unsigned long *)packet;
    if (event == 1 || event == 3 || event == 16)
    {
        if (!bh_math_environment_supported() || !*(unsigned short *)((char *)e + 0x1e6))
        {
            c->error = BH_ACTOR_ROUTE_NUMERICAL;
            return;
        }
        tick = bh_actor_route_now(c->route);
    }
    bh_actor_damage_query_core(e, packet, c, tick);
}
