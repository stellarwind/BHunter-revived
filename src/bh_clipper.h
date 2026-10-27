#ifndef BH_CLIPPER_H
#define BH_CLIPPER_H
#include "bh_texture.h"
#include "bh_math.h"
typedef struct BHClipVertex
{
    BHVec3 local, point;
    float u, v, sx, sy;
    unsigned long unknown28;
    struct BHClipVertex *next, *previous;
    float frame;
    unsigned long projected;
} BHClipVertex;
typedef enum BHClipError
{
    BH_CLIP_OK,
    BH_CLIP_ARGUMENT,
    BH_CLIP_TOPOLOGY,
    BH_CLIP_NUMERICAL,
    BH_CLIP_CAPACITY
} BHClipError;
typedef struct BHClipContext
{
    float near_z, far_z, left, right, top, bottom;
    BHMat34 camera, inverse;
    unsigned long inverse_ready, used;
    BHClipVertex scratch[12];
    BHClipError error;
} BHClipContext;
/* Explicit source configuration. Caller owns PC53/nearest/masked profile.
   Camera maps world coordinates to the homogeneous projection coordinates
   stored in point. Set it once per frame to invalidate projected-texture cache. */
int __cdecl bh_clip_init(BHClipContext *, float near_z, float far_z, float left, float right, float top, float bottom);
int __cdecl bh_clip_set_camera(BHClipContext *, const BHMat34 *camera);
/* Complete00452140. Mutates a convex doubly-linked valid-pointer ring (<=1024
   input nodes). NULL with errorOK means clipped away. Output borrows input and
   context scratch, valid until next call; output cannot be reused as next input.
   Input projected flags/cache coordinates are honored. Image may be NULL;
   image.unused54/padding encode original projected-UV flag and6 float terms. */
BHClipVertex *__cdecl bh_clip_polygon(BHClipContext *, BHClipVertex *, const BHTextureImage *);
#endif
