#ifndef BH_VIEW_CULL_H
#define BH_VIEW_CULL_H
#include "bh_main_camera.h"
/* Complete0045a7c0. Returns0xffffffff when sphere is outside, otherwise the
   remaining hierarchical plane mask. Bits1/2=near/far;4/8/16/32=right/left/
   top/bottom. If EITHER depth bit is present BOTH depth tests are performed.
   Near/far clears use original AND0x3e/0x3d, discarding unknown high bits;
   side clears subtract their bit and retain high bits. Strict comparisons
   retain tangent bits. Plane0 supplies depth; reserved plane1 is never read.
   Optional depth is stored even on mask0 or rejection and may be NULL.
   Comparisons retain PC53 depth despite binary32 depth-output storage.
   Valid camera/config storage, finite coordinates/radius>=0, original plane
   normalization and caller PC53/nearest/masked profile are caller contracts.
   Lazy reads match original: mask0/depthNULL requires neither context; config
   is read only when a depth bit is present, camera only for active planes or
   requested depth. No pointer is retained, no original code is executed. */
unsigned long __cdecl bh_view_cull_sphere(const BHViewConfig *, const BHMainCamera *, float x, float y, float z,
                                          float radius, unsigned long mask, float *depth);
#endif
