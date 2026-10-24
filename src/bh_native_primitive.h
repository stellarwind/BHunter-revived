#ifndef BH_NATIVE_PRIMITIVE_H
#define BH_NATIVE_PRIMITIVE_H
#include "bh_clipper.h"
typedef struct BHNativePrimitive {
    BHTextureContext *textures;
    float near_z;
    unsigned long polygons,vertices;
} BHNativePrimitive;
/* Explicit native-only composition adapter implementing0046e390's reviewed
   native TL conversion through real recovered queue owners. Whole0046e390 is
   NOT recovered: its software dispatcher/rasterizers remain unfinished. This
   adapter rejects software binding rather than silently consuming its input.
   Callback signature is shared with source model traversal. Rings/images are
   borrowed; no queue publication on source guard/temporary allocation failure.
   Native depth is near_z/z (GREATEREQUAL); rhw is1/z, raw color is preserved. */
int __cdecl bh_native_primitive_emit(BHClipVertex *,BHTextureImage *,unsigned long color,void *user);
#endif
