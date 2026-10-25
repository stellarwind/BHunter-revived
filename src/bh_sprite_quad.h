#ifndef BH_SPRITE_QUAD_H
#define BH_SPRITE_QUAD_H
#include "bh_main_camera.h"
#include "bh_light_config.h"
#include "bh_light_eval.h"
#include "bh_model_draw.h"

typedef enum BHSpriteQuadError {
    BH_SPRITE_QUAD_OK,BH_SPRITE_QUAD_ARGUMENT,BH_SPRITE_QUAD_NUMERICAL,
    BH_SPRITE_QUAD_CLIP,BH_SPRITE_QUAD_LIGHT,BH_SPRITE_QUAD_EMIT
} BHSpriteQuadError;
/* Explicit borrowed bindings of 00457590's original camera/light globals.
   The emitter consumes the temporary clipped ring synchronously. This owner
   does not implement the software branch of original dispatch 0046e390. */
typedef struct BHSpriteQuadContext {
    const BHMainCamera *camera;
    BHClipContext *clip;
    const BHLightConfig *directional;
    const BHLightContext *world_lights;
    long point_enabled;
    unsigned long base_color,color,emitted;
    BHModelPolygonEmit emit;
    void *emit_user;
    BHSpriteQuadError error;
} BHSpriteQuadContext;
/* Complete 00457590: arbitrary world plane, Gram-Schmidt up basis, optional
   in-plane rotation, backface/winding and UV controls, real clipping and both
   lighting paths. flags bits0/1 flip U/V; bit2 transposes UV; bit3 admits the
   back face. Modulation is original float argument15, missed by decompiler.
   Original input normal is negated before use. Zero/parallel basis vectors
   retain original degenerate geometry. Moderate finite PC53 contract; source
   guards reject nonfinite/conversion overflow. Returns1 also for a cull.
   UV arguments are start/extent pairs. Inputs are copied before output writes.
   No original code or fake backend callback is supplied by this module. */
int __cdecl bh_sprite_quad_draw(BHSpriteQuadContext *,BHTextureImage *,
    const BHVec3 *center,const BHVec3 *normal,const BHVec3 *up,
    float width,float height,float angle,unsigned char flags,float modulation,
    float u,float u_extent,float v,float v_extent);
#endif
