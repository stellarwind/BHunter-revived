#include "bh_blocks.h"
#include <stdlib.h>
#include <string.h>

static void *__cdecl allocate(unsigned long n, void *user)
{
    return malloc(n);
}
static void __cdecl release(void *p, void *user)
{
    free(p);
}
static int fail(BHBlockLibrary *library, BHBspError error)
{
    if (library)
        library->error = error;
    return 0;
}

int __cdecl bh_blocks_init(BHBlockLibrary *library, const BHSceneAllocator *allocator)
{
    BHSceneAllocator memory;
    if (!library || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    if (allocator)
        memory = *allocator;
    else
    {
        memory.allocate = allocate;
        memory.release = release;
        memory.user = NULL;
    }
    memset(library, 0, sizeof(*library));
    library->allocator = memory;
    return 1;
}
void __cdecl bh_blocks_dispose(BHBlockLibrary *library)
{
    unsigned long i;
    BHSceneAllocator memory;
    if (!library)
        return;
    memory = library->allocator;
    for (i = 0; i < library->count; i++)
    {
        bh_bsp_dispose(&library->blocks[i].bsp);
        if (library->blocks[i].script)
            memory.release(library->blocks[i].script, memory.user);
    }
    if (library->blocks)
        memory.release(library->blocks, memory.user);
    memset(library, 0, sizeof(*library));
    library->allocator = memory;
}

static int names(const unsigned char *bytes, unsigned long size, BHBlockData *output, unsigned long *count)
{
    unsigned long at = 0, start, length, i, n = 0;
    char name[16];
    while (at < size)
    {
        start = at;
        while (at < size && bytes[at] != '\r' && bytes[at] != '\n')
        {
            if (!bytes[at])
                return BH_BSP_FORMAT;
            ++at;
        }
        length = at - start;
        if (length > 511)
            return BH_BSP_LIMIT;
        if (at < size)
        {
            unsigned char first = bytes[at++];
            if (at < size && ((first == '\r' && bytes[at] == '\n') || (first == '\n' && bytes[at] == '\r')))
                ++at;
        }
        if (!length || bytes[start] == ';')
            continue;
        if (length == 3 && (bytes[start] == 'e' || bytes[start] == 'E') &&
            (bytes[start + 1] == 'n' || bytes[start + 1] == 'N') &&
            (bytes[start + 2] == 'd' || bytes[start + 2] == 'D'))
            break;
        if (length >= 16 || length < 5)
            return BH_BSP_FORMAT;
        for (i = 0; i < length; i++)
            if (bytes[start + i] <= 32 || bytes[start + i] >= 127 || bytes[start + i] == '/' ||
                bytes[start + i] == '\\' || bytes[start + i] == ':')
                return BH_BSP_FORMAT;
        memcpy(name, bytes + start, length);
        name[length] = 0;
        if (_stricmp(name + length - 4, ".bsp"))
            return BH_BSP_FORMAT;
        if (n == 600)
            return BH_BSP_LIMIT;
        if (output)
            memcpy(output[n].name, name, length + 1);
        ++n;
    }
    *count = n;
    return BH_BSP_OK;
}

int __cdecl bh_blocks_decode_names(BHBlockLibrary *library, const void *bytes, unsigned long size)
{
    BHBlockLibrary next;
    unsigned long count, i;
    int error;
    if (!library || !library->allocator.allocate || !bytes)
        return fail(library, BH_BSP_ARGUMENT);
    if (size > 65536)
        return fail(library, BH_BSP_LIMIT);
    error = names((const unsigned char *)bytes, size, NULL, &count);
    if (error)
        return fail(library, (BHBspError)error);
    bh_blocks_init(&next, &library->allocator);
    if (count)
    {
        next.blocks = (BHBlockData *)next.allocator.allocate(count * sizeof(BHBlockData), next.allocator.user);
        if (!next.blocks)
            return fail(library, BH_BSP_MEMORY);
        memset(next.blocks, 0, count * sizeof(BHBlockData));
        next.count = count;
        for (i = 0; i < count; i++)
            bh_bsp_init(&next.blocks[i].bsp, &next.allocator);
        names((const unsigned char *)bytes, size, next.blocks, &count);
    }
    bh_blocks_dispose(library);
    *library = next;
    return 1;
}
long __cdecl bh_blocks_find(const BHBlockLibrary *library, const char *name)
{
    unsigned long i;
    if (!library || !name)
        return -1;
    for (i = 0; i < library->count; i++)
        if (!_stricmp(library->blocks[i].name, name))
            return (long)i;
    return -1;
}

/* A private complete text read: caller closes the handle. No text-mode CRLF
   translation, so sidecars retain exact archive bytes for later semantics. */
static int text(BHBlockLibrary *library, BHVFile *file, unsigned char **bytes, unsigned long *size)
{
    long length;
    if (bh_vfile_seek(file, 0, SEEK_END) || (length = bh_vfile_tell(file)) < 0 || bh_vfile_seek(file, 0, SEEK_SET))
        return fail(library, BH_BSP_IO);
    if (length > 65536)
        return fail(library, BH_BSP_LIMIT);
    *bytes = (unsigned char *)library->allocator.allocate(length ? (unsigned long)length : 1, library->allocator.user);
    if (!*bytes)
        return fail(library, BH_BSP_MEMORY);
    *size = (unsigned long)length;
    if (bh_vfile_read(*bytes, 1, *size, file) != *size || bh_vfile_error() != BH_VFILE_OK)
    {
        library->allocator.release(*bytes, library->allocator.user);
        *bytes = NULL;
        return fail(library, BH_BSP_IO);
    }
    return 1;
}

int __cdecl bh_blocks_load(BHBlockLibrary *library, const char *list_path, const char *prefix, BHBspSearch search)
{
    BHBlockLibrary next;
    BHVFile *file;
    unsigned char *bytes = NULL;
    unsigned long size, i, prefix_length = 0, name_length;
    char sidecar[256];
    int ok;
    BHBspError error;
    if (!library || !library->allocator.allocate || !list_path || !prefix || search < BH_BSP_BARE_ONLY ||
        search > BH_BSP_PREFIX_ONLY)
        return fail(library, BH_BSP_ARGUMENT);
    while (prefix_length < 256 && prefix[prefix_length])
        ++prefix_length;
    if (prefix_length > 239)
        return fail(library, BH_BSP_LIMIT);
    bh_blocks_init(&next, &library->allocator);
    file = bh_vfile_open(list_path, "rb");
    if (!file)
        return fail(library, BH_BSP_IO);
    ok = text(&next, file, &bytes, &size);
    if (bh_vfile_close(file) && ok)
    {
        next.error = BH_BSP_IO;
        ok = 0;
    }
    if (ok)
        ok = bh_blocks_decode_names(&next, bytes, size);
    if (bytes)
        next.allocator.release(bytes, next.allocator.user);
    if (!ok)
        goto rejected;
    for (i = 0; i < next.count; i++)
    {
        if (!bh_bsp_load(&next.blocks[i].bsp, next.blocks[i].name, prefix, search))
        {
            next.error = next.blocks[i].bsp.error;
            goto rejected;
        }
        next.total_bytes += next.blocks[i].bsp.bytes;
        if (next.total_bytes > 64UL * 1024 * 1024)
        {
            next.error = BH_BSP_LIMIT;
            goto rejected;
        }
        name_length = (unsigned long)strlen(next.blocks[i].name);
        memcpy(sidecar, prefix, prefix_length);
        memcpy(sidecar + prefix_length, next.blocks[i].name, name_length + 1);
        memcpy(sidecar + prefix_length + name_length - 4, ".TXT", 5);
        file = bh_vfile_open(sidecar, "rb");
        if (!file)
        {
            if (bh_vfile_error() != BH_VFILE_NOT_FOUND)
            {
                next.error = BH_BSP_IO;
                goto rejected;
            }
            continue;
        }
        ok = text(&next, file, &next.blocks[i].script, &next.blocks[i].script_bytes);
        if (bh_vfile_close(file) && ok)
        {
            next.error = BH_BSP_IO;
            ok = 0;
        }
        if (!ok)
            goto rejected;
        next.blocks[i].script_present = 1;
        ++next.unapplied_scripts;
        next.total_bytes += next.blocks[i].script_bytes;
        if (next.total_bytes > 64UL * 1024 * 1024)
        {
            next.error = BH_BSP_LIMIT;
            goto rejected;
        }
    }
    bh_blocks_dispose(library);
    *library = next;
    return 1;
rejected:
    error = next.error;
    bh_blocks_dispose(&next);
    return fail(library, error);
}
