#ifndef BH_WEAPON_ARC_LIFECYCLE_H
#define BH_WEAPON_ARC_LIFECYCLE_H
#include "bh_weapon_common.h"
#include "bh_world_segment.h"

typedef enum BHWeaponArcLifecycleError {
    BH_WEAPON_ARC_LIFECYCLE_OK,BH_WEAPON_ARC_LIFECYCLE_ARGUMENT,
    BH_WEAPON_ARC_LIFECYCLE_CALLBACK,BH_WEAPON_ARC_LIFECYCLE_NUMERICAL,
    BH_WEAPON_ARC_LIFECYCLE_LIMIT,BH_WEAPON_ARC_LIFECYCLE_WORLD,
    BH_WEAPON_ARC_LIFECYCLE_SOUND
} BHWeaponArcLifecycleError;
typedef struct BHWeaponArcLifecycleContext {
    BHSoundQueue *sound;BHWorldSegmentContext *world;
    BHEntity **head_owner; /* live original004dfb28 storage */
    BHEntity *head; /* actual004dfb28 list, successor at actor+f0 */
    unsigned long activate_sound,release_sound,charge16_sound,charge51_sound;
    unsigned long limit,visited,queries,world_queries,requests,updates,stops,random_calls;
    BHWeaponArcLifecycleError error;
} BHWeaponArcLifecycleContext;
/* Complete004324a0. Exact weapon subset reset, one CRT rand/.8 level cue,
   then clear+108 AFTER synchronous real sound allocation. */
int __cdecl bh_weapon_arc_activate(BHWeaponArcLifecycleContext *,BHEntity *weapon,
    const BHEntity *player);
/* Complete00432510. Capture handle+100 before clearing target/charge, stop
   actual queued voice if nonsentinel, then clear+108 and set+100=-1. */
int __cdecl bh_weapon_arc_deactivate(BHWeaponArcLifecycleContext *,BHEntity *weapon);
/* Complete00432c60. Consume latch+108, acquire/retain actual queried target,
   update signed charge with32bit wrap and real sound thresholds/lifetimes.
   Player position+0c/velocity+48/aim+280 and shared weapon fields remain LIVE
   across all queries/sound allocation; no snapshot or invented callback.
   Acquire tests nearest projection<150 and cone<.1 (unordered also accepted)
   through real world query;
   retain allows projection150/cone storedfloat<=double.1, preserves original
   masked0/0 unordered cone acceptance. Zero trigger resets charge; losing a
   tracked target preserves charge. Cache+104 aliases actual shared storage.
   Borrowed actors/weapon/player/sound/world storage survives callbacks. Head
   is loaded once for acquisition; successor is read after query/world effects.
   Source guards diagnose absent methods, cycles, nonfinite fields and binding
   errors. Successful prior state/sound/query effects remain on later failure.
   PC53 nearest/masked moderate finite inputs; no Play or original execution. */
int __cdecl bh_weapon_arc_track(BHWeaponArcLifecycleContext *,BHEntity *weapon,
    const BHEntity *player);
#endif
