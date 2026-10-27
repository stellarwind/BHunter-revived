#ifndef BH_SCREEN_QUAD_H
#define BH_SCREEN_QUAD_H
#include "bh_pickup_draw.h"
typedef struct BHScreenQuadContext
{
    const BHViewConfig *view;
    BHScreenClipContext *clip;
    BHModelPolygonEmit emit;
    void *emit_user;
    unsigned long base_color, emitted, culled;
} BHScreenQuadContext;
/* Complete00455410, including missed modulation8th argument. Whole full-image
   quad, flip/quarter-turn UV, rotated geometry, clipping, live near_z+.01.
   Selected emitter consumes synchronously; software dispatcher is separate.
   Borrowed dependencies/PC53 nearest profile must outlive synchronous call. */
int __cdecl bh_screen_quad_draw(BHScreenQuadContext *, BHTextureImage *, float x, float y, float width, float height,
                                float angle, unsigned char flags, float modulation);
/* Complete00478e50. Original out-of-range selectors map to header0.
   Return1 queued,0 full,-1 invalid source binding/topology. */
int __cdecl bh_screen_quad_queue(BHDrawQueue *, unsigned long bucket, unsigned long image, float x, float y,
                                 float width, float height, float angle, unsigned long flags, float modulation);
typedef struct BHScreenQuadSubmit
{
    BHScreenQuadContext *draw;
    BHDrawImageLookup lookup;
    void *lookup_user;
} BHScreenQuadSubmit;
/* Complete00477680: low-byte flags, then next load after draw (also on source
   lookup/draw failure). Input pointers/output/context must not overlap. */
int __cdecl bh_screen_quad_execute(BHScreenQuadSubmit *, const BHDrawCommand *, unsigned long *next);
#endif
