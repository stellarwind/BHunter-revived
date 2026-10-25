#ifndef BH_PRESENT_H
#define BH_PRESENT_H
#include "bh_fade.h"
/* Complete0045d690 over explicit live backend bindings. auxiliary(+9c48)
   and color(+9c3c) are already locked actual surfaces on software entry.
   target(+9c4c) is optional. Software flag is fade.render.textures.software.
   All interfaces and fade pixels remain borrowed from the actual owners. */
typedef struct BHPresentContext {
    BHFadeContext *fade;
    LPDIRECTDRAWSURFACE auxiliary,color,target;
    unsigned long locked; /* original004e9c50; cleared after both Unlock calls */
    unsigned long native_calls,fade_calls,restores,copies,failures;
    HRESULT last_hresult;
} BHPresentContext;
/* Positive ordered amount applies fade first. Native ends the existing scene.
   Software unlocks both surfaces, restores only DDERR_SURFACELOST, and copies
   color to optional target via BltFast(0,0,NULL,DDBLTFAST_WAIT). Native HRESULT
   failures are reported while later original operations still execute. */
int __cdecl bh_present_end(BHPresentContext *);
#endif
