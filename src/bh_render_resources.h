#ifndef BH_RENDER_RESOURCES_H
#define BH_RENDER_RESOURCES_H
#include "bh_draw_resource.h"

typedef struct BHRenderResourceEntry BHRenderResourceEntry;
typedef enum BHRenderResourcesError
{
    BH_RENDER_RESOURCES_OK,
    BH_RENDER_RESOURCES_ARGUMENT,
    BH_RENDER_RESOURCES_MODEL,
    BH_RENDER_RESOURCES_MEMORY,
    BH_RENDER_RESOURCES_FULL
} BHRenderResourcesError;

/* Source-owned model resource adapter. Zero-initialize before first init;
   dispose before reinitializing. Init rejects a populated owner unchanged.
   Required prototype camera/config/clip/emitter and enabled light bindings
   must be present before any model descriptor can be published.
   The model context and all borrowed
   bindings in prototype must outlive this owner. A queued command borrows its
   handle: drain the queue before disposing the owner. Handles are slot+1,
   stable until disposal, and zero is invalid. */
typedef struct BHRenderResources
{
    BHRenderResourceEntry **entries;
    unsigned long count, capacity;
    BHSceneAllocator allocator;
    BHModelContext *models;
    BHModelInstanceContext prototype;
    BHRenderResourcesError error;
} BHRenderResources;

int __cdecl bh_render_resources_init(BHRenderResources *owner, BHModelContext *models,
                                     const BHModelInstanceContext *prototype, const BHSceneAllocator *allocator);
unsigned long __cdecl bh_render_resources_load_model(BHRenderResources *owner, const char *name);
const BHDrawResource *__cdecl bh_render_resources_lookup(unsigned long handle, void *owner);
void __cdecl bh_render_resources_dispose(BHRenderResources *owner);
#endif
