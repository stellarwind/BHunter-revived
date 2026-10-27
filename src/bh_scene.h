#ifndef BH_SCENE_H
#define BH_SCENE_H
#include "bh_vfile.h"

/* Owned data adapters for reader portions of 004100d0 and 00407f30.
   No entity/graph construction or original global-state ABI is provided. */
typedef struct BHScenePlacement
{
    char name[16];
    long x, y, z;
    unsigned long flags;
} BHScenePlacement;
typedef struct BHSceneLink
{
    unsigned long placement;
    short values[8]; /* opaque per-block payload; NOT eight node indices */
} BHSceneLink;
typedef struct BHSceneRoute
{
    long exit_slot;
    float distance;
} BHSceneRoute;
typedef enum BHSceneError
{
    BH_SCENE_OK,
    BH_SCENE_ARGUMENT,
    BH_SCENE_TRUNCATED,
    BH_SCENE_FORMAT,
    BH_SCENE_LIMIT,
    BH_SCENE_MEMORY,
    BH_SCENE_IO,
    BH_SCENE_COUNT_MISMATCH
} BHSceneError;
typedef struct BHSceneAllocator
{
    void *(__cdecl *allocate)(unsigned long bytes, void *user);
    void(__cdecl *release)(void *pointer, void *user);
    void *user;
} BHSceneAllocator;
typedef struct BHSceneLevel
{
    BHScenePlacement *placements;
    BHSceneLink *links;
    unsigned long placement_count, link_count;
    BHSceneError error;
    BHSceneAllocator allocator; /* initialize once; keep ownership private */
} BHSceneLevel;
typedef struct BHSceneRoutes
{
    BHSceneRoute *cells; /* cells[destination * node_count + source] */
    unsigned long node_count;
    BHSceneError error;
    BHSceneAllocator allocator;
} BHSceneRoutes;

/* Initialize before use, dispose before reinitializing; NULL allocator uses
   real CRT malloc/free. Input buffers must not overlap owned output storage.
   Successful decode replaces old data; failure preserves it and sets error.
   Limits: 65535 placements/links, 2048 route nodes, 64 MiB input. */
int __cdecl bh_scene_level_init(BHSceneLevel *level, const BHSceneAllocator *allocator);
int __cdecl bh_scene_routes_init(BHSceneRoutes *routes, const BHSceneAllocator *allocator);
void __cdecl bh_scene_level_dispose(BHSceneLevel *level);
void __cdecl bh_scene_routes_dispose(BHSceneRoutes *routes);
int __cdecl bh_scene_level_decode(BHSceneLevel *level, const void *bytes, unsigned long size);
/* expected_nodes comes from the owning navigation graph. There is no silent
   mismatch fallback: graph construction/route recomputation are unrecovered. */
int __cdecl bh_scene_routes_decode(BHSceneRoutes *routes, const void *bytes, unsigned long size,
                                   unsigned long expected_nodes);
/* Borrow an open VFILE, read its remaining payload, never close it. Success
   leaves it at end; rejection attempts to restore its original cursor.
   The caller retains handle/cache lifetime and must serialize shared use. */
int __cdecl bh_scene_level_read(BHSceneLevel *level, BHVFile *file);
int __cdecl bh_scene_routes_read(BHSceneRoutes *routes, BHVFile *file, unsigned long expected_nodes);
#endif
