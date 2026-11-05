#include "bh_script_lifetime.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char
    layout[sizeof(void *) == 4 && offsetof(BHEntity, origin) == 0x48 && offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
/* Offsets below are original fields. Only the400-byte payload offset used for
   the persistent source context is private; +52 is the existing timer binding. */
static void get(BHEntity *entity, unsigned long offset, void *out, unsigned long n)
{
    memcpy(out, (unsigned char *)entity + offset, n);
}
static void put(BHEntity *entity, unsigned long offset, const void *value, unsigned long n)
{
    memcpy((unsigned char *)entity + offset, value, n);
}
static BHScriptLifetimeContext *context(BHEntity *entity)
{
    BHScriptLifetimeContext *ctx;
    memcpy(&ctx, entity->payload + 400, 4);
    return ctx;
}
static long loaded(BHEntity *entity)
{
    long v;
    get(entity, 0xf4, &v, 4);
    return v;
}
static BHEntity *actor(BHEntity *entity)
{
    BHEntity *v;
    get(entity, 0xf0, &v, 4);
    return v;
}
static unsigned long sound(BHEntity *entity)
{
    unsigned long v;
    get(entity, 0xfc, &v, 4);
    return v;
}
static void release_entity(BHEntity *entity)
{
    bh_entity_release(context(entity)->timers.pool, entity);
}
static void stop(BHEntity *entity)
{
    bh_sound_stop(context(entity)->sounds, sound(entity));
}
static void defer(BHEntity *entity)
{
    entity->step = bh_script_timer_delete_step();
}
int __cdecl bh_script_lifetime_bind(BHScriptLifetimeContext *ctx, BHEntity *entity, const BHScriptLifetimeState *state,
                                    BHEntityCallback query)
{
    BHScriptTimerContext *timer;
    if (!ctx || !ctx->timers.pool || !ctx->sounds || !entity || !state)
        return 0;
    if (!bh_entity_is_active(ctx->timers.pool, entity))
        return 0;
    timer = &ctx->timers;
    memcpy(entity->payload + 52, &timer, 4);
    memcpy(entity->payload + 400, &ctx, 4);
    put(entity, 0xf0, &state->actor, 4);
    put(entity, 0xf4, &state->script_loaded, 4);
    put(entity, 0xfc, &state->sound, 4);
    put(entity, 0x104, &state->mission_resource, 4);
    put(entity, 0x128, &state->countdown, 4);
    put(entity, 0x130, &state->delta, 12);
    put(entity, 0x180, &state->mission_countdown, 4);
    return bh_entity_set_query(ctx->timers.pool, entity, query);
}
int __cdecl bh_script_lifetime_bind_context(BHScriptLifetimeContext *ctx, BHEntity *entity)
{
    BHScriptTimerContext *timer;
    if (!ctx || !ctx->timers.pool || !ctx->sounds || !entity || !bh_entity_is_active(ctx->timers.pool, entity))
        return 0;
    timer = &ctx->timers;
    memcpy(entity->payload + 52, &timer, 4);
    memcpy(entity->payload + 400, &ctx, 4);
    return 1;
}
int __cdecl bh_script_lifetime_read(BHEntity *entity, BHScriptLifetimeState *state)
{
    if (!entity || !state)
        return 0;
    state->actor = actor(entity);
    state->script_loaded = loaded(entity);
    state->sound = sound(entity);
    get(entity, 0x104, &state->mission_resource, 4);
    get(entity, 0x128, &state->countdown, 4);
    get(entity, 0x130, &state->delta, 12);
    get(entity, 0x180, &state->mission_countdown, 4);
    return 1;
}
void __cdecl bh_script_unloaded_query(BHEntity *entity, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 20 && !loaded(entity))
        release_entity(entity);
}
/* Original FCOM precedes FSTP. Test unrounded subtraction and include unordered
   (AH mask41), so NaN expires too. Ordinary <= would silently change this. */
static int elapsed(BHEntity *entity, unsigned long offset)
{
    float old;
    double value;
    get(entity, offset, &old, 4);
    value = (double)old - (1.0 / 30.0);
    bh_round_float(&old, value);
    put(entity, offset, &old, 4);
    return !(value > 0.0);
}
void __cdecl bh_script_sound_expiry(BHEntity *entity)
{
    if (elapsed(entity, 0x128))
    {
        stop(entity);
        release_entity(entity);
    }
}
void __cdecl bh_script_motion_query(BHEntity *entity, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    BHEntity *a;
    BHVec3 d;
    if (p->event == 4 || (!loaded(entity) && p->event == 20))
    {
        release_entity(entity);
        return;
    }
    if (p->event == 10 && p->actor == actor(entity))
    {
        a = actor(entity);
        get(entity, 0x130, &d, 12);
        bh_round_float(&a->origin.x, (double)a->origin.x - d.x);
        bh_round_float(&a->origin.y, (double)a->origin.y - d.y);
        bh_round_float(&a->origin.z, (double)a->origin.z - d.z);
        defer(entity);
    }
}
void __cdecl bh_script_sensor_query(BHEntity *entity, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 4 || (!loaded(entity) && p->event == 20))
    {
        release_entity(entity);
        return;
    }
    if (p->event == 10 && p->actor == actor(entity))
        defer(entity);
}
void __cdecl bh_script_sound_query(BHEntity *entity, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 4 || (!loaded(entity) && p->event == 20))
    {
        stop(entity);
        release_entity(entity);
    }
}
void __cdecl bh_script_sound_actor_query(BHEntity *entity, void *packet)
{
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    if (p->event == 4 || p->event == 20)
    {
        stop(entity);
        release_entity(entity);
        return;
    }
    if (p->event == 9 && p->actor == actor(entity))
    {
        stop(entity);
        defer(entity);
    }
}
void __cdecl bh_mission_sound_tick(BHEntity *entity)
{
    unsigned long resource;
    if (elapsed(entity, 0x180))
    {
        get(entity, 0x104, &resource, 4);
        bh_sound_request(context(entity)->sounds, resource, 1, 1, NULL, NULL);
        release_entity(entity);
    }
}
static BHEntity *spawn(BHScriptLifetimeContext *ctx, unsigned long resource)
{
    BHEntity *entity;
    BHScriptTimerContext *timer;
    float countdown = 4;
    if (!ctx || !ctx->timers.pool || !ctx->sounds)
        return NULL;
    entity = bh_entity_acquire(ctx->timers.pool);
    if (!entity)
        return NULL;
    timer = &ctx->timers;
    memcpy(entity->payload + 52, &timer, 4);
    memcpy(entity->payload + 400, &ctx, 4);
    put(entity, 0x180, &countdown, 4);
    put(entity, 0x104, &resource, 4);
    entity->step = bh_mission_sound_tick;
    return entity;
}
BHEntity *__cdecl bh_mission_sound_first(BHScriptLifetimeContext *ctx)
{
    return ctx ? spawn(ctx, ctx->mission_sound[0]) : NULL;
}
BHEntity *__cdecl bh_mission_sound_second(BHScriptLifetimeContext *ctx)
{
    return ctx ? spawn(ctx, ctx->mission_sound[1]) : NULL;
}
