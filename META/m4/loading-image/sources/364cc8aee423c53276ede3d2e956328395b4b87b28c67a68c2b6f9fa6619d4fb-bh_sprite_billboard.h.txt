#ifndef BH_SPRITE_BILLBOARD_H
#define BH_SPRITE_BILLBOARD_H
#include "bh_main_camera.h"
#include "bh_screen_clipper.h"
#include "bh_model_draw.h"

typedef enum BHSpriteBillboardError {
    BH_SPRITE_BILLBOARD_OK, BH_SPRITE_BILLBOARD_ARGUMENT,
    BH_SPRITE_BILLBOARD_NUMERICAL, BH_SPRITE_BILLBOARD_CLIP,
    BH_SPRITE_BILLBOARD_EMIT
} BHSpriteBillboardError;

/* Explicit binding of 00455960's scattered camera, viewport, color and
   primitive state. All pointers are borrowed for the synchronous call. */
typedef struct BHSpriteBillboardContext {
    const BHMainCamera *camera;
    const BHViewConfig *config;
    BHScreenClipContext *clip;
    BHModelPolygonEmit emit;
    void *emit_user;
    unsigned long base_color,color,emitted;
    BHSpriteBillboardError error;
} BHSpriteBillboardContext;

/* Complete 00455960. Position, world width/height and angle are float inputs;
   flags low bits flip U, flip V, and quarter-turn UV orientation. Final
   modulation is a distinct float ninth argument (omitted by old decompiler
   signature). Returns 1 on success including depth/screen cull (emitted=0).
   Emitter consumes the clipped ring synchronously and must not retain it. */
int __cdecl bh_sprite_billboard_draw(BHSpriteBillboardContext *,BHTextureImage *,
    float x,float y,float z,float width,float height,float angle,
    unsigned long flags,float modulation);
#endif
