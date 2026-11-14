#include "bh_script_motion.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && offsetof(BHEntity, origin) == 0x48,
              "motion layout");
static BHScriptMotionContext *context(BHEntity *entity)
{
    BHScriptMotionContext *ctx;
    memcpy(&ctx, &entity->bindings.script_motion, 4);
    return ctx;
}
static int live(BHEntityPool *pool, const BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
int bh_script_motion_actor_read(const BHEntity *entity, BHScriptMotionActor *script_motion_actor)
{
    if (!entity || !script_motion_actor)
        return 0;
    memcpy(&script_motion_actor->angles, &entity->euler, 12);
    memcpy(&script_motion_actor->minimum, &entity->placement.minimum, 12);
    memcpy(&script_motion_actor->maximum, &entity->placement.maximum, 12);
    memcpy(&script_motion_actor->local_to_world, &entity->placement.transform, 48);
    memcpy(&script_motion_actor->world_to_local, &entity->placement.inverse, 48);
    memcpy(&script_motion_actor->attachments, &entity->placement.attachments, 4);
    return 1;
}
int bh_script_motion_actor_write(BHEntity *entity, const BHScriptMotionActor *script_motion_actor)
{
    if (!entity || !script_motion_actor)
        return 0;
    memcpy(&entity->euler, &script_motion_actor->angles, 12);
    memcpy(&entity->placement.minimum, &script_motion_actor->minimum, 12);
    memcpy(&entity->placement.maximum, &script_motion_actor->maximum, 12);
    memcpy(&entity->placement.transform, &script_motion_actor->local_to_world, 48);
    memcpy(&entity->placement.inverse, &script_motion_actor->world_to_local, 48);
    memcpy(&entity->placement.attachments, &script_motion_actor->attachments, 4);
    return 1;
}
BHEntity *bh_script_motion_attachment_next(const BHEntity *entity)
{
    BHEntity *next = NULL;
    if (entity)
        memcpy(&next, &entity->script_lifetime.attachment_next, 4);
    return next;
}
void bh_script_motion_attachment_link(BHEntity *entity, BHEntity *next)
{
    if (entity)
        memcpy(&entity->script_lifetime.attachment_next, &next, 4);
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
    memcpy(&entity->bindings.script_motion, &ctx, 4);
    entity->step = rotate ? bh_script_rotate_tick : bh_script_translate_tick;
    return 1;
}
static void add(BHVec3 *vector, const BHVec3 *delta)
{
    bh_round_float(&vector->x, (double)vector->x + delta->x);
    bh_round_float(&vector->y, (double)vector->y + delta->y);
    bh_round_float(&vector->z, (double)vector->z + delta->z);
}
static void publish(BHScriptMotionContext *ctx, BHEntity *entity)
{
    unsigned long i;
    BHScriptMotionActor a;
    const BHScriptMotionBinding *script_motion_binding;
    for (i = 0; ctx->bindings && i < ctx->binding_count; ++i)
    {
        script_motion_binding = ctx->bindings + i;
        if (script_motion_binding->entity != entity)
            continue;
        if (!script_motion_binding->world)
        {
            ctx->error = BH_MOTION_ARGUMENT;
            return;
        }
        bh_script_motion_actor_read(entity, &a);
        script_motion_binding->world->position = entity->position;
        script_motion_binding->world->transform = a.local_to_world;
        script_motion_binding->world->inverse = a.world_to_local;
        script_motion_binding->world->minimum = a.minimum;
        script_motion_binding->world->maximum = a.maximum;
        if (script_motion_binding->draw_angles)
            *script_motion_binding->draw_angles = a.angles;
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
    memcpy(&entity->script_lifetime.countdown, &countdown, 4);
    return !(next > 0);
}
void bh_script_translate_tick(BHEntity *entity)
{
    BHScriptMotionContext *ctx = context(entity);
    BHScriptLifetimeState state;
    BHScriptMotionActor a;
    BHEntity *parent;
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
    for (parent = attachments(ctx, state.actor, a.attachments); parent; parent = bh_script_motion_attachment_next(parent))
    {
        add(&parent->position, &delta);
        publish(ctx, parent);
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
    memcpy(&angles, &state.actor->euler, 12);
    add(&angles, &state.delta);
    wrap(&angles.x);
    wrap(&angles.y);
    wrap(&angles.z);
    memcpy(&state.actor->euler, &angles, 12);
    publish(ctx, state.actor);
    if (expire(entity, state.countdown))
        bh_entity_release(ctx->lifetime->timers.pool, entity);
}
