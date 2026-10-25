#ifndef BH_NATIVE_ATTRIBUTE_H
#define BH_NATIVE_ATTRIBUTE_H
#include "bh_native_primitive.h"
/* Explicit hardware-only adapter for 0046e430. Each unknown28 float is the
   modulation consumed by the original native branch. Base color is separate;
   grayscale conversion truncates toward zero with unsigned 32-bit wrapping.
   Software dispatch/rasterization is unfinished and explicitly rejected.
   Borrows the ring/image synchronously; user points to BHNativePrimitive. */
int __cdecl bh_native_attribute_emit(BHClipVertex *,BHTextureImage *,unsigned long base_color,void *user);
#endif
