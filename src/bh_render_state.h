#ifndef BH_RENDER_STATE_H
#define BH_RENDER_STATE_H
#include "bh_texture.h"
/* Explicit borrowed bindings for original render-state globals. Texture
   context frame is original004ede80, shared with real image residency. */
typedef struct BHRenderState
{
    BHTextureContext *textures;
    unsigned long depth, blend, source, destination, mode;
    unsigned long native_calls, barriers, failures;
    HRESULT last_hresult;
} BHRenderState;
/* New binding adapter applies opaque defaults to the actual device rather
   than assuming externally initialized native states agree with its cache. */
int __cdecl bh_render_state_init(BHRenderState *, BHTextureContext *);
/* Complete0045b410: cache all values; native modes0/1/2 control depth write and
   compare. Unknown values update only the cache, as original. */
int __cdecl bh_render_depth(BHRenderState *, unsigned long mode);
/* Complete0045b490: no value normalization, no-op when all three match. */
int __cdecl bh_render_blend(BHRenderState *, unsigned long enable, unsigned long source, unsigned long destination);
/* Complete0045b500: native mode barrier ALWAYS increments residency epoch,
   flushes every pending image, EndScene/BeginScene, then selects mode0..4.
   Unknown mode still performs barrier and publishes mode. Software branch
   only updates original caches/mode: no native calls or queue consumption.
   Native errors are reported separately without rolling caches back or
   skipping subsequent calls. Caller must have begun a real native scene. */
int __cdecl bh_render_mode(BHRenderState *, unsigned long mode);
#endif
