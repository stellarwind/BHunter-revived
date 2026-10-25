#ifndef BH_FLYBY_SCENE_H
#define BH_FLYBY_SCENE_H
#include "bh_flyby_camera.h"
#include "bh_scene_background.h"
#include "bh_scene_owner.h"
#include "bh_sound_queue.h"
#include "bh_input_evaluate.h"

typedef enum BHFlybySceneError {
    BH_FLYBY_SCENE_OK,BH_FLYBY_SCENE_ARGUMENT,BH_FLYBY_SCENE_BINDING,
    BH_FLYBY_SCENE_OWNER,BH_FLYBY_SCENE_QUEUE,BH_FLYBY_SCENE_PACKET
} BHFlybySceneError;

/* All game state is borrowed from source owners. Pointer fields corresponding
   to original globals remain live across synchronous callbacks and queries.
   The source-only registry below ties void entity callbacks to this explicit
   context; it never maps or invokes an original code address. */
typedef struct BHFlybyScene {
    BHEntity *camera_entity,**current_player;
    BHEntityPool *entities;
    BHDrawQueue *queue;
    BHSoundQueue *sound;
    BHSceneBackground *background;
    BHSceneOwner *scene;
    BHPlayerInputOwner *input_owner;
    const BHInputProfile *input_profile;
    BHPlayerCameraContext *player_camera;
    BHFlybyCameraContext *flyby_camera;
    BHFlybyCameraState *flyby_state;
    const BHPlayerCameraStart *starts;
    unsigned long start_count;
    const long *mode_count,*selected_waypoint,*scenario,*player_slot;
    const long *display_width,*display_height;
    const float *near_z;
    unsigned long *event_value,*mission_identity;
    unsigned long horizon_image,full_image,corner_left_image,corner_right_image;
    unsigned long scenario_resources[3];
    unsigned long attempted,queued,dropped,random_calls;
    BHEntity *bound_entity;
    unsigned long bound_generation;
    struct BHFlybyScene *registry_next;
    BHFlybySceneError error;
} BHFlybyScene;

/* Bind/unbind the exact live entity and generation to source callbacks. Bind
   installs the restored scene-refresh and camera-update callbacks. Unbind
   removes source callbacks only while the entity generation still matches. */
int __cdecl bh_flyby_scene_bind(BHFlybyScene *);
void __cdecl bh_flyby_scene_unbind(BHFlybyScene *);
/* Complete 00428f50, including render, input/player relocation, two live
   entity queries, and callback handoff. Queue-full drops are successful
   original mutations; missing live dependencies return 0 with error set. */
int __cdecl bh_flyby_scene_draw(BHFlybyScene *);
/* Source adapters for original 0042a9b0 and installed 00429fb0. */
void __cdecl bh_flyby_scene_rearm_entity(BHEntity *);
void __cdecl bh_flyby_scene_camera_step_entity(BHEntity *);
void __cdecl bh_flyby_scene_refresh_entity(BHEntity *,void *packet);
#endif
