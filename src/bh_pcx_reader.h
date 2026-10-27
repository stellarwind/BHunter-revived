#ifndef BH_PCX_READER_H
#define BH_PCX_READER_H
#include "bh_pcx.h"

typedef struct BHPcxReader
{
    BHVFile *file;
    unsigned char palette[768], map[256], buffer[1024];
    unsigned short width, height;
    unsigned long buffered, cursor, compressed_left, pending;
    unsigned char repeated;
    BHPcxError error;
} BHPcxReader;

void __cdecl bh_pcx_reader_init(BHPcxReader *reader);
int __cdecl bh_pcx_reader_close(BHPcxReader *reader);
/* 0044f0a0 over an explicit owning VFILE reader. Close before reopening.
   Checks the original first-12-byte manufacturer/encoding/bit-depth fields;
   version, origin, plane count and line stride are not new restrictions.
   Width/height are original 16-bit xmax+1/ymax+1, including wrap to zero.
   Reads the final 768 palette bytes, resets identity map/run/buffer state and
   seeks to 128. It does not require a palette marker. Failed open closes its
   temporary file and leaves the previous reader unchanged except error.
   Safety extension: >=896 and <=64 MiB total bytes, all reads/seeks bounded. */
int __cdecl bh_pcx_reader_open(BHPcxReader *reader, const char *name);
/* 0044efa0's low-byte pixel behavior, with explicit success/error instead of
   undefined upper return bits. C0 is skipped, C1..FF read a value and repeat it.
   Every output consults current map, including the remainder of an active run.
   1024-byte buffering supports a run prefix/value across refills. Invalid EOF
   before the palette is a sticky error, with output unchanged. No allocation.
   Read exactly the pixel count required by the caller's image/layout contract;
   this low-level reader does not silently normalize PCX rows or image dialects. */
int __cdecl bh_pcx_reader_next(BHPcxReader *reader, unsigned char *pixel);
#endif
