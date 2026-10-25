#ifndef BH_LOADING_BLIT_H
#define BH_LOADING_BLIT_H
#include "bh_loading_image.h"
typedef struct BHLoadingBlit {
 BHLoadingImages *images;
 long width,height;
 unsigned long writes,failures;
} BHLoadingBlit;
/* Allocation base and logical first-row offset are separate so negative signed
   pitch has a checked lifetime. The original arguments were logical row0 and
   pitch; no ownership is transferred. */
typedef struct BHLoadingBlitTarget {
 unsigned char *base;unsigned long bytes;long row0,pitch;
} BHLoadingBlitTarget;
/* Whole original four-argument wedge owners over explicit borrowed state.
   Finite from/to within +/-1e6, dimensions1..4096, PC53/nearest/masked profile.
   Pixel bytes1/2/4 map the BAD second page through current normal LUTs; other
   depths retain original no-store behavior. Signed column pitch truncates to
   a multiple of pixel size, exactly as the original typed pointer increments.
   Caller must provide distinct accessible source and destination spans. */
int __cdecl bh_loading_blit_top(BHLoadingBlit *,const BHLoadingBlitTarget *,float from,float to); /*0044a220*/
int __cdecl bh_loading_blit_right(BHLoadingBlit *,const BHLoadingBlitTarget *,float from,float to); /*0044a520*/
int __cdecl bh_loading_blit_bottom(BHLoadingBlit *,const BHLoadingBlitTarget *,float from,float to); /*0044a380*/
int __cdecl bh_loading_blit_left(BHLoadingBlit *,const BHLoadingBlitTarget *,float from,float to); /*0044a8a0*/
int __cdecl bh_loading_blit_top_last(BHLoadingBlit *,const BHLoadingBlitTarget *,float from,float to); /*0044a720*/
#endif
