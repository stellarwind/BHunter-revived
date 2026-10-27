#ifndef BH_SCENE_OWNER_H
#define BH_SCENE_OWNER_H
#include "bh_scene_bindings.h"

typedef enum BHSceneOwnerError
{
    BH_SCENE_OWNER_OK,
    BH_SCENE_OWNER_INACTIVE,
    BH_SCENE_OWNER_ARGUMENT,
    BH_SCENE_OWNER_BINDING,
    BH_SCENE_OWNER_SCHEDULE,
    BH_SCENE_OWNER_QUEUE
} BHSceneOwnerError;

/* 00409130 scene pass. Bindings and scheduler are borrowed. The two target
   dimensions belong to the draw target; camera dimensions are per call and
   may differ. mode==1 is the original scene-suppression gate. */
typedef struct BHSceneOwner
{
    BHSceneBindings *bindings;
    long target_width, target_height;
    long camera_width, camera_height;
    unsigned long mode, frame;
    BHMat34 view;
    BHViewCamera camera; /* projected matrix and eye used by portal facing */
    BHVec3 eye, angles, relative_camera;
    float near_z, far_z, focal, eye_projection_dot;
    BHSceneOwnerError error;
} BHSceneOwner;

int __cdecl bh_scene_owner_init(BHSceneOwner *owner, BHSceneBindings *bindings, long target_width, long target_height);
/* Returns one after scheduling and both full-target clip commands. A gated or
   failed call returns zero; a gate clears cached room without advancing frame.
   The scheduler's clip rectangle is set to camera dimensions before traversal;
   viewport width/height retain the caller's target extent for queued resets.
   Its near/far and camera are synchronized to this pass. */
int __cdecl bh_scene_owner_step(BHSceneOwner *owner, const BHVec3 *eye, const BHVec3 *angles, float fov_degrees,
                                float near_z, float far_z, long camera_width, long camera_height);
#endif
