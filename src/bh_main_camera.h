#ifndef BH_MAIN_CAMERA_H
#define BH_MAIN_CAMERA_H
#include "bh_view_camera.h"
/* Explicit owning context for original scattered globals. Its native layout is
   deliberately not an assertion that original globals formed this structure.
   planes[] retains the original 4e97b8-relative slots: forward, untouched,
   right, left, top, bottom. Plane0 passes through the eye (no near-depth shift).
   Depth settings remain in BHViewConfig and are not used to construct planes. */
typedef struct BHMainCamera {
    BHMat34 projected,inverse,view;
    BHVec3 position,angles;
    BHViewPlane planes[6];
    float focal;
    unsigned long projection_state;
} BHMainCamera;
/* 00451720: degree-to-radian FOV conversion and double-to-float depth stores.
   Other viewport/config fields are preserved. Guard failure preserves config. */
int __cdecl bh_main_camera_settings(BHViewConfig *,double degrees,double near_z,double far_z);
/* 00451750: retains plane slot1, stored pose angles and view matrix, resets
   projection_state. Camera may equal context.view or context.projected (the
   latter intentionally inverses the newly projected matrix, as original).
   Other partial/output overlaps are unsupported. All context bytes must be
   initialized by its owner. Caller PC53/nearest/masked profile is required.
   Validation failure preserves context; finite nonrigid matrices retain the
   original rigid-inverse semantics. No depth clipping plane is synthesized. */
int __cdecl bh_main_camera_build(const BHViewConfig *,const BHMat34 *,BHMainCamera *);
/* 00452070: degree settings are separate; pose angles below are radians.
   Stores binary32 angles, constructs Rz(-roll)*Rx(-pitch)*Ry(-yaw)*T(-eye),
   then builds the global projection and planes. Output position is the
   recovered rigid inverse origin, not a substitute exact copy of eye. */
int __cdecl bh_main_camera_pose(const BHViewConfig *,BHMainCamera *,
    double x,double y,double z,double yaw,double pitch,double roll);
/* Complete004018a0, original world-Z pre-rotation including translation.
   Declared here while preserving the existing rotation module source variant.
   Moderate finite angles exercised; huge-angle FSIN/FCOS remains unvalidated. */
void __cdecl bh_mat34_rotate_world_z(float radians,BHMat34 *);
#endif
