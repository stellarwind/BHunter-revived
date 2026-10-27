#ifndef BH_SOFTWARE_TABLES_H
#define BH_SOFTWARE_TABLES_H
#include "bh_texture.h"
/* Explicit owning bindings for0046e600's four global allocations. All planes
   are row-major [outer256][palette256], palette is original fixed6x7x6 RGB.
   shade tables have intensity then RGB332-modulation planes. Blend tables
   have screen,inverse multiply,saturating add,then inner weights1..5 /6.
   Indexed blend outer is6x7x6 palette; native blend outer is coarse RGB332
   (components224/224/192 at maximum, deliberately not normalized to255). */
typedef struct BHSoftwareTables
{
    unsigned char *shade_indexed; /*004e3858:128KiB, always present */
    unsigned char *blend_indexed; /*004e2df4:512KiB, indexed mode only */
    unsigned short *shade_native; /*004e2de4:256KiB, native16 mode only */
    unsigned short *blend_native; /*004e38c8:1MiB, native16 mode only */
    unsigned long bytes_per_pixel, rgb332_mask[3];
    long rgb332_shift[3];
    BHSceneAllocator allocator;
} BHSoftwareTables;
/* Complete0046e600 with explicit globals. Zero initialize owner first. Uses
   real q allocator and admitted bh_texture_tables_init. bytes_per_pixel1 is
   indexed;2 is native16. Native masks must be contiguous/disjoint <=16bit,
   have3..8 R/G bits and2..8 B bits, matching q display shifts. Source guards
   reject malformed masks (original division loops could hang), invalid/live
   owners and allocation failure. q's table initialization still latches. */
int __cdecl bh_software_tables_initialize(BHSoftwareTables *, BHTextureContext *, unsigned long bytes_per_pixel,
                                          const unsigned long masks[3]);
/* Explicit source lifetime adapter, not an original destructor claim. */
void __cdecl bh_software_tables_dispose(BHSoftwareTables *);
#endif
