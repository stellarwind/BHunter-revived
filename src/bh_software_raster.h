#ifndef BH_SOFTWARE_RASTER_H
#define BH_SOFTWARE_RASTER_H
#include "bh_clipper.h"
#include "bh_software_tables.h"
#include "bh_horizon_color.h"

typedef enum BHRasterError
{
    BH_RASTER_OK,
    BH_RASTER_ARGUMENT,
    BH_RASTER_TOPOLOGY,
    BH_RASTER_NUMERICAL,
    BH_RASTER_BOUNDS
} BHRasterError;
typedef struct BHSoftwareRaster
{
    unsigned char *pixels;
    long pitch;
    unsigned short *depth;
    long depth_pitch;
    unsigned long width, height;
    const unsigned char *texels;
    unsigned long texture_width;
    const BHTextureImage *image;
    const BHSoftwareTables *tables;
    const BHHorizonColor *gradient;
    float near_z;
    unsigned long shade_offset, blend_plane, attribute, attribute_row;
    unsigned long spans, visited, written;
    BHRasterError error;
    double rejected_value; /* last masked-invalid signed64 conversion */
    unsigned long masked_conversions;
} BHSoftwareRaster;
typedef int(__cdecl *BHRasterKernel)(BHSoftwareRaster *, BHClipVertex *);
/* Whole0045e790..0046cd20:28 original software polygon callback owners.
   Borrow an already locked tightly packed square power-of-two indexed texture,
   actual image palette maps, actual software/gradient table owners and frame
   color/depth storage. Pitches are signed BYTE pitches; pixels/depth point to
   logical row0. All accessible rows and lookup tables outlive the call.
   Convex clockwise screen ring <=1024nodes, finite positive depth, PC53/nearest
   masked profile. Source validates topology, framebuffer bounds and finite
   signed32 conversion domain. It does not clip, lock, allocate or mutate rings.
   Integer Y samples are y+1, X/UV have original no-subpixel-correction behavior.
   Depth uses near/z*2^24 then low16 of >>8; equality passes. The third depth
   policy always writes, even when an existing depth is nearer. Blends never
   write depth and retain BOTH raw-index-zero and mapped-index-zero rejection. */
#define BH_RASTER_KERNELS(X, P)                                                                                        \
    X(P, plain_test_write)                                                                                             \
    X(P, plain_test)                                                                                                   \
    X(P, plain_write)                                                                                                  \
    X(P, masked_test_write)                                                                                            \
    X(P, masked_test)                                                                                                  \
    X(P, masked_write)                                                                                                 \
    X(P, shade_test_write)                                                                                             \
    X(P, shade_test)                                                                                                   \
    X(P, shade_write)                                                                                                  \
    X(P, masked_shade_test_write)                                                                                      \
    X(P, masked_shade_test)                                                                                            \
    X(P, masked_shade_write)                                                                                           \
    X(P, blend)                                                                                                        \
    X(P, shade_blend)
#define BH_RASTER_DECLARE(P, N) int __cdecl bh_raster_##P##_##N(BHSoftwareRaster *, BHClipVertex *);
BH_RASTER_KERNELS(BH_RASTER_DECLARE, indexed)
BH_RASTER_KERNELS(BH_RASTER_DECLARE, packed)
#undef BH_RASTER_DECLARE
#endif
