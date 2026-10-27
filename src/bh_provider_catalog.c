#include "bh_provider_catalog.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
typedef char provider_record_size[sizeof(void *) == 4 && sizeof(BHProviderRecord) == 188 &&
                                          offsetof(BHProviderRecord, connection) == 16 &&
                                          offsetof(BHProviderRecord, kind) == 24
                                      ? 1
                                      : -1];
static void *__cdecl allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void __cdecl release(void *pointer, void *user)
{
    free(pointer);
}
int __cdecl bh_provider_catalog_init(BHProviderCatalog *catalog, const BHSceneAllocator *allocator)
{
    if (!catalog || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(catalog, 0, sizeof(*catalog));
    if (allocator)
        catalog->allocator = *allocator;
    else
    {
        catalog->allocator.allocate = allocate;
        catalog->allocator.release = release;
    }
    return 1;
}
void __cdecl bh_provider_catalog_dispose(BHProviderCatalog *catalog)
{
    long i;
    if (!catalog || !catalog->allocator.release || catalog->count < 0 || catalog->count > 8)
        return;
    for (i = 0; i < catalog->count; ++i)
        if (catalog->records[i].connection)
            catalog->allocator.release(catalog->records[i].connection, catalog->allocator.user);
    memset(catalog, 0, sizeof(*catalog));
}
BOOL FAR PASCAL bh_provider_catalog_callback(LPCGUID provider, LPVOID bytes, DWORD size, LPCDPNAME name, DWORD flags,
                                             LPVOID user)
{
    static const unsigned char ids[4][16] = {
        {0x60, 0xa7, 0xea, 0x44, 0x68, 0xcb, 0xcf, 0x11, 0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e},
        {0xe0, 0x5e, 0xe9, 0x36, 0x77, 0x85, 0xcf, 0x11, 0x96, 0x0c, 0x00, 0x80, 0xc7, 0x53, 0x4e, 0x82},
        {0x00, 0xc4, 0x5b, 0x68, 0x2c, 0x9d, 0xcf, 0x11, 0xa9, 0xcd, 0x00, 0xaa, 0x00, 0x68, 0x86, 0xe3},
        {0x60, 0x68, 0x1d, 0x0f, 0xd9, 0x88, 0xcf, 0x11, 0x9c, 0x4e, 0x00, 0xa0, 0xc9, 0x05, 0x42, 0x5e}};
    static const long kinds[4] = {2, 1, 0, 3};
    static const char *labels[4] = {"Modem-to-Modem", "TCP/IP Internet", "IPX Local Network", "Serial Cable"};
    BHProviderCatalog *catalog = (BHProviderCatalog *)user;
    BHProviderRecord *record;
    void *copy;
    char *label;
    unsigned long i;
    (void)flags;
    if (!catalog || !catalog->allocator.allocate || !provider || !name || !name->lpszShortNameA || (!bytes && size) ||
        catalog->count < 0 || catalog->count >= 8)
        return FALSE;
    copy = catalog->allocator.allocate(size, catalog->allocator.user);
    if (!copy && size)
    {
        catalog->allocation_failed = 1;
        return FALSE;
    }
    record = &catalog->records[catalog->count];
    record->provider = *provider;
    record->connection = copy;
    record->reserved = 0;
    if (size)
        memcpy(copy, bytes, size);
    label = (char *)record + 156;
    strncpy(label, name->lpszShortNameA, 20);
    label[20] = 0;
    record->kind = 4;
    for (i = 0; i < 4; ++i)
        if (!memcmp(provider, ids[i], 16))
        {
            record->kind = kinds[i];
            strcpy(label, labels[i]);
            break;
        }
    catalog->connection_bytes[catalog->count] = size;
    ++catalog->count;
    return catalog->count < 8;
}
