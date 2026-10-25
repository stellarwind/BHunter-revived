#ifndef BH_PICKUP_DRAW_H
#define BH_PICKUP_DRAW_H
#include "bh_entities.h"
#include "bh_draw_submit.h"
#include "bh_native_primitive.h"
#include "bh_pickup.h"
#include "bh_entity_resource_draw.h"

/* Explicit bindings of original scattered renderer globals. Atlas/model/glow
   handles are source identities, never original image or code pointers. */
typedef struct BHPickupDrawContext {
    BHDrawQueue *queue;
    const BHMainCamera *camera;
    const BHViewConfig *view;
    BHScreenClipContext *clip;
    BHModelPolygonEmit emit;
    void *emit_user;
    unsigned long atlas_image,glow_image;
    long display_depth;
    unsigned long rectangles,culled;
} BHPickupDrawContext;

/* Source callback binding outlives spawned pickups. The constructor stores
   BHPickupContext through its own owner field; these step adapters resolve it
   on each invocation, without embedding renderer pointers in original payload. */
typedef struct BHPickupRenderBinding {
    BHPickupDrawContext *default_draw;
    BHEntityResourceDrawContext *special_draw;
} BHPickupRenderBinding;
int __cdecl bh_pickup_render_bind(BHPickupContext *,BHPickupRenderBinding *);
void __cdecl bh_pickup_default_step(BHEntity *);
void __cdecl bh_pickup_special_step(BHEntity *);

/* Complete 004558d0 projected-point helper over an explicit camera/view. */
int __cdecl bh_pickup_project(const BHPickupDrawContext *,
    float *x,float *y,float *z,float *scale);

/* Complete 00478f60: original 0x54 queued UV rectangle record and bucket links.
   UV words retain original pair order: U-start,U-extent,V-start,V-extent.
   Returns 1 appended, 0 full, -1 invalid queue. */
int __cdecl bh_pickup_rect_queue(BHDrawQueue *,unsigned long bucket,
    unsigned long image,float x,float y,float width,float height,float angle,
    unsigned long flags,float alpha,float u,float u_extent,float v,float v_extent,float depth);

/* 00454fc0 hardware path composed from recovered screen clipping and native
   emission. Software rasterization remains outside this adapter. */
int __cdecl bh_pickup_rect_draw(BHPickupDrawContext *,BHTextureImage *,
    float x,float y,float width,float height,float angle,unsigned long flags,
    float alpha,float u,float u_extent,float v,float v_extent,float depth);
/* Complete 004776c0 command consumer; next link is read after synchronous draw. */
int __cdecl bh_pickup_rect_execute(BHPickupDrawContext *,
    BHDrawImageLookup lookup,void *lookup_user,const BHDrawCommand *,unsigned long *next);

/* 00440c40 complete two-layer numeric/unit label queueing. */
int __cdecl bh_pickup_label(BHPickupDrawContext *,const BHEntity *,long total,long count);
/* 004412f0 default pickup draw: pose, resource, glow, numeric label. */
int __cdecl bh_pickup_draw(BHPickupDrawContext *,BHEntity *);
#endif
