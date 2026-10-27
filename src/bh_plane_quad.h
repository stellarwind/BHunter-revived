#ifndef BH_PLANE_QUAD_H
#define BH_PLANE_QUAD_H
#include "bh_quad_queue.h"
/* Complete00456d30 full-image arbitrary world plane quad. Explicit context
   binds the same camera/clip/lighting globals as the arbitrary-UV owner, but
   this is a separately reviewed original boundary and source implementation.
   center/normal/up copy before writes. Original normal negation, backface bit8,
   Gram-Schmidt, rotation, UV flip/transpose, clipping, point/directional light,
   signed color conversion and missed15th modulation argument are preserved.
   No rasterizer is supplied: synchronous emitter and all borrowed inputs
   follow BHSpriteQuadContext lifetime/nonreentrancy/PC53 finite contracts. */
int __cdecl bh_plane_quad_draw(BHSpriteQuadContext *, BHTextureImage *, const BHVec3 *center, const BHVec3 *normal,
                               const BHVec3 *up, float width, float height, float angle, unsigned char flags,
                               float modulation);
/* Complete00477710. Decode BH_DRAW_ORIENTED, preserve reordered up/normal,
   truncate raw flags to byte, invoke real full-image owner, then read next.
   Missing image also publishes next and fails. Output must not alias command.
   This callback binds actual image lookup; no original addresses/vtables. */
int __cdecl bh_plane_quad_execute(BHQuadQueueContext *, const BHDrawCommand *, unsigned long *next);
#endif
