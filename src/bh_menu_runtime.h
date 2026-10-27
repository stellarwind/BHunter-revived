#ifndef BH_MENU_RUNTIME_H
#define BH_MENU_RUNTIME_H
#include "bh_menu_actions.h"
#include "bh_menu_present.h"
#include "bh_menu_transition.h"
#include "bh_flyby_scene.h"
typedef struct BHMenuRuntime BHMenuRuntime;
typedef struct BHMenuRuntimeContext
{
    BHEntityPool *pool;
    BHInputSnapshot *snapshot;
    BHSoundQueue *sound;
    BHNetworkSessions *sessions;
    const BHProviderRecord **provider;
    BHMenuTransition *transition;
    BHFlybyScene *flyby;
    BHMenuRuntime *root, *ship_menus[2];
    long *mode, *paused, *waypoint, *input_changed, *stay;
    unsigned long move_sound, open_sound, slide_sound, press_sound, release_sound;
    unsigned long sounds, refreshes, camera_steps;
    int error;
} BHMenuRuntimeContext;
/* Typed original menu fields, row destinations and actual optional source
   callback identities. Active real text/key capture owners automatically block
   normal menu input, corresponding to original non-NULL commit+1f0.
   actions->row_actions is authoritative and remains live
   through callback mutations; optional edits use the existing typed owners.
   All menus/entity/input/row/scene/transition bindings outlive callbacks.
   Zero initialize source state; pending_selection=-1,back_selection=-1 and
   slide_voice=BH_SOUND_INVALID are the original inactive values. */
struct BHMenuRuntime
{
    BHMenuRuntimeContext *context;
    BHEntity *entity;
    BHMenuActionsRoot *actions;
    BHMenuActions *action_context;
    BHMenuEdits *edit_context;
    BHMenuMissionAction *edit_actions;
    BHMenuRuntime **destinations;
    BHPlayerInputList *input;
    BHPlayerInputState input_state;
    BHMenuPresent *present;
    unsigned long phase, slide_voice;
    long exiting, capture, input_blocked;
    long back_selection, pending_selection, waypoint;
    float push, distance, scroll;
};
/* Count-zero binding of real source callbacks to an existing pool entity. */
int __cdecl bh_menu_runtime_bind(BHMenuRuntime *);
void __cdecl bh_menu_runtime_step_entity(BHEntity *);
void __cdecl bh_menu_runtime_refresh_entity(BHEntity *, void *);
/* Whole0042b2e0. Consumes already cached input only: no device polling,
   messages, keyboard/mouse automation or game process. Requires at least one
   actionable row; source guards bound formerly unbounded selection loops.
   PC53 finite input/animation profile; PC24 unresolved. */
int __cdecl bh_menu_runtime_step(BHMenuRuntime *);
/* Whole0042f400. Return-to-menu resources, actual flyby state reset and fifty
   synchronous source camera-settling calls; no render/game loop execution.
   flyby must already be a real bound source scene with live table/ring/font.
   Requires at least eleven actual camera start records. */
int __cdecl bh_menu_runtime_return(BHMenuRuntimeContext *);
#endif
