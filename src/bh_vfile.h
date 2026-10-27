#ifndef BH_VFILE_H
#define BH_VFILE_H

#include <stdio.h>

/* Observed x86 layouts; analytical names, historical provenance unresolved. */
typedef struct BHArchiveEntry
{
    char name[248];
    unsigned long begin, end;
} BHArchiveEntry;

typedef struct BHVFile
{
    FILE *file;
    int archive;
    long begin, end, position;
    int eof;
    unsigned char *memory;
    struct BHVFile *next;
} BHVFile;

typedef void(__cdecl *BHVFileProgress)(float fraction);
typedef enum BHVFileError
{
    BH_VFILE_OK,
    BH_VFILE_ARGUMENT,
    BH_VFILE_IO,
    BH_VFILE_FORMAT,
    BH_VFILE_MEMORY,
    BH_VFILE_BUSY,
    BH_VFILE_NOT_FOUND,
    BH_VFILE_OVERFLOW
} BHVFileError;

int __cdecl bh_vfile_mount(const char *path);                       /* 0047d270 */
BHVFile *__cdecl bh_vfile_open(const char *name, const char *mode); /* 0047cbd0 */
int __cdecl bh_vfile_close(BHVFile *file);                          /* 0047ce50 */
int __cdecl bh_vfile_eof(BHVFile *file);                            /* 0047cea0 */
int __cdecl bh_vfile_seek(BHVFile *file, long offset, int origin);  /* 0047cec0 */
unsigned long __cdecl bh_vfile_read(void *buffer, unsigned long size, unsigned long count,
                                    BHVFile *file);               /* 0047cf60 */
long __cdecl bh_vfile_tell(BHVFile *file);                        /* 0047d230 */
int __cdecl bh_vfile_preload(const char *name, const char *mode); /* 0047ca70 */
/* Archive/memory mode reads a window of up to 'window' bytes, as observed.
   Supply window+1 bytes so a full window can also be safely terminated.
   Loose mode retains fgets(window) semantics. Zero window is rejected. */
char *__cdecl bh_vfile_gets(char *buffer, unsigned long window, BHVFile *file); /* 0047d050 */

/* Explicit source lifecycle/configuration support; no original recovery claim. */
int __cdecl bh_vfile_detach_archive(void);
void __cdecl bh_vfile_shutdown(void);
void __cdecl bh_vfile_set_progress(BHVFileProgress callback);
void __cdecl bh_vfile_set_debug(unsigned int flags);
BHVFileError __cdecl bh_vfile_error(void);
unsigned long __cdecl bh_vfile_entry_count(void);

#endif
