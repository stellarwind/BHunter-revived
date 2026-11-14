#ifndef BH_MISSION_PROGRESS_H
#define BH_MISSION_PROGRESS_H
#include "../Config/bh_config.h"
#include "Scripts/bh_script_lifetime.h"
typedef struct BHMissionProgressContext
{
    BHConfig *config;
    const char *path;
    BHScriptLifetimeContext *lifetime;
    unsigned long first_sound, unlocked_sound, save_attempts;
    int save_result, error;
} BHMissionProgressContext;
/* Whole00422570: signed16 saved progression versus signed32 argument; save
   new low16 only when greater, then always attempt the real deferred cue.
   Save failure preserves the progression write and does not suppress cue.
   Borrowed config/path/lifetime/pool/sound owners survive until cue expires.
   Returns the acquired cue as source convenience; original return is void. */
BHEntity *bh_mission_progress(BHMissionProgressContext *, long progression);
#endif
