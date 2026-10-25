#ifndef BH_VIEW_CAMERA_H
#define BH_VIEW_CAMERA_H
#include "bh_math.h"
typedef struct BHViewConfig {
    float fov,near_z,far_z,width,height,origin_x,origin_y;
    float left,right,top,bottom;
} BHViewConfig;
typedef struct BHViewPlane {BHVec3 normal;float offset;} BHViewPlane;
enum {BH_VIEW_NEAR,BH_VIEW_FAR,BH_VIEW_RIGHT,BH_VIEW_LEFT,BH_VIEW_TOP,BH_VIEW_BOTTOM};
typedef struct BHViewCamera {
    BHMat34 projected,inverse;
    BHVec3 position;
    BHViewPlane planes[6];
    float focal;
} BHViewCamera;
/* Explicit source config/default binding; no original constructor claim. */
int __cdecl bh_view_config_init(BHViewConfig *,long width,long height);
/* Complete00451660 over explicit target dimensions/context. Resets origins to
   zero and clip bounds to full target; preserves FOV and depth settings. */
int __cdecl bh_view_reset_viewport(BHViewConfig *,long width,long height);
/* Complete004515b0. Preserves asymmetric clamps and inverted bounds. */
int __cdecl bh_view_clip_rect(BHViewConfig *,float x,float y,float width,float height);
/* Complete00451b80. Outward plane inequality: dot(normal,point)<=offset.
   Caller PC53/nearest/masked profile. Uses original rigid-inverse semantics;
   does not substitute a general inverse for nonrigid inputs. Input/output may
   not overlap. Source validation failure retains output. */
int __cdecl bh_view_camera_build(const BHViewConfig *,const BHMat34 *,BHViewCamera *);
#endif
