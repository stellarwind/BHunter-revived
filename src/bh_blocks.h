#ifndef BH_BLOCKS_H
#define BH_BLOCKS_H
#include "bh_bsp.h"
typedef struct BHBlockData
{
    char name[16];
    BHBsp bsp;
    unsigned char *script;
    unsigned long script_bytes; /* retained bytes, explicitly UNAPPLIED */
    int script_present;
} BHBlockData;
typedef struct BHBlockLibrary
{
    BHBlockData *blocks;
    unsigned long count, total_bytes, unapplied_scripts;
    BHSceneAllocator allocator;
    BHBspError error;
} BHBlockLibrary;
/* Data registry adapter for 00411f00; does not install script/entity callbacks
   or resolve texture images. Names remain in original order; first match wins.
   Original limits: 600 entries, 16-byte names. Strict input safety limits:
   64 KiB text/sidecar, 511 bytes per line, 64 MiB aggregate stored payload.
   Prefix is the explicit directory string, including its trailing separator.
   Initialize once, dispose before reuse; allocator lifetime stays with caller. */
int __cdecl bh_blocks_init(BHBlockLibrary *library, const BHSceneAllocator *allocator);
void __cdecl bh_blocks_dispose(BHBlockLibrary *library);
int __cdecl bh_blocks_decode_names(BHBlockLibrary *library, const void *bytes, unsigned long size);
int __cdecl bh_blocks_load(BHBlockLibrary *library, const char *list_path, const char *prefix, BHBspSearch search);
long __cdecl bh_blocks_find(const BHBlockLibrary *library, const char *name);
#endif
