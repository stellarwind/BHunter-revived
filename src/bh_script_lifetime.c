#include "bh_script_lifetime.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char
    layout[sizeof(void *) == 4 && offsetof(BHEntity, origin) == 0x48 && offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
/* Offsets below are original fields. Only the400-byte payload offset used for
   the persistent source context is private; +52 is the existing timer binding. */
static void get(BHEntity *e, unsigned long offset, void *out, unsigned long n)
{
    memcpy(out, (unsigned char *)e + offset, n);
}
static void put(BHEntity *e, unsigned long offset, const void *value, unsigned long n)
{
    memcpy((unsigned char *)e + offset, value, n);
}
static BHScriptLifetimeContext *context(BHEntity *e)
{
    BHScriptLifetimeContext *c;
    memcpy(&c, e->payload + 400, 4);
    return c;
}
static long loaded(BHEntity *e)
{
    long v;
    get(e, 0xf4, &v, 4);
    return v;
}
static BHEntity *actor(BHEntity *e)
{
    BHEntity *v;
    get(e, 0xf0, &v, 4);
    return v;
}
static unsigned long sound(BHEntity *e)
{
    unsigned long v;
    get(e, 0xfc, &v, 4);
    return v;
}
static void release_entity(BHEntity *e)
{
    bh_entity_release(context(e)->timers.pool, e);
}
static void stop(BHEntity *e)
{
    bh_sound_stop(context(e)->sounds, sound(e));
}
static void defer(BHEntity *e)
{
    e->step = bh_script_timer_delete_step();
}
int __cdecl bh_script_lifetime_bind(BHScriptLifetimeContext *c, BHEntity *e, const BHScriptLifetimeState *s,
                                    BHEntityCallback query)
{
    BHScriptTimerContext *timer;
    if (!c || !c->timers.pool || !c->sounds || !e || !s)
        return 0;
    if (!bh_entity_is_active(c->timers.pool, e))
        return 0;
    timer = &c->timers;
    memcpy(e->payload + 52, &timer, 4);
    memcpy(e->payload + 400, &c, 4);
    put(e, 0xf0, &s->actor, 4);
    put(e, 0xf4, &s->script_loaded, 4);
    put(e, 0xfc, &s->sound, 4);
    put(e, 0x104, &s->mission_resource, 4);
    put(e, 0x128, &s->countdown, 4);
    put(e, 0x130, &s->delta, 12);
    put(e, 0x180, &s->mission_countdown, 4);
    return bh_entity_set_query(c->timers.pool, e, query);
}
int __cdecl bh_script_lifetime_bind_context(BHScriptLifetimeContext *c, BHEntity *e)
{
    BHScriptTimerContext *timer;
    if (!c || !c->timers.pool || !c->sounds || !e || !bh_entity_is_active(c->timers.pool, e))
        return 0;
    timer = &c->timers;
    memcpy(e->payload + 52, &timer, 4);
    memcpy(e->payload + 400, &c, 4);
    return 1;
}
int __cdecl bh_script_lifetime_read(BHEntity *e, BHScriptLifetimeState *s)
{
    if (!e || !s)
        return 0;
    s->actor = actor(e);
    s->script_loaded = loaded(e);
    s->sound = sound(e);
    get(e, 0x104, &s->mission_resource, 4);
    get(e, 0x128, &s->countdown, 4);
    get(e, 0x130, &s->delta, 12);
    get(e, 0x180, &s->mission_countdown, 4);
    return 1;
}
void __cdecl bh_script_unloaded_query(BHEntity *e, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 20 && !loaded(e))
        release_entity(e);
}
/* Original FCOM precedes FSTP. Test unrounded subtraction and include unordered
   (AH mask41), so NaN expires too. Ordinary <= would silently change this. */
static int elapsed(BHEntity *e, unsigned long offset)
{
    float old;
    double value;
    get(e, offset, &old, 4);
    value = (double)old - (1.0 / 30.0);
    bh_round_float(&old, value);
    put(e, offset, &old, 4);
    return !(value > 0.0);
}
void __cdecl bh_script_sound_expiry(BHEntity *e)
{
    if (elapsed(e, 0x128))
    {
        stop(e);
        release_entity(e);
    }
}
void __cdecl bh_script_motion_query(BHEntity *e, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    BHEntity *a;
    BHVec3 d;
    if (p->event == 4 || (!loaded(e) && p->event == 20))
    {
        release_entity(e);
        return;
    }
    if (p->event == 10 && p->actor == actor(e))
    {
        a = actor(e);
        get(e, 0x130, &d, 12);
        bh_round_float(&a->origin.x, (double)a->origin.x - d.x);
        bh_round_float(&a->origin.y, (double)a->origin.y - d.y);
        bh_round_float(&a->origin.z, (double)a->origin.z - d.z);
        defer(e);
    }
}
void __cdecl bh_script_sensor_query(BHEntity *e, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 4 || (!loaded(e) && p->event == 20))
    {
        release_entity(e);
        return;
    }
    if (p->event == 10 && p->actor == actor(e))
        defer(e);
}
void __cdecl bh_script_sound_query(BHEntity *e, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 4 || (!loaded(e) && p->event == 20))
    {
        stop(e);
        release_entity(e);
    }
}
void __cdecl bh_script_sound_actor_query(BHEntity *e, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 4 || p->event == 20)
    {
        stop(e);
        release_entity(e);
        return;
    }
    if (p->event == 9 && p->actor == actor(e))
    {
        stop(e);
        defer(e);
    }
}
void __cdecl bh_mission_sound_tick(BHEntity *e)
{
    unsigned long resource;
    if (elapsed(e, 0x180))
    {
        get(e, 0x104, &resource, 4);
        bh_sound_request(context(e)->sounds, resource, 1, 1, NULL, NULL);
        release_entity(e);
    }
}
static BHEntity *spawn(BHScriptLifetimeContext *c, unsigned long resource)
{
    BHEntity *e;
    BHScriptTimerContext *timer;
    float countdown = 4;
    if (!c || !c->timers.pool || !c->sounds)
        return NULL;
    e = bh_entity_acquire(c->timers.pool);
    if (!e)
        return NULL;
    timer = &c->timers;
    memcpy(e->payload + 52, &timer, 4);
    memcpy(e->payload + 400, &c, 4);
    put(e, 0x180, &countdown, 4);
    put(e, 0x104, &resource, 4);
    e->step = bh_mission_sound_tick;
    return e;
}
BHEntity *__cdecl bh_mission_sound_first(BHScriptLifetimeContext *c)
{
    return c ? spawn(c, c->mission_sound[0]) : NULL;
}
BHEntity *__cdecl bh_mission_sound_second(BHScriptLifetimeContext *c)
{
    return c ? spawn(c, c->mission_sound[1]) : NULL;
}
