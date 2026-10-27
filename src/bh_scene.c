#include "bh_scene.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

typedef char placement_size[sizeof(BHScenePlacement) == 32 && offsetof(BHScenePlacement, x) == 16 ? 1 : -1];
typedef char link_size[sizeof(BHSceneLink) == 20 && sizeof(short) == 2 ? 1 : -1];
typedef char route_size[sizeof(BHSceneRoute) == 8 && offsetof(BHSceneRoute, distance) == 4 ? 1 : -1];
#define MAX_INPUT (64UL * 1024 * 1024)
#define MAX_RECORDS 65535UL
#define MAX_NODES 2048UL

static void *__cdecl allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void __cdecl release(void *pointer, void *user)
{
    free(pointer);
}
static unsigned long get32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}
static int setup(BHSceneAllocator *output, const BHSceneAllocator *input)
{
    if (input)
    {
        if (!input->allocate || !input->release)
            return 0;
        *output = *input;
    }
    else
    {
        output->allocate = allocate;
        output->release = release;
        output->user = NULL;
    }
    return 1;
}
int __cdecl bh_scene_level_init(BHSceneLevel *level, const BHSceneAllocator *allocator)
{
    BHSceneAllocator memory;
    if (!level || !setup(&memory, allocator))
        return 0;
    memset(level, 0, sizeof(*level));
    level->allocator = memory;
    return 1;
}
int __cdecl bh_scene_routes_init(BHSceneRoutes *routes, const BHSceneAllocator *allocator)
{
    BHSceneAllocator memory;
    if (!routes || !setup(&memory, allocator))
        return 0;
    memset(routes, 0, sizeof(*routes));
    routes->allocator = memory;
    return 1;
}
void __cdecl bh_scene_level_dispose(BHSceneLevel *level)
{
    if (!level)
        return;
    if (level->placements)
        level->allocator.release(level->placements, level->allocator.user);
    if (level->links)
        level->allocator.release(level->links, level->allocator.user);
    level->placements = NULL;
    level->links = NULL;
    level->placement_count = level->link_count = 0;
}
void __cdecl bh_scene_routes_dispose(BHSceneRoutes *routes)
{
    if (!routes)
        return;
    if (routes->cells)
        routes->allocator.release(routes->cells, routes->allocator.user);
    routes->cells = NULL;
    routes->node_count = 0;
}
static int level_fail(BHSceneLevel *level, BHSceneError error)
{
    if (level)
        level->error = error;
    return 0;
}
static int routes_fail(BHSceneRoutes *routes, BHSceneError error)
{
    if (routes)
        routes->error = error;
    return 0;
}

int __cdecl bh_scene_level_decode(BHSceneLevel *level, const void *bytes, unsigned long size)
{
    const unsigned char *data = (const unsigned char *)bytes;
    unsigned long count, offset, links, i;
    BHSceneLevel next;
    if (!level || !level->allocator.allocate || !data)
        return level_fail(level, BH_SCENE_ARGUMENT);
    if (size > MAX_INPUT)
        return level_fail(level, BH_SCENE_LIMIT);
    if (size < 4)
        return level_fail(level, BH_SCENE_TRUNCATED);
    count = get32(data);
    if (count & 0x80000000UL)
        return level_fail(level, BH_SCENE_FORMAT);
    if (count > MAX_RECORDS)
        return level_fail(level, BH_SCENE_LIMIT);
    if (count > (size - 4) / 32)
        return level_fail(level, BH_SCENE_TRUNCATED);
    offset = 4 + count * 32;
    if ((size - offset) % 20)
        return level_fail(level, BH_SCENE_TRUNCATED);
    links = (size - offset) / 20;
    if (links > MAX_RECORDS)
        return level_fail(level, BH_SCENE_LIMIT);
    for (i = 0; i < count; i++)
        if (!memchr(data + 4 + i * 32, 0, 16))
            return level_fail(level, BH_SCENE_FORMAT);
    for (i = 0; i < links; i++)
        if (get32(data + offset + i * 20) >= count)
            return level_fail(level, BH_SCENE_FORMAT);
    bh_scene_level_init(&next, &level->allocator);
    if (count)
    {
        next.placements = (BHScenePlacement *)next.allocator.allocate(count * 32, next.allocator.user);
        if (!next.placements)
            return level_fail(level, BH_SCENE_MEMORY);
        memcpy(next.placements, data + 4, count * 32);
        next.placement_count = count;
    }
    if (links)
    {
        next.links = (BHSceneLink *)next.allocator.allocate(links * 20, next.allocator.user);
        if (!next.links)
        {
            bh_scene_level_dispose(&next);
            return level_fail(level, BH_SCENE_MEMORY);
        }
        memcpy(next.links, data + offset, links * 20);
        next.link_count = links;
    }
    bh_scene_level_dispose(level);
    *level = next;
    return 1;
}

int __cdecl bh_scene_routes_decode(BHSceneRoutes *routes, const void *bytes, unsigned long size,
                                   unsigned long expected_nodes)
{
    const unsigned char *data = (const unsigned char *)bytes;
    unsigned long count, cells, required, i, slot, distance, magnitude;
    BHSceneRoutes next;
    if (!routes || !routes->allocator.allocate || !data)
        return routes_fail(routes, BH_SCENE_ARGUMENT);
    if (size > MAX_INPUT || expected_nodes > MAX_NODES)
        return routes_fail(routes, BH_SCENE_LIMIT);
    if (size < 4)
        return routes_fail(routes, BH_SCENE_TRUNCATED);
    count = get32(data);
    if (count & 0x80000000UL)
        return routes_fail(routes, BH_SCENE_FORMAT);
    if (count > MAX_NODES)
        return routes_fail(routes, BH_SCENE_LIMIT);
    if (count != expected_nodes)
        return routes_fail(routes, BH_SCENE_COUNT_MISMATCH);
    cells = count * count;
    required = 4 + cells * 8; /* count cap bounds both products */
    if (size < required)
        return routes_fail(routes, BH_SCENE_TRUNCATED);
    if (size > required)
        return routes_fail(routes, BH_SCENE_FORMAT);
    for (i = 0; i < cells; i++)
    {
        slot = get32(data + 4 + i * 8);
        distance = get32(data + 8 + i * 8);
        magnitude = distance & 0x7fffffffUL;
        if ((slot != 0xffffffffUL && slot > 5) || magnitude >= 0x7f800000UL || ((distance & 0x80000000UL) && magnitude))
            return routes_fail(routes, BH_SCENE_FORMAT);
    }
    bh_scene_routes_init(&next, &routes->allocator);
    if (cells)
    {
        next.cells = (BHSceneRoute *)next.allocator.allocate(cells * 8, next.allocator.user);
        if (!next.cells)
            return routes_fail(routes, BH_SCENE_MEMORY);
        memcpy(next.cells, data + 4, cells * 8);
        next.node_count = count;
    }
    bh_scene_routes_dispose(routes);
    *routes = next;
    return 1;
}

/* Read the remaining payload once. VFILE seek/read handles each backend's
   cursor, including an archive's shared FILE and cached-handle behavior. */
static int blob(BHVFile *file, const BHSceneAllocator *memory, unsigned char **data, unsigned long *size, long *start,
                BHSceneError *error)
{
    long end;
    *data = NULL;
    if (!file)
    {
        *error = BH_SCENE_ARGUMENT;
        return 0;
    }
    /* Embedded handles share a backing FILE. Re-select this handle's stored
       cursor before tell(), whose original contract observes that FILE. */
    if (bh_vfile_seek(file, 0, SEEK_CUR))
    {
        *error = BH_SCENE_IO;
        return 0;
    }
    *start = bh_vfile_tell(file);
    if (*start < 0 || bh_vfile_seek(file, 0, SEEK_END))
    {
        *error = BH_SCENE_IO;
        return 0;
    }
    end = bh_vfile_tell(file);
    if (bh_vfile_seek(file, *start, SEEK_SET) || end < *start)
    {
        *error = BH_SCENE_IO;
        return 0;
    }
    *size = (unsigned long)(end - *start);
    if (*size > MAX_INPUT)
    {
        *error = BH_SCENE_LIMIT;
        return 0;
    }
    if (!*size)
    {
        *error = BH_SCENE_TRUNCATED;
        return 0;
    }
    *data = (unsigned char *)memory->allocate(*size, memory->user);
    if (!*data)
    {
        *error = BH_SCENE_MEMORY;
        return 0;
    }
    if (bh_vfile_read(*data, 1, *size, file) != *size || bh_vfile_error() != BH_VFILE_OK)
    {
        memory->release(*data, memory->user);
        *data = NULL;
        *error = BH_SCENE_IO;
        return 0;
    }
    return 1;
}

int __cdecl bh_scene_level_read(BHSceneLevel *level, BHVFile *file)
{
    unsigned char *data;
    unsigned long size;
    long start = -1;
    int ok;
    if (!level || !level->allocator.allocate)
        return level_fail(level, BH_SCENE_ARGUMENT);
    ok = blob(file, &level->allocator, &data, &size, &start, &level->error);
    if (ok)
    {
        ok = bh_scene_level_decode(level, data, size);
        level->allocator.release(data, level->allocator.user);
    }
    if (!ok && file && start >= 0 && bh_vfile_seek(file, start, SEEK_SET))
        level->error = BH_SCENE_IO;
    return ok;
}
int __cdecl bh_scene_routes_read(BHSceneRoutes *routes, BHVFile *file, unsigned long expected_nodes)
{
    unsigned char *data;
    unsigned long size;
    long start = -1;
    int ok;
    if (!routes || !routes->allocator.allocate)
        return routes_fail(routes, BH_SCENE_ARGUMENT);
    if (expected_nodes > MAX_NODES)
        return routes_fail(routes, BH_SCENE_LIMIT);
    ok = blob(file, &routes->allocator, &data, &size, &start, &routes->error);
    if (ok)
    {
        ok = bh_scene_routes_decode(routes, data, size, expected_nodes);
        routes->allocator.release(data, routes->allocator.user);
    }
    if (!ok && file && start >= 0 && bh_vfile_seek(file, start, SEEK_SET))
        routes->error = BH_SCENE_IO;
    return ok;
}
