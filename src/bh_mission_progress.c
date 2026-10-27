#include "bh_mission_progress.h"
#include <string.h>
typedef char mission_abi[sizeof(short) == 2 && sizeof(long) == 4 && sizeof(void *) == 4 ? 1 : -1];
BHEntity *__cdecl bh_mission_progress(BHMissionProgressContext *c, long progression)
{
    short old;
    unsigned short low;
    BHEntity *e;
    BHScriptLifetimeState s;
    if (!c || !c->config || !c->path || !c->lifetime || !c->lifetime->timers.pool || !c->lifetime->sounds)
    {
        if (c)
            c->error = 1;
        return NULL;
    }
    c->error = 0;
    c->save_result = -1;
    memcpy(&old, c->config->settings + 2, 2);
    if ((long)old < progression)
    {
        low = (unsigned short)(unsigned long)progression;
        memcpy(c->config->settings + 2, &low, 2);
        ++c->save_attempts;
        c->save_result = bh_config_save(c->config, c->path);
        if (!c->save_result)
            c->error = 2;
    }
    e = bh_entity_acquire(c->lifetime->timers.pool);
    if (!e)
    {
        c->error = 3;
        return NULL;
    }
    /* Bind actual lifetime context without overwriting unrelated recycled words. */
    bh_script_lifetime_read(e, &s);
    s.mission_countdown = 4;
    s.mission_resource = progression < 1 ? c->first_sound : c->unlocked_sound;
    if (!bh_script_lifetime_bind(c->lifetime, e, &s, NULL))
    {
        c->error = 4;
        bh_entity_release(c->lifetime->timers.pool, e);
        return NULL;
    }
    e->step = bh_mission_sound_tick;
    return e;
}
