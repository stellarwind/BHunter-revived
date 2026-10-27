#ifndef BH_HORIZON_COLOR_H
#define BH_HORIZON_COLOR_H
#include "bh_texture.h"
typedef enum BHHorizonColorError
{
    BH_HORIZON_COLOR_OK,
    BH_HORIZON_COLOR_ARGUMENT,
    BH_HORIZON_COLOR_MEMORY
} BHHorizonColorError;
typedef struct BHHorizonColor
{
    BHTextureContext *textures;
    unsigned char *indexed;
    unsigned short *packed;
    unsigned long color, fills;
    BHSceneAllocator allocator;
    BHHorizonColorError error;
} BHHorizonColor;
/* Fresh source owning adapters; borrowed texture context/allocator user outlive
   this owner. Dispose before reinitializing. No original startup claim. */
int __cdecl bh_horizon_color_init(BHHorizonColor *, BHTextureContext *);
void __cdecl bh_horizon_color_dispose(BHHorizonColor *);
/* Complete0045d7a0. Exactly mode1 produces indexed256x256 bytes; every other
   mode produces packed256x256 unsigned16 values. Both buffers share the same
   full32bit key; cross-mode stale cache behavior is intentionally preserved.
   RGB math ignores the top byte, key comparisons do not. Display shifts are
   read live only on packed misses and retain x86 low5bit count masking.
   Actual texture table initializer runs before even a cache hit. Source null
   allocation/binding guards report0; original allocation failure was unchecked. */
int __cdecl bh_horizon_color_prepare(BHHorizonColor *, unsigned long mode, unsigned long color);
#endif
