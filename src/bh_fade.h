#ifndef BH_FADE_H
#define BH_FADE_H
#include "bh_render_state.h"
#include "bh_queue_reserve.h"
/* Explicit borrowed display/renderer bindings. Original active/cached globals
   are separate: rebuild copies cached INTO active, as the original does. */
typedef struct BHFadeContext
{
    BHRenderState *render;
    long width, height, pitch;
    unsigned char *pixels;
    unsigned long pixel_bytes, masks[3], extract[3], expand[3];
    float near_plane, amount, cached_amount;
    unsigned long color, cached_color;
    void *table;
    unsigned long table_bytes, table_ready;
    unsigned long rebuilds, pixels_changed, native_draws, failures;
    HRESULT last_hresult;
} BHFadeContext;
void __cdecl bh_fade_set(BHFadeContext *, float amount, unsigned long color);             /*0045cf30*/
int __cdecl bh_fade_queue(BHDrawQueue *, float amount, unsigned long color);              /*00478930*/
int __cdecl bh_fade_execute(BHFadeContext *, const BHDrawCommand *, unsigned long *next); /*00477f20*/
/* Complete0045cf50. Source guards reject unbound buffers/short row pitch and the
   original undefined first-use equal-cache table. Tables stay owned until dispose.
   Caller supplies accessible pitch*height pixel bytes; this API has no buffer-size field.
   Existing caller-supplied tables must be malloc-compatible. Source-built checks
   cover finite amount in[0,1] with ordinary8/16-bit display masks. Raw
   setter/queued state transport remains unrestricted; extreme conversion
   overflow behavior outside this domain is not independently validated. */
int __cdecl bh_fade_apply(BHFadeContext *);
void __cdecl bh_fade_dispose(BHFadeContext *);
#endif
