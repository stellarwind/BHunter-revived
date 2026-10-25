#ifndef BH_QUEUE_RESERVE_H
#define BH_QUEUE_RESERVE_H
#include "bh_draw_queue.h"
#include "bh_render_state.h"
/* Complete00477530. Reserves borrowed command storage without writing kind,
   resource or ANY payload word. All signed selectors outside0..8 map header0.
   Existing source queue count/capacity bounds and topology must be valid.
   NULL means full or invalid binding; inspect queue.error. No pointer retained. */
BHDrawCommand *__cdecl bh_queue_reserve(BHDrawQueue *,long bucket);
/* Explicit source convenience adapter, not a third original function. */
int __cdecl bh_queue_mode(BHDrawQueue *,long bucket,unsigned long mode);
/* Complete00477360: actual mode owner then current next link load, even when
   source mode reports failure. Context/command/next must not overlap. */
int __cdecl bh_queue_mode_execute(BHRenderState *,const BHDrawCommand *,unsigned long *next);
#endif
