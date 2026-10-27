#include "bh_render_resources.h"
#include <stdlib.h>
#include <string.h>

struct BHRenderResourceEntry
{
    BHModel *model;
    BHModelDrawCache cache;
    BHModelInstanceContext instance;
    BHDrawResource descriptor;
};

static void *__cdecl allocate(unsigned long bytes, void *user)
{
    (void)user;
    return malloc(bytes);
}
static void __cdecl release(void *pointer, void *user)
{
    (void)user;
    free(pointer);
}
static int reserve(BHRenderResources *owner)
{
    BHRenderResourceEntry **next;
    unsigned long capacity;
    if (owner->count < owner->capacity)
        return 1;
    if (owner->capacity >= 65535UL)
        return 0;
    capacity = owner->capacity ? owner->capacity * 2 : 8;
    if (capacity > 65535UL)
        capacity = 65535UL;
    next = (BHRenderResourceEntry **)owner->allocator.allocate(capacity * sizeof(*next), owner->allocator.user);
    if (!next)
        return 0;
    if (owner->count)
        memcpy(next, owner->entries, owner->count * sizeof(*next));
    if (owner->entries)
        owner->allocator.release(owner->entries, owner->allocator.user);
    owner->entries = next;
    owner->capacity = capacity;
    return 1;
}
int __cdecl bh_render_resources_init(BHRenderResources *owner, BHModelContext *models,
                                     const BHModelInstanceContext *prototype, const BHSceneAllocator *allocator)
{
    if (!owner || owner->entries || owner->count || owner->capacity || !models || !models->allocator.allocate ||
        !models->allocator.release || !prototype || !prototype->camera || !prototype->config || !prototype->clip ||
        !prototype->emit ||
        (prototype->point_enabled && (!prototype->light_config || !prototype->lights || !prototype->light_reference)) ||
        (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(owner, 0, sizeof(*owner));
    owner->models = models;
    owner->prototype = *prototype;
    if (allocator)
        owner->allocator = *allocator;
    else
    {
        owner->allocator.allocate = allocate;
        owner->allocator.release = release;
    }
    return 1;
}
unsigned long __cdecl bh_render_resources_load_model(BHRenderResources *owner, const char *name)
{
    BHRenderResourceEntry *entry;
    if (!owner || !owner->models || !owner->allocator.allocate || !name || !name[0])
    {
        if (owner)
            owner->error = BH_RENDER_RESOURCES_ARGUMENT;
        return 0;
    }
    if (owner->count >= 65535UL)
    {
        owner->error = BH_RENDER_RESOURCES_FULL;
        return 0;
    }
    entry = (BHRenderResourceEntry *)owner->allocator.allocate(sizeof(*entry), owner->allocator.user);
    if (!entry)
    {
        owner->error = BH_RENDER_RESOURCES_MEMORY;
        return 0;
    }
    memset(entry, 0, sizeof(*entry));
    entry->model = bh_model_load(owner->models, name);
    if (!entry->model)
    {
        owner->error = BH_RENDER_RESOURCES_MODEL;
        goto failure;
    }
    if (!bh_model_draw_cache_init(&entry->cache, entry->model, &owner->allocator))
    {
        owner->error = BH_RENDER_RESOURCES_MEMORY;
        goto failure;
    }
    if (!reserve(owner))
    {
        owner->error = owner->capacity >= 65535UL ? BH_RENDER_RESOURCES_FULL : BH_RENDER_RESOURCES_MEMORY;
        goto failure;
    }
    entry->instance = owner->prototype;
    entry->instance.model = entry->model;
    entry->instance.cache = &entry->cache;
    entry->instance.frame = 0;
    entry->instance.drawn = 0;
    entry->instance.faces_marked = 0;
    entry->instance.polygons_emitted = 0;
    entry->instance.error = BH_MODEL_INSTANCE_OK;
    memset(&entry->instance.last_camera_view, 0, sizeof(entry->instance.last_camera_view));
    entry->descriptor.tag = BH_RESOURCE_TAG_MODEL;
    entry->descriptor.data.model.instance = &entry->instance;
    owner->entries[owner->count++] = entry;
    owner->error = BH_RENDER_RESOURCES_OK;
    return owner->count;
failure:
    bh_model_draw_cache_dispose(&entry->cache);
    if (entry->model)
        bh_model_delete(owner->models, entry->model);
    owner->allocator.release(entry, owner->allocator.user);
    return 0;
}
const BHDrawResource *__cdecl bh_render_resources_lookup(unsigned long handle, void *user)
{
    BHRenderResources *owner = (BHRenderResources *)user;
    if (!owner || !handle || handle > owner->count || !owner->entries)
        return NULL;
    return &owner->entries[handle - 1]->descriptor;
}
void __cdecl bh_render_resources_dispose(BHRenderResources *owner)
{
    unsigned long i;
    if (!owner)
        return;
    if (owner->entries)
        for (i = 0; i < owner->count; ++i)
        {
            BHRenderResourceEntry *entry = owner->entries[i];
            bh_model_draw_cache_dispose(&entry->cache);
            bh_model_delete(owner->models, entry->model);
            owner->allocator.release(entry, owner->allocator.user);
        }
    if (owner->entries)
        owner->allocator.release(owner->entries, owner->allocator.user);
    memset(owner, 0, sizeof(*owner));
}
