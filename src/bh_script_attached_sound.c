#include "bh_script_attached_sound.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char field_layout[sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
static void get(BHEntity *entity, unsigned long at, void *out)
{
    memcpy(out, (unsigned char *)entity + at, 4);
}
static void put(BHEntity *entity, unsigned long at, const void *value)
{
    memcpy((unsigned char *)entity + at, value, 4);
}
static BHScriptLifetimeContext *context(BHEntity *entity)
{
    BHScriptLifetimeContext *ctx;
    memcpy(&ctx, entity->payload + 400, 4);
    return ctx;
}
static const BHScriptTimerActor *actor_record(BHScriptLifetimeContext *ctx, BHEntity *actor)
{
    return bh_script_timer_actor(&ctx->timers, actor);
}
int __cdecl bh_script_attached_sound_read(BHEntity *entity, BHScriptAttachedSoundState *state)
{
    if (!entity || !state)
        return 0;
    get(entity, 0xf0, &state->actor);
    get(entity, 0xf4, &state->script_loaded);
    get(entity, 0xfc, &state->handle);
    get(entity, 0x100, &state->resource);
    get(entity, 0x104, &state->level);
    get(entity, 0x108, &state->target);
    return 1;
}
int __cdecl bh_script_attached_sound_bind(BHScriptLifetimeContext *ctx, BHEntity *entity,
                                          const BHScriptAttachedSoundState *state)
{
    BHScriptLifetimeState base;
    if (!ctx || !entity || !state || !ctx->sounds || !actor_record(ctx, state->actor))
        return 0;
    /* Reuse the persistent real context/query installer, retaining all fields
       outside its specified binding footprint by restoring its unused writes. */
    {
        unsigned char retained[24];
        memcpy(retained, (unsigned char *)entity + 0x128, 4);
        memcpy(retained + 4, (unsigned char *)entity + 0x130, 12);
        memcpy(retained + 16, (unsigned char *)entity + 0x180, 4);
        memset(&base, 0, sizeof(base));
        base.actor = state->actor;
        base.script_loaded = state->script_loaded;
        base.sound = state->handle;
        if (!bh_script_lifetime_bind(ctx, entity, &base, bh_script_sound_actor_query))
            return 0;
        memcpy((unsigned char *)entity + 0x128, retained, 4);
        memcpy((unsigned char *)entity + 0x130, retained + 4, 12);
        memcpy((unsigned char *)entity + 0x180, retained + 16, 4);
    }
    put(entity, 0x100, &state->resource);
    put(entity, 0x104, &state->level);
    put(entity, 0x108, &state->target);
    entity->step = bh_script_attached_sound_tick;
    return 1;
}
void __cdecl bh_script_attached_sound_tick(BHEntity *entity)
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
        put(entity, 0x104, &state.level);
        if (state.level == 1)
        {
            state.handle = bh_sound_request(ctx->sounds, state.resource, 0.066666670143604278564453125f, 1,
                                            &state.actor->position, &zero);
            put(entity, 0xfc, &state.handle);
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
        put(entity, 0x104, &state.level);
        if (state.level == 0)
        {
            bh_sound_stop(ctx->sounds, state.handle);
            state.handle = BH_SOUND_INVALID;
            put(entity, 0xfc, &state.handle);
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
            put(entity, 0x108, &state.target);
        }
    }
    else if (!actor->active)
    {
        state.target = 0;
        put(entity, 0x108, &state.target);
    }
}
