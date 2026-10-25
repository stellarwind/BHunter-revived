#ifndef BH_PCX_H
#define BH_PCX_H
#include "bh_scene.h"
typedef enum BHPcxError {
    BH_PCX_OK, BH_PCX_ARGUMENT, BH_PCX_TRUNCATED, BH_PCX_FORMAT,
    BH_PCX_UNSUPPORTED, BH_PCX_LIMIT, BH_PCX_MEMORY, BH_PCX_IO
} BHPcxError;
typedef struct BHPcx {
    unsigned char *pixels;
    unsigned char palette[768]; /* original RGB triples, not transformed */
    unsigned long width,height,pixel_bytes;
    BHPcxError error;
    BHSceneAllocator allocator;
} BHPcx;
typedef struct BHPcxPalette {
    unsigned char rgb[768];
    unsigned char indices[256];
} BHPcxPalette;
/* Fresh owner initialization. Transactional replacement on successful decode;
   input must not overlap owned pixels. Supported original asset dialect:
   v5, RLE, indexed 8-bit, one plane, zero origin, bytes-per-line == width.
   Limits: 4096 each dimension, 16 MiB pixels, 64 MiB input. */
int __cdecl bh_pcx_init(BHPcx *image,const BHSceneAllocator *allocator);
void __cdecl bh_pcx_dispose(BHPcx *image);
int __cdecl bh_pcx_decode(BHPcx *image,const void *bytes,unsigned long size);
/* Borrow remaining VFILE payload, success at end; rejection attempts cursor
   restoration. Caller owns serialized handle/cache lifetime and closes it. */
int __cdecl bh_pcx_read(BHPcx *image,BHVFile *file);
/* Complete 0044f1c0 transform, expressed with explicit owning buffers.
   Initialize identity map, then mode 0: reserve zero/brighten dark colors;
   mode 2: blacken very dark colors/map black to zero; all other modes:
   reserve caller's packed 0xRRGGBB color, brighten dark nonzero entries.
   Input may be output.rgb; copied before publishing output. */
int __cdecl bh_pcx_palette(BHPcxPalette *output,const unsigned char *rgb,
                         int mode,unsigned long color);
#endif
