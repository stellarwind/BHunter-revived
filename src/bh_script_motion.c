#include "bh_script_motion.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char motion_layout
    [sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && offsetof(BHEntity, origin) == 0x48 ? 1 : -1];
static void get(const BHEntity *entity, unsigned long o, void *v, unsigned long n)
{
    memcpy(v, (const unsigned char *)entity + o, n);
}
static void put(BHEntity *entity, unsigned long o, const void *v, unsigned long n)
{
    memcpy((unsigned char *)entity + o, v, n);
}
static BHScriptMotionContext *context(BHEntity *entity)
{
    BHScriptMotionContext *ctx;
    memcpy(&ctx, entity->payload + 408, 4);
    return ctx;
}
static int live(BHEntityPool *pool, const BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
int bh_script_motion_actor_read(const BHEntity *entity, BHScriptMotionActor *s)
{
    if (!entity || !s)
        return 0;
    get(entity, 0x3c, &s->angles, 12);
    get(entity, 0x1b0, &s->minimum, 12);
    get(entity, 0x1bc, &s->maximum, 12);
    get(entity, 0x150, &s->local_to_world, 48);
    get(entity, 0x180, &s->world_to_local, 48);
    get(entity, 0x12c, &s->attachments, 4);
    return 1;
}
int bh_script_motion_actor_write(BHEntity *entity, const BHScriptMotionActor *s)
{
    if (!entity || !s)
        return 0;
    put(entity, 0x3c, &s->angles, 12);
    put(entity, 0x1b0, &s->minimum, 12);
    put(entity, 0x1bc, &s->maximum, 12);
    put(entity, 0x150, &s->local_to_world, 48);
    put(entity, 0x180, &s->world_to_local, 48);
    put(entity, 0x12c, &s->attachments, 4);
    return 1;
}
BHEntity *bh_script_motion_attachment_next(const BHEntity *entity)
{
    BHEntity *next = NULL;
    if (entity)
        get(entity, 0xec, &next, 4);
    return next;
}
void bh_script_motion_attachment_link(BHEntity *entity, BHEntity *next)
{
    if (entity)
        put(entity, 0xec, &next, 4);
}
static BHEntity *attachments(BHScriptMotionContext *ctx, BHEntity *entity, BHEntity *raw)
{
    unsigned long i;
    for (i = 0; ctx->bindings && i < ctx->binding_count; ++i)
        if (ctx->bindings[i].entity == entity && ctx->bindings[i].attachments_owner)
            return *ctx->bindings[i].attachments_owner;
    return raw;
}
static int valid(BHScriptMotionContext *ctx, BHEntity *entity, const BHScriptLifetimeState *state, int rotate)
{
    BHEntityPool *pool;
    BHEntity *p;
    BHScriptMotionActor a;
    unsigned long n = 0;
    if (!ctx || !ctx->lifetime || !ctx->lifetime->timers.pool || !entity || !state)
    {
        if (ctx)
            ctx->error = BH_MOTION_ARGUMENT;
        return 0;
    }
    pool = ctx->lifetime->timers.pool;
    if (entity == state->actor || !live(pool, entity) || !live(pool, state->actor))
    {
        ctx->error = BH_MOTION_LIFETIME;
        return 0;
    }
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_MOTION_NUMERICAL;
        return 0;
    }
    if (!rotate)
    {
        bh_script_motion_actor_read(state->actor, &a);
        for (p = attachments(ctx, state->actor, a.attachments); p; p = bh_script_motion_attachment_next(p))
        {
            if (++n > pool->used || p == state->actor || p == entity || !live(pool, p))
            {
                ctx->error = BH_MOTION_ATTACHMENTS;
                return 0;
            }
        }
    }
    ctx->error = BH_MOTION_OK;
    return 1;
}
int bh_script_motion_bind(BHScriptMotionContext *ctx, BHEntity *entity, const BHScriptLifetimeState *state, int rotate)
{
    if (!valid(ctx, entity, state, rotate))
        return 0;
    if (!bh_script_lifetime_bind(ctx->lifetime, entity, state,
                                 rotate ? bh_script_sensor_query : bh_script_motion_query))
    {
        ctx->error = BH_MOTION_ARGUMENT;
        return 0;
    }
    memcpy(entity->payload + 408, &ctx, 4);
    entity->step = rotate ? bh_script_rotate_tick : bh_script_translate_tick;
    return 1;
}
static void add(BHVec3 *v, const BHVec3 *d)
{
    bh_round_float(&v->x, (double)v->x + d->x);
    bh_round_float(&v->y, (double)v->y + d->y);
    bh_round_float(&v->z, (double)v->z + d->z);
}
static void publish(BHScriptMotionContext *ctx, BHEntity *entity)
{
    unsigned long i;
    BHScriptMotionActor a;
    const BHScriptMotionBinding *b;
    for (i = 0; ctx->bindings && i < ctx->binding_count; ++i)
    {
        b = ctx->bindings + i;
        if (b->entity != entity)
            continue;
        if (!b->world)
        {
            ctx->error = BH_MOTION_ARGUMENT;
            return;
        }
        bh_script_motion_actor_read(entity, &a);
        b->world->position = entity->position;
        b->world->transform = a.local_to_world;
        b->world->inverse = a.world_to_local;
        b->world->minimum = a.minimum;
        b->world->maximum = a.maximum;
        if (b->draw_angles)
            *b->draw_angles = a.angles;
        return;
    }
}
static int expire(BHEntity *entity, float countdown)
{
    double next;
    if (!(countdown > 0))
        return 0;
    next = (double)countdown - (1.0 / 30.0);
    bh_round_float(&countdown, next);
    put(entity, 0x128, &countdown, 4);
    return !(next > 0);
}
void bh_script_translate_tick(BHEntity *entity)
{
    BHScriptMotionContext *ctx = context(entity);
    BHScriptLifetimeState state;
    BHScriptMotionActor a;
    BHEntity *p;
    BHVec3 delta;
    bh_script_lifetime_read(entity, &state);
    if (!valid(ctx, entity, &state, 0))
        return;
    delta = state.delta;
    bh_script_motion_actor_read(state.actor, &a);
    add(&a.minimum, &delta);
    add(&a.maximum, &delta);
    add(&state.actor->position, &delta);
    bh_mat34_translate_world(delta.x, delta.y, delta.z, &a.local_to_world);
    bh_mat34_translate_local(-delta.x, -delta.y, -delta.z, &a.world_to_local);
    bh_script_motion_actor_write(state.actor, &a);
    publish(ctx, state.actor);
    for (p = attachments(ctx, state.actor, a.attachments); p; p = bh_script_motion_attachment_next(p))
    {
        add(&p->position, &delta);
        publish(ctx, p);
    }
    if (expire(entity, state.countdown))
    {
        bh_round_float(&state.actor->origin.x, (double)state.actor->origin.x - delta.x);
        bh_round_float(&state.actor->origin.y, (double)state.actor->origin.y - delta.y);
        bh_round_float(&state.actor->origin.z, (double)state.actor->origin.z - delta.z);
        bh_entity_release(ctx->lifetime->timers.pool, entity);
    }
}
/* Exact PE doubles00489498..004894b0, not float approximations and not fmod.
   One correction only. The low-side C0 branch includes unordered. */
static void wrap(float *v)
{
    if ((double)*v > 3.141592653589793)
        bh_round_float(v, (double)*v - 6.283185307179586);
    else if (!((double)*v >= -3.141592653589793))
        bh_round_float(v, (double)*v - (-6.283185307179586));
}
void bh_script_rotate_tick(BHEntity *entity)
{
    BHScriptMotionContext *ctx = context(entity);
    BHScriptLifetimeState state;
    BHVec3 angles;
    bh_script_lifetime_read(entity, &state);
    if (!valid(ctx, entity, &state, 1))
        return;
    get(state.actor, 0x3c, &angles, 12);
    add(&angles, &state.delta);
    wrap(&angles.x);
    wrap(&angles.y);
    wrap(&angles.z);
    put(state.actor, 0x3c, &angles, 12);
    publish(ctx, state.actor);
    if (expire(entity, state.countdown))
        bh_entity_release(ctx->lifetime->timers.pool, entity);
}
