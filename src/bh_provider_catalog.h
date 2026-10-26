#ifndef BH_PROVIDER_CATALOG_H
#define BH_PROVIDER_CATALOG_H
#include <windows.h>
#include <dplay.h>
#include "bh_scene.h"
/* Only verified fields are named. The remaining160 bytes retain original
   profile storage, including the21-byte display name at whole-record+156.
   Other profile fields overlap that region; no new complete layout is guessed. */
typedef struct BHProviderRecord {GUID provider;void *connection;DWORD reserved;long kind;unsigned char data[160];} BHProviderRecord;
typedef struct BHProviderCatalog {
    BHProviderRecord records[8];long count;unsigned long connection_bytes[8];
    BHSceneAllocator allocator;int allocation_failed;
} BHProviderCatalog;
/* Count-zero owning adapters. Initialize fresh storage, dispose before reuse.
   Callback source guards reject invalid/full storage and failed allocation;
   valid callback changes only original verified fields and owned blob metadata. */
int __cdecl bh_provider_catalog_init(BHProviderCatalog *,const BHSceneAllocator *);
void __cdecl bh_provider_catalog_dispose(BHProviderCatalog *);
/* Whole00475d60, actual six-argument x86 WINAPI EnumConnections callback.
   Copies the borrowed connection payload,20-byte ANSI short name then NUL,
   classifies exact original GUIDs, retains original labels/kind0..4, and stops
   after eighth record. Does not initialize any connection or open a session. */
BOOL FAR PASCAL bh_provider_catalog_callback(LPCGUID,LPVOID,DWORD,LPCDPNAME,DWORD,LPVOID);
#endif
