#ifndef BH_HORIZON_H
#define BH_HORIZON_H
#include "bh_horizon_color.h"
#include "bh_main_camera.h"
#include "bh_attribute_clipper.h"
#include "bh_render_state.h"
/* gradient=1 selects original0046e530; gradient=0 selects0046e390 with white
   diffuse color. Scalar unknown28 is float fog coverage:0 inner/1 outer.
   RGB contains original signed-truncation extraction, in0..255 floats.
   Callback consumes synchronously without retaining/mutating context or ring.
   Selecting a callback does not implement either software raster dispatcher. */
typedef int (__cdecl *BHHorizonEmit)(BHClipVertex *,BHTextureImage *,int gradient,const float rgb[3],void *);
typedef enum BHHorizonError {BH_HORIZON_OK,BH_HORIZON_ARGUMENT,BH_HORIZON_COLOR,BH_HORIZON_CLIP,BH_HORIZON_EMIT,BH_HORIZON_BACKEND} BHHorizonError;
typedef struct BHHorizonContext {
    const BHMainCamera *camera;BHAttributeClipContext *clip;
    BHRenderState *state;BHHorizonColor *colors;
    BHHorizonEmit emit;void *user;
    float rgb[3];unsigned long attempted,emitted;
    BHHorizonError error;
} BHHorizonContext;
/* Explicit borrowed owning bindings. Camera's projected matrix, position.x/z
   and angles.x are the original global projection/eye/yaw values. Context,
   dependencies and emitter user must not overlap or recursively enter. */
int __cdecl bh_horizon_init(BHHorizonContext *,const BHMainCamera *,BHAttributeClipContext *,BHRenderState *,BHHorizonColor *,BHHorizonEmit,void *);
/* Complete0045db80 with explicit selected primitive binding. All nine source
   vertices/eight polygons, native/software branch choices, software color
   preparation, native fog/depth and flush lifetime are retained. Software
   callback must own the still-unrecovered software dispatch/raster contracts;
   no software backend is supplied here. Caller owns PC53 profile. */
int __cdecl bh_horizon_draw(BHHorizonContext *,BHTextureImage *,float texture_scale,float height,
    float outer_radius,float inner_radius,unsigned long color);
/* Explicit native-only adapter for0046e530's native branch. Whole0046e530
   remains unrecovered because its software branch calls0046dda0. Uses fixed
   original near numerator1 and specular alpha trunc(-255*scalar)+255. */
typedef struct BHHorizonNative {BHTextureContext *textures;unsigned long polygons,vertices;} BHHorizonNative;
int __cdecl bh_horizon_native_emit(BHClipVertex *,BHTextureImage *,int,const float [3],void *);
#endif
