#ifndef BH_CAMERA_STATE_H
#define BH_CAMERA_STATE_H
#include "bh_math.h"

/* Recovered camera mode records and the exact x86 state embedded at actor+e8.
   The final velocity ends at +9c: pack4 prevents native double tail padding.
   Mode-specific +24..2f fields remain opaque until their owners are restored. */
#pragma pack(push, 4)
typedef struct BHCameraModeState
{
    long mode, first_update;
    float seed_yaw;
    BHVec3 position, angles;
    unsigned long mode_data[3];
} BHCameraModeState;
typedef struct BHCameraState
{
    BHCameraModeState current, previous;
    float look_yaw, look_pitch;
    double blend_remaining, elapsed;
    BHVec3 position, angles, velocity;
} BHCameraState;
#pragma pack(pop)

/* Original global 004f559c is shared across camera states. The third argument
   is an opaque original actor token, despite decompiler float inference at
   0047aac0. The callback borrows both positions synchronously and may move eye.
   Installing a callback does not execute it or claim its behavior recovered. */
typedef void(__cdecl *BHCameraCorrection)(BHVec3 *eye, const BHVec3 *target, unsigned long actor_token);
typedef void(__cdecl *BHCameraCorrectionContextual)(BHVec3 *eye, const BHVec3 *target, unsigned long actor_token,
                                                    void *user);
typedef struct BHCameraInput
{
    BHVec3 target, angles, basis_x, basis_y, basis_z;
    unsigned long actor_token;
} BHCameraInput;
typedef struct BHCameraStateContext
{
    BHCameraCorrection correction;
    BHVec3 target;
    unsigned long actor_token;
    double elapsed;
    BHCameraCorrectionContextual contextual_correction;
    void *correction_user;
} BHCameraStateContext;

/* Complete0047b2e0. Only the eleven original words are initialized. The owner
   must supply any pose/mode data before the camera update consumes it. */
int __cdecl bh_camera_state_init(BHCameraState *, long mode);
/* Complete0047b210. Same-mode call preserves every byte, including the timer.
   A change copies the entire old mode record and starts a .5-second blend.
   Modes8/11 seed their yaw from output yaw rotated by pi and reset elapsed.
   Special-mode nonfinite yaw is rejected unchanged by the source guard.
   Requires caller PC53/nearest profile; PC24 and huge-angle fidelity open. */
int __cdecl bh_camera_state_set_mode(BHCameraState *, long mode);
/* Complete0047b320 over an explicit owner for the shared callback global.
   ignored_state is deliberately unused, including when null. */
int __cdecl bh_camera_state_set_correction(BHCameraStateContext *, const BHCameraState *ignored_state,
                                           BHCameraCorrection);
/* Source binding adapter for recovered callbacks whose original globals now
   belong to an explicit source context. Replaces raw binding; the original
   pointer-install owner above conversely clears this contextual binding. */
int __cdecl bh_camera_state_bind_correction(BHCameraStateContext *, BHCameraCorrectionContextual, void *user);
/* Complete00479910. Corrections borrow/may change desired_position before
   the first-update fast path or retention smoothing. The state pose is written
   in original order, with binary32 products stored before position blending. */
void __cdecl bh_camera_mode_smooth(BHCameraStateContext *, BHCameraModeState *, BHVec3 *desired_position,
                                   const BHVec3 *desired_angles, double position_retention, double angle_retention);
/* Complete0047aac0 once linked with every real mode handler. Input borrows
   must not overlap state/context. All vectors, timers and look requests must
   be finite and dt positive; guard failure preserves state/context. */
int __cdecl bh_camera_state_update(BHCameraStateContext *, BHCameraState *, const BHCameraInput *, double dt,
                                   float look_yaw, float look_pitch);
#endif
