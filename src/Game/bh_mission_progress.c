#include "bh_mission_progress.h"
#include <string.h>
static_assert(sizeof(short) == 2 && sizeof(long) == 4 && sizeof(void *) == 4, "mission abi");
BHEntity *bh_mission_progress(BHMissionProgressContext *context, long progression)
{
    short old;
    unsigned short low;
    BHEntity *cue_entity;
    BHScriptLifetimeState cue_state;
    if (!context || !context->config || !context->path || !context->lifetime ||
        !context->lifetime->timers.pool || !context->lifetime->sounds)
    {
        if (context)
            context->error = 1;
        return NULL;
    }
    context->error = 0;
    context->save_result = -1;
    memcpy(&old, &context->config->values.unlocked_mission, 2);
    if ((long)old < progression)
    {
        low = (unsigned short)(unsigned long)progression;
        memcpy(&context->config->values.unlocked_mission, &low, 2);
        ++context->save_attempts;
        context->save_result = bh_config_save(context->config, context->path);
        if (!context->save_result)
            context->error = 2;
    }
    cue_entity = bh_entity_acquire(context->lifetime->timers.pool);
    if (!cue_entity)
    {
        context->error = 3;
        return NULL;
    }
    /* Bind actual lifetime context without overwriting unrelated recycled words. */
    bh_script_lifetime_read(cue_entity, &cue_state);
    cue_state.mission_countdown = 4;
    cue_state.mission_resource = progression < 1 ? context->first_sound : context->unlocked_sound;
    if (!bh_script_lifetime_bind(context->lifetime, cue_entity, &cue_state, NULL))
    {
        context->error = 4;
        bh_entity_release(context->lifetime->timers.pool, cue_entity);
        return NULL;
    }
    cue_entity->step = bh_mission_sound_tick;
    return cue_entity;
}
