#ifndef BH_DRAW_SUBMIT_H
#define BH_DRAW_SUBMIT_H
#include "bh_draw_queue.h"
#include "bh_draw_resource.h"

/* 00478170: append a borrowed resource handle and binary32 pose to one
   bucket. The handle is resolved at execution time; it is never interpreted
   as an original x86 pointer. Returns 1 queued, 0 full, -1 invalid. */
int __cdecl bh_draw_submit_resource(BHDrawQueue *queue,unsigned long bucket,
    unsigned long handle,const BHVec3 *position,const BHVec3 *angles,
    float modulation);

typedef const BHDrawResource *(__cdecl *BHDrawResourceLookup)(
    unsigned long handle,void *user);
typedef struct BHDrawSubmitContext {
    BHDrawResourceContext *draw;
    BHDrawResourceLookup lookup;
    void *lookup_user;
} BHDrawSubmitContext;

/* 004775f0: dispatch one queued command, then read its next link after the
   draw returns. All bindings are borrowed through this synchronous call.
   The link is published even when the source draw reports failure, as the
   original callback reads command+0x50 after invoking the resource owner.
   The next output must not alias command storage. Lookup must not mutate the
   command; descriptor and renderer bindings stay valid until draw returns. */
int __cdecl bh_draw_submit_execute(BHDrawSubmitContext *context,
    const BHDrawCommand *command,unsigned long *next);

typedef BHTextureImage *(__cdecl *BHDrawImageLookup)(
    unsigned long handle,void *user);
typedef struct BHDrawBillboardSubmitContext {
    BHSpriteBillboardContext *draw;
    BHDrawImageLookup lookup;
    void *lookup_user;
} BHDrawBillboardSubmitContext;

/* 00477640: consume the 00478280 billboard payload. The image handle is
   resolved explicitly and borrowed through the synchronous sprite draw.
   Publish the command's current next link after drawing, even on a source
   draw failure. next must not alias command storage; lookup must not mutate
   the command. Missing image bindings publish the link and report failure. */
int __cdecl bh_draw_submit_billboard_execute(
    BHDrawBillboardSubmitContext *context,const BHDrawCommand *command,
    unsigned long *next);
#endif
