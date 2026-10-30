#ifndef BH_PORTAL_CLIP_H
#define BH_PORTAL_CLIP_H
#include "bh_draw_queue.h"
#include "bh_main_camera.h"
#define BH_PORTAL_CLIP_WORLD 0x5052544cUL
#define BH_PORTAL_CLIP_CAPACITY 3500
/* Parent links and world-space corners survive interpolation unchanged. Bounds
   are cached only for one render execution, never across camera samples. */
typedef struct BHPortalClipCache
{
    float bounds[BH_PORTAL_CLIP_CAPACITY][4];
    unsigned char ready[BH_PORTAL_CLIP_CAPACITY];
    unsigned long projections;
} BHPortalClipCache;
void __cdecl bh_portal_clip_reset(BHPortalClipCache *);
int __cdecl bh_portal_clip_project(const BHDrawQueue *, unsigned long command, const BHMainCamera *,
                                   const BHViewConfig *, BHPortalClipCache *, float bounds[4]);
#endif
