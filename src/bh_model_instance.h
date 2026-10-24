#ifndef BH_MODEL_INSTANCE_H
#define BH_MODEL_INSTANCE_H
#include "bh_main_camera.h"
#include "bh_model_draw_lit.h"

typedef enum BHModelInstanceError {
    BH_MODEL_INSTANCE_OK, BH_MODEL_INSTANCE_ARGUMENT, BH_MODEL_INSTANCE_NUMERICAL,
    BH_MODEL_INSTANCE_CAMERA, BH_MODEL_INSTANCE_LIGHT, BH_MODEL_INSTANCE_DRAW
} BHModelInstanceError;

/* Explicit bindings for 0045aa60's scattered globals. All pointers are
   borrowed and must outlive a call. The model starts from the original
   identity transform constant at 0048c030;
   camera->view is the global world-to-camera transform.
   light_reference is transformed from model to world space before point-light
   prepare; its borrowed source represents writable global 00495e30.
   The emitter may read opacity from this context during its synchronous call. */
typedef struct BHModelInstanceContext {
    BHModel *model;
    BHModelDrawCache *cache;
    const BHMainCamera *camera;
    const BHViewConfig *config;
    BHClipContext *clip;
    BHLightContext *lights;
    const BHLightConfig *light_config;
    const BHVec3 *light_reference;
    BHModelPolygonEmit emit;
    void *emit_user;
    unsigned long texture_mask,base_color,color,frame;
    float opacity;
    long point_enabled;
    BHMat34 last_camera_view;
    unsigned long drawn,faces_marked,polygons_emitted;
    BHModelInstanceError error;
} BHModelInstanceContext;

/* Complete 0045aa60 model-instance owner over explicit bindings. x/y/z and
   local Y/X/Z angles are original double arguments; rounding to float happens
   only when building the model transform. opacity is the final float argument,
   used for the subtractive grayscale color and retained for primitive output.
   Returns 1 for a successful call, including a culled instance (drawn=0).
   Culling leaves model/output/counters unchanged. Visible calls mutate the
   model transform, advance frame, and copy camera->view to last_camera_view
   after traversal. A failed visible draw may have emitted earlier polygons;
   color and counters retain that partial traversal, while drawn stays zero.
   Frame wrap is rejected by the source guard because the draw cache reserves
   zero as its initial invalid token. */
int __cdecl bh_model_instance_draw(BHModelInstanceContext *,
    double x,double y,double z,double rotate_y,double rotate_x,double rotate_z,
    float opacity);
#endif
