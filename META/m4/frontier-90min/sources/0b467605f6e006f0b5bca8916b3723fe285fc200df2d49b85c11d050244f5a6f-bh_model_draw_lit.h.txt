#ifndef BH_MODEL_DRAW_LIT_H
#define BH_MODEL_DRAW_LIT_H
#include "bh_model_draw.h"
#include "bh_light_eval.h"
#include "bh_light_config.h"
typedef enum BHModelDrawLitError {BH_MODEL_DRAW_LIT_OK,BH_MODEL_DRAW_LIT_ARGUMENT,BH_MODEL_DRAW_LIT_LIGHT,BH_MODEL_DRAW_LIT_NUMERICAL,BH_MODEL_DRAW_LIT_DRAW} BHModelDrawLitError;
typedef struct BHModelDrawLitContext {
    BHModelDrawContext *draw;
    const BHLightContext *prepared;
    const BHLightConfig *directional;
    BHVec3 local_direction;
    long point_enabled;
    float scale;
    unsigned long base_color,nodes_shaded;
    BHModelDrawLitError error;
} BHModelDrawLitContext;
/* Explicit scattered-global binding. Prepared lights have already been
   prepared for this model. Real world-to-model direction transform is used
   only when directional.enabled!=0; no normalization is added. Contexts and
   borrowed lists/config outlive traversal; no caller-owned data is copied
   except local_direction/scalars. Existing draw cache/model must be valid. */
int __cdecl bh_model_draw_lit_context_init(BHModelDrawLitContext *,BHModelDrawContext *,
    const BHLightContext *,const BHLightConfig *,const BHMat34 *world_to_model,
    long point_enabled,float scale,unsigned long base_color);
/* Complete00459510. Original visibility/BSP painter order and side-specific
   lighting are preserved; emits through actual frozen00458310/00458430 owners.
   Dynamic point uses binary32 AABB midpoint and raw node normal on both sides;
   directional sign reverses only for nonnegative eye side. Signed32 truncation
   then channel clamping and base-color32bit addition match original.
   Source guards reject nonfinite/out-of-range conversion rather than invoking
   undefined C conversion. draw.color retains last shaded node even without a
   leaf, and earlier emission is not rolled back on a later error. Shares draw
   task storage; callback/lifetime/nonreentrancy and PC53 contracts apply. Whole
   software dispatcher remains unfinished; native emission is explicit adapter. */
int __cdecl bh_model_draw_lit(BHModelDrawLitContext *,long node,unsigned long mask);
#endif
