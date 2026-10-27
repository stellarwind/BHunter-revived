#ifndef BH_ATTRIBUTE_CLIPPER_H
#define BH_ATTRIBUTE_CLIPPER_H
#include "bh_clipper.h"
typedef struct BHAttributeClipContext
{
    float near_z, far_z, left, right, top, bottom;
    unsigned long used;
    BHClipError error;
    BHClipVertex scratch[12];
} BHAttributeClipContext;
/* Explicit source context adapter. Caller owns PC53/nearest/masked profile. */
int __cdecl bh_attribute_clip_init(BHAttributeClipContext *, float near_z, float far_z, float left, float right,
                                   float top, float bottom);
/* Complete00453c30. Nonzero project_depth clips homogeneous point against
   near/far then projects point.xy/z only when projected==0. Zero skips both
   depth bounds and projection, honoring supplied sx/sy even if projected==0.
   Four side cuts perspective-correct UV/Z, but interpolate unknown28's FLOAT
   representation affinely in screen coordinates. Depth cuts interpolate that
   scalar with homogeneous attributes. No semantic color/alpha name is assumed.
   Use memcpy to store/load a float in unknown28 without violating C aliasing.
   Valid-pointer convex reciprocal ring <=1024 nodes is the caller contract.
   Output borrows input+scratch until next clipping operation; consume it before
   any clipping owner reuses original shared scratch. No scratch output as input.
   Generated nodes retain untouched local/frame/pointxy bytes (pointxy written
   only by depth cuts), projected=1 after depth projection,0 after side cuts.
   Source guards reject malformed/nonfinite data; earlier completed cuts may
   remain on error. NULL/errorOK is complete rejection. No pointers retained. */
BHClipVertex *__cdecl bh_attribute_clip_polygon(BHAttributeClipContext *, BHClipVertex *, int project_depth);
#endif
