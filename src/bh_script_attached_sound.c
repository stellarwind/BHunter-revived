#include "bh_script_attached_sound.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4, "field layout");
static BHScriptLifetimeContext *context(BHEntity *entity)
{
    BHScriptLifetimeContext *ctx;
    memcpy(&ctx, &entity->bindings.script_lifetime, 4);
    return ctx;
}
static const BHScriptTimerActor *actor_record(BHScriptLifetimeContext *ctx, BHEntity *actor)
{
    return bh_script_timer_actor(&ctx->timers, actor);
}
int bh_script_attached_sound_read(BHEntity *entity, BHScriptAttachedSoundState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->actor, &entity->attached_sound.actor, 4);
    memcpy(&state->script_loaded, &entity->attached_sound.script_loaded, 4);
    memcpy(&state->handle, &entity->attached_sound.handle, 4);
    memcpy(&state->resource, &entity->attached_sound.resource, 4);
    memcpy(&state->level, &entity->attached_sound.level, 4);
    memcpy(&state->target, &entity->attached_sound.target, 4);
    return 1;
}
int bh_script_attached_sound_bind(BHScriptLifetimeContext *ctx, BHEntity *entity,
                                  const BHScriptAttachedSoundState *state)
{
    BHScriptLifetimeState base;
    if (!ctx || !entity || !state || !ctx->sounds || !actor_record(ctx, state->actor))
        return 0;
    /* Reuse the persistent real context/query installer, retaining all fields
       outside its specified binding footprint by restoring its unused writes. */
    {
        unsigned char retained[24];
        memcpy(retained, (char *)&entity->script_lifetime.countdown, 4);
        memcpy(retained + 4, (char *)&entity->script_lifetime.delta, 12);
        memcpy(retained + 16, (char *)&entity->script_lifetime.mission_countdown, 4);
        memset(&base, 0, sizeof(base));
        base.actor = state->actor;
        base.script_loaded = state->script_loaded;
        base.sound = state->handle;
        if (!bh_script_lifetime_bind(ctx, entity, &base, bh_script_sound_actor_query))
            return 0;
        memcpy((char *)&entity->script_lifetime.countdown, retained, 4);
        memcpy((char *)&entity->script_lifetime.delta, retained + 4, 12);
        memcpy((char *)&entity->script_lifetime.mission_countdown, retained + 16, 4);
    }
    memcpy(&entity->attached_sound.resource, &state->resource, 4);
    memcpy(&entity->attached_sound.level, &state->level, 4);
    memcpy(&entity->attached_sound.target, &state->target, 4);
    entity->step = bh_script_attached_sound_tick;
    return 1;
}
void bh_script_attached_sound_tick(BHEntity *entity)
{
    BHScriptAttachedSoundState state;
    BHScriptLifetimeContext *ctx = context(entity);
    const BHScriptTimerActor *actor;
    BHVec3 zero = {0, 0, 0};
    float gain;
    unsigned long next;
    bh_script_attached_sound_read(entity, &state);
    actor = actor_record(ctx, state.actor);
    if (!actor)
    {
        ctx->timers.error = BH_TIMER_ACTOR;
        return;
    }
    if (actor->owner)
        actor = actor->owner;
    if (state.level < state.target)
    {
        next = (unsigned long)state.level + 1UL;
        memcpy(&state.level, &next, 4);
        memcpy(&entity->attached_sound.level, &state.level, 4);
        if (state.level == 1)
        {
            state.handle = bh_sound_request(ctx->sounds, state.resource, 0.066666670143604278564453125f, 1,
                                            &state.actor->position, &zero);
            memcpy(&entity->attached_sound.handle, &state.handle, 4);
        }
        else
        {
            bh_round_float(&gain, (double)state.level * 0.06666666666666666574148081281236955);
            bh_sound_update(ctx->sounds, state.handle, gain, 1, &state.actor->position, &zero);
        }
    }
    else if (state.level > state.target)
    {
        next = (unsigned long)state.level - 1UL;
        memcpy(&state.level, &next, 4);
        memcpy(&entity->attached_sound.level, &state.level, 4);
        if (state.level == 0)
        {
            bh_sound_stop(ctx->sounds, state.handle);
            state.handle = BH_SOUND_INVALID;
            memcpy(&entity->attached_sound.handle, &state.handle, 4);
        }
        else
        {
            bh_round_float(&gain, (double)state.level * 0.06666666666666666574148081281236955);
            bh_sound_update(ctx->sounds, state.handle, gain, 1, &state.actor->position, &zero);
        }
    }
    if (state.handle == BH_SOUND_INVALID)
    {
        if (actor->active)
        {
            state.target = 15;
            memcpy(&entity->attached_sound.target, &state.target, 4);
        }
    }
    else if (!actor->active)
    {
        state.target = 0;
        memcpy(&entity->attached_sound.target, &state.target, 4);
    }
}
