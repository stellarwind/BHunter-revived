#ifndef BH_FLASH_FLARE_H
#define BH_FLASH_FLARE_H
#include "bh_script_flash.h"
#include "bh_screen_quad.h"
#include "bh_project_scale.h"
/* New source semantic selector; parent renderer adds a real dispatch case.
   Source payload: worldXYZ, size, mutable strength, angle, four texture IDs. */
#define BH_DRAW_FLARE 17UL
typedef struct BHFlashFlareContext
{
    BHEntityPool *pool;
    BHDrawQueue *queue;
    unsigned long textures[4];
} BHFlashFlareContext;
typedef struct BHFlareSubmit
{
    const BHMainCamera *camera;
    const BHViewConfig *view;
    long width, height;
    BHScreenQuadSubmit *screen;
    unsigned long attempted, emitted, culled, failures;
} BHFlareSubmit;
/* Count-zero VM state adapter: actual lifetime owner00403640 remains installed,
   real refresh below reads lifetime+e8, strength+ec and size+f0. No original
   VM constructor claim. Context/pool/queue and image bindings stay borrowed. */
int __cdecl bh_flash_flare_bind(BHFlashFlareContext *, BHEntity *, float lifetime, float strength, float size);
void __cdecl bh_flash_flare_refresh(BHEntity *, void *); /*004035e0*/
/* Whole00478540: fixed bucket7/header7; fourth texture0 borrows third.
   Return1 queued,0 full,-1 bad source context/topology. Other payload retained. */
int __cdecl bh_flare_queue(BHDrawQueue *, const BHVec3 *, float size, float strength, unsigned long first,
                           unsigned long second, unsigned long third, unsigned long fourth, float angle);
/* Whole00477ae0, including command strength normalization/mutation, complete
  14-row table and each real screenquad call. Camera/view/size bindings are
   actual source owners. Positive focal/near and width>=2/height>0 are source
   guards. Consumer publishes CURRENT next after all draw calls/culls/failures.
   Projection failure leaves strength unchanged; subsequent culls retain its
   normalized store. Image lookup/draw errors don't suppress later table rows.
   next may not alias command/context; draw callbacks must retain their owners. */
int __cdecl bh_flare_execute(BHFlareSubmit *, BHDrawCommand *, unsigned long *next);
#endif
