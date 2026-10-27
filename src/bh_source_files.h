#ifndef BH_SOURCE_FILES_H
#define BH_SOURCE_FILES_H
#include <windows.h>
typedef struct BHSourceFiles
{
    HKEY root;
    const char *subkey;
    unsigned long expected_checksum;
    unsigned long checksum, bytes, blocks;
    LONG registry_result;
    int error;
} BHSourceFiles;
/* Count-zero defaults adapter: original HKLM Paths/SourceDir and checksum. */
void __cdecl bh_source_files_init(BHSourceFiles *);
/* Whole00473a40: query SourceDir twice, open copyright.txt as binary and XOR
   complete little-endian dwords in each <=65536-byte block. Trailing1..3bytes
   are deliberately ignored. BIP_B1/B2 names are constructed but not opened,
   as in the original. Native registry/files are read-only; source closes the
   registry handle and rejects allocation/short-read/unterminated-path errors
   rather than reproducing the original leak/unchecked malformed-input paths. */
int __cdecl bh_source_files_check(BHSourceFiles *);
#endif
