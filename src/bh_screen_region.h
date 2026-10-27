#ifndef BH_SCREEN_REGION_H
#define BH_SCREEN_REGION_H
#include "bh_screen_quad.h"
/* Whole00454fc0: region UVs are start/extent, flips apply after sum spills;
   quarter-turn transposes the off-diagonal corners. Original cull, rotation,
   affine screen clip and modulation retained. Clip uses live near depth;
   emitted vertices receive caller output_depth, including scratch vertices.
   Selected real emitter consumes ring synchronously. Moderate finite PC53;
   extreme trig and out-of-range FTOL reject explicitly. */
int __cdecl bh_screen_region_draw(BHScreenQuadContext *, BHTextureImage *, float x, float y, float width, float height,
                                  float angle, unsigned char flags, float modulation, float u, float u_extent, float v,
                                  float v_extent, float output_depth);
#endif
