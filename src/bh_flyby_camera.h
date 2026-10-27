#ifndef BH_FLYBY_CAMERA_H
#define BH_FLYBY_CAMERA_H
#include "bh_camera_route.h"
#include "bh_camera_curve.h"
#include "bh_camera_state.h"
#include "bh_player_camera.h"
#include "bh_text_queue.h"
typedef struct BHFlybyCredits
{
    long gate;
    unsigned long index;
} BHFlybyCredits;
typedef struct BHFlybyCameraState
{
    BHCameraRouteState route;
    BHVec3 position, previous_position, older_position, angles, velocity;
    BHVec3 basis_x, basis_y, basis_z;
    BHCameraState camera;
    double orbit_time;
} BHFlybyCameraState;
typedef struct BHFlybyCameraContext
{
    BHCameraRouteContext *route;
    BHCameraStateContext *camera;
    const BHPlayerCameraStart *starts;
    unsigned long start_count, actor_token;
    BHAtlasPage *credit_font;
    BHTextQueue *text;
    BHFlybyCredits *credits;
} BHFlybyCameraContext;
/* Complete00429fb0: fixed-step orbit/route transitions, filtered actor pose,
   motion-derived basis/velocity, real camera update and all credits branches.
   metadata[] of actual32-byte start rows contains float orbit amplitudes here;
   reset owner preserves those same words. Explicit route positions must agree.
   Context/state storage is initialized, live, finite, nonoverlapping. At phase0
   rate is unused and may be zero as in the fresh original constructor; active
   phases require positive finite rate. Producer needs live page/atlas metrics;
   later native text consumption also requires live actual image/surfaces.
   Source guards reject invalid phase/table/font bindings. Returns1 complete,0 text
   command dropped on full ring,-1 invalid/dependency failure. Native glyph
   execution occurs later through the real text ring consumers. No game loop
   or original callback is loaded/executed by this owner. */
int __cdecl bh_flyby_camera_step(BHFlybyCameraContext *, BHFlybyCameraState *);
#endif
