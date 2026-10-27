#ifndef BH_SCREEN_CLIPPER_H
#define BH_SCREEN_CLIPPER_H
#include "bh_clipper.h"
/* Explicit binding of original global bounds and shared scratch ring. The
   original vertex record is reused after offset/size checks in the real TU. */
typedef struct BHScreenClipContext
{
    float near_z, far_z, left, right, top, bottom;
    unsigned long used;
    BHClipError error;
    BHClipVertex scratch[8];
} BHScreenClipContext;
/* Source-only config/context adapter; no original owner counted. */
int __cdecl bh_screen_clip_init(BHScreenClipContext *, float near_z, float far_z, float left, float right, float top,
                                float bottom);
/* Complete00453300: reject one uniform depth against near/far, then clip a
   convex doubly-linked screen-space polygon against left/right/top/bottom.
   u/v interpolate affinely, not with perspective correction. Existing vertices
   (including their point.z and projected flags) are untouched except links.
   Each generated pair receives depth, u/v, sx/sy, projected=0 and links; other
   scratch fields retain previous bytes. Caller owns valid pointer storage and
   PC53/nearest/masked profile. Input ring <=1024 nodes; no scratch as input.
   Return borrows input and context scratch until next call. NULL/errorOK means
   fully rejected. Validation can reject malformed/nonfinite input; failure
   after an earlier cut may leave ring links changed. Context is not thread-safe.
   Equal-boundary points, degenerate rings and winding are retained as original. */
BHClipVertex *__cdecl bh_screen_clip_polygon(BHScreenClipContext *, BHClipVertex *, float depth);
#endif
