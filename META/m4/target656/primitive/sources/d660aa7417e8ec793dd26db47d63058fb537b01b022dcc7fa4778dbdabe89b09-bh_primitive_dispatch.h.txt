#ifndef BH_PRIMITIVE_DISPATCH_H
#define BH_PRIMITIVE_DISPATCH_H
#include "bh_software_raster.h"
#include "bh_depth_subdivide.h"
#include "bh_render_state.h"
typedef enum BHPrimitiveError {BH_PRIMITIVE_OK,BH_PRIMITIVE_ARGUMENT,
    BH_PRIMITIVE_BACKEND,BH_PRIMITIVE_RASTER,BH_PRIMITIVE_QUEUE} BHPrimitiveError;
typedef struct BHPrimitiveDispatch {
    BHRenderState *state;BHSoftwareRaster raster;BHDepthSplitContext split;
    BHRasterKernel selected;unsigned long selected_index,key_enabled,base_color;
    unsigned long locks,unlocks,polygons,vertices;
    HRESULT last_result;BHPrimitiveError error;
} BHPrimitiveDispatch;
/* Count-zero borrowed-frame binding. Frame memory, native image/context,
   software/gradient tables and state must outlive synchronous emission. */
int __cdecl bh_primitive_dispatch_init(BHPrimitiveDispatch *,BHRenderState *,
    const BHSoftwareRaster *frame,float left,float right,float top,float bottom);
/* Whole0046dda0: actual source-surface Lock/Unlock, color/mode/depth/key
   selection across28 real raster owners, then actual plain/scalar subdivision.
   Original ignores texture pitch and assumes packed square8-bit texels; source
   rejects an incompatible Lock layout. The original alpha bucket0 path leaked
   its Lock; source always Unlocks, including alpha0 and guarded failure. */
int __cdecl bh_software_polygon(BHPrimitiveDispatch *,BHTextureImage *,BHClipVertex *,unsigned long color);
/* Whole0046e390/0046e430/0046e530: actual native per-vertex arena append/run
   completion or full software path. Scalar uses live base_color(+00491758),
   unsigned packed-color carry and average255 scalar in software. Gradient
   keeps native white diffuse/specular alpha; software temporarily enables
   scalar subdivision/gradient palette and resets that flag to0 afterwards. */
int __cdecl bh_primitive_emit(BHPrimitiveDispatch *,BHTextureImage *,BHClipVertex *,unsigned long color);
int __cdecl bh_primitive_emit_scalar(BHPrimitiveDispatch *,BHTextureImage *,BHClipVertex *);
int __cdecl bh_primitive_emit_gradient(BHPrimitiveDispatch *,BHTextureImage *,BHClipVertex *,unsigned long color);
#endif
