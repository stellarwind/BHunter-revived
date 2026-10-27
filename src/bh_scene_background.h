#ifndef BH_SCENE_BACKGROUND_H
#define BH_SCENE_BACKGROUND_H
#include "bh_draw_submit.h"
/* Exact 48-byte records at original 004ce2b0. The two endpoints have Y=0. */
typedef struct BHSceneBackgroundSlot
{
    float elapsed, duration, delay, yaw, height;
    unsigned long model;
    BHVec3 start, end;
} BHSceneBackgroundSlot;
typedef struct BHSceneBackground
{
    BHSceneBackgroundSlot slots[8];
    BHDrawQueue *queue;
    unsigned long first_model, second_model, first_flare, second_flare;
} BHSceneBackground;
/* Source binding/owning adapter. Recovers startup's eight negative elapsed
   sentinels without admitting the larger 004474c0 initialization owner. */
int __cdecl bh_scene_background_init(BHSceneBackground *, BHDrawQueue *, unsigned long first_model,
                                     unsigned long second_model, unsigned long first_flare, unsigned long second_flare);
/* Complete 0041c020: one slot's random initialization, wait/recycle, fixed
   timestep parallax trajectory, model and two/six flame command production.
   Uses the real shared VC5 CRT rand state; full queues still consume the
   original random calls and update slot state. Returns 1 complete, 0 dropped
   command, -1 invalid source binding. Requires caller's PC53/nearest profile.
   Moderate finite trajectories covered; PC24/transcendental edges unresolved. */
int __cdecl bh_scene_background_step(BHSceneBackground *, unsigned long slot, const BHVec3 *eye);
#endif
