#ifndef BH_DRAW_CONTROLS_H
#define BH_DRAW_CONTROLS_H
#include "bh_draw_queue.h"
#include "bh_render_resources.h"
#include "bh_screen_clipper.h"
#include "bh_attribute_clipper.h"

typedef struct BHDrawControlContext
{
    BHViewConfig *view;
    BHRenderResources *resources;
    BHClipContext *model_clip;
    BHScreenClipContext *screen_clip;
    BHAttributeClipContext *attribute_clip;
    unsigned long texture_mask;
} BHDrawControlContext;
/* Complete 00478990/00478a70. Source return extends original void API:
   1 queued, 0 full, -1 invalid. Float payloads are copied without sanitizing. */
int __cdecl bh_draw_controls_state(BHDrawQueue *, unsigned long bucket, long value);
int __cdecl bh_draw_controls_clip_rect(BHDrawQueue *, unsigned long bucket, float x, float y, float width,
                                       float height);
/* 00458300 over explicit global bindings. Registered model resources and
   future resource prototypes receive the same mask as the shared state. */
int __cdecl bh_draw_controls_texture_mask(BHDrawControlContext *, unsigned long mask);
/* Complete 00477f40/00477e40. The latter skips applying its rectangle if the
   next command is another rectangle. Queue storage must be live and stable;
   next output must not alias queue/context memory. Returns source success. */
int __cdecl bh_draw_controls_execute(BHDrawControlContext *, const BHDrawQueue *, const BHDrawCommand *,
                                     unsigned long *next);
#endif
