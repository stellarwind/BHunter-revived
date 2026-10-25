#ifndef BH_CAMERA_MODES_H
#define BH_CAMERA_MODES_H
#include "bh_camera_state.h"
/* Complete00479c70: target minus eye, basis up, Y/X/Z Euler extraction.
   Aliasing output with an input is supported through local vectors. */
void __cdecl bh_camera_look_at(BHVec3 *angles,const BHVec3 *target,const BHVec3 *eye,const BHVec3 *up);
/* Source dispatch for the twelve actual original mode-table owners. The
   shared smoothing/correction owner is real source, not a test callback.
   Inputs/context initialized by camera update; moderate finite PC53 inputs.
   Degenerate look vectors retain the original normalization behavior. */
int __cdecl bh_camera_mode_update(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_0(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_1(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_2(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_3(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_4(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_5(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_6(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_7(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_8(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_9(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_10(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
void __cdecl bh_camera_mode_11(BHCameraStateContext *,BHCameraModeState *,const BHCameraInput *);
#endif
