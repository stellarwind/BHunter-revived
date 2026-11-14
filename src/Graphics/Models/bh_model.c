#include "bh_model.h"
#include "../Textures/bh_texture.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

static_assert(sizeof(BHModel) == 0x80, "BHModel size");
static_assert(offsetof(BHModel, transform) == 8, "BHModel.transform offset");
static_assert(offsetof(BHModel, materials) == 0x3c, "BHModel.materials offset");
static_assert(offsetof(BHModel, images) == 0x68, "BHModel.images offset");
static_assert(offsetof(BHModel, runtime) == 0x6c, "BHModel.runtime offset");
static void *allocate(unsigned long n, void *user)
{
    return malloc(n);
}
static void release(void *p, void *user)
{
    free(p);
}
static int fail(BHModelContext *context, BHBspError error)
{
    if (context)
        context->error = error;
    return 0;
}
int bh_model_context_init(BHModelContext *context, BHTextureContext *textures, const BHSceneAllocator *scene_allocator)
{
    if (!context || !textures || (scene_allocator && (!scene_allocator->allocate || !scene_allocator->release)))
        return 0;
    memset(context, 0, sizeof(*context));
    context->textures = textures;
    context->search = BH_BSP_BARE_ONLY;
    if (scene_allocator)
        context->allocator = *scene_allocator;
    else
    {
        context->allocator.allocate = allocate;
        context->allocator.release = release;
    }
    return 1;
}
int bh_model_set_path(BHModelContext *context, const char *prefix, BHBspSearch search)
{
    unsigned long n = 0;
    if (!context || !prefix || search < BH_BSP_BARE_ONLY || search > BH_BSP_PREFIX_ONLY)
        return fail(context, BH_BSP_ARGUMENT);
    while (n < 256 && prefix[n])
        ++n;
    if (n == 256)
        return fail(context, BH_BSP_LIMIT);
    memcpy(context->prefix, prefix, n + 1);
    context->search = search;
    context->error = BH_BSP_OK;
    return 1;
}
void bh_model_dispose(BHModelContext *context, BHModel *model)
{
    if (!context || !model)
        return;
    if (model->materials)
        context->allocator.release(model->materials, context->allocator.user);
    if (model->vertices)
        context->allocator.release(model->vertices, context->allocator.user);
    if (model->edges)
        context->allocator.release(model->edges, context->allocator.user);
    if (model->faces)
        context->allocator.release(model->faces, context->allocator.user);
    if (model->nodes)
        context->allocator.release(model->nodes, context->allocator.user);
    if (model->leaves)
        context->allocator.release(model->leaves, context->allocator.user);
    if (model->images)
        context->allocator.release(model->images, context->allocator.user);
    memset(model, 0, sizeof(*model));
}
void bh_model_delete(BHModelContext *context, BHModel *m)
{
    if (!context || !m)
        return;
    bh_model_dispose(context, m);
    context->allocator.release(m, context->allocator.user);
}
static void *copy(BHModelContext *context, const void *p, unsigned long bytes)
{
    void *out = context->allocator.allocate(bytes, context->allocator.user);
    if (out)
        memcpy(out, p, bytes);
    return out;
}
int bh_model_read(BHModelContext *context, BHVFile *file, BHModel *model)
{
    BHModel next;
    BHBsp data;
    unsigned long i, j;
    BHBspError error;
    if (!context || !context->textures || !context->allocator.allocate || !file || !model)
        return fail(context, BH_BSP_ARGUMENT);
    memset(&next, 0, sizeof(next));
    bh_bsp_init(&data, &context->allocator);
    /* Explicit validation dependency rejects corrupt record graphs before any
       texture cache side effects. Original record arrays are copied verbatim. */
    if (!bh_bsp_read(&data, file))
    {
        error = data.error;
        bh_bsp_dispose(&data);
        return fail(context, error);
    }
    next.transform.m[0][0] = next.transform.m[1][1] = next.transform.m[2][2] = 1;
    next.radius = data.root_radius;
    next.material_count = data.counts[0];
    next.vertex_count = data.counts[1];
    next.edge_count = data.counts[2];
    next.face_count = data.counts[3];
    next.node_count = data.counts[4];
    next.leaf_count = data.counts[5];
    next.materials = (BHBspMaterial *)copy(context, data.materials, next.material_count * 32);
    next.vertices = (BHBspVertex *)copy(context, data.vertices, next.vertex_count * 60);
    next.edges = (BHBspEdge *)copy(context, data.edges, next.edge_count * 16);
    next.faces = (BHBspFace *)copy(context, data.faces, next.face_count * 12);
    next.nodes = (BHBspNode *)copy(context, data.nodes, next.node_count * 56);
    next.leaves = (BHBspLeaf *)copy(context, data.leaves, next.leaf_count * 8);
    next.images =
        (BHTextureImage **)context->allocator.allocate(next.material_count * sizeof(*next.images), context->allocator.user);
    bh_bsp_dispose(&data);
    if (!next.materials || !next.vertices || !next.edges || !next.faces || !next.nodes || !next.leaves || !next.images)
    {
        bh_model_dispose(context, &next);
        return fail(context, BH_BSP_MEMORY);
    }
    for (i = 0; i < next.material_count; ++i)
    {
        next.images[i] = NULL;
        for (j = 0; j < next.face_count; ++j)
            if (next.faces[j].material == (long)i)
                break;
        if (j < next.face_count && next.materials[i].texture[0])
            next.images[i] = bh_texture_load(context->textures, next.materials[i].texture);
    }
    bh_model_dispose(context, model);
    *model = next;
    context->error = BH_BSP_OK;
    return 1;
}
BHModel *bh_model_load(BHModelContext *context, const char *name)
{
    char path[256];
    unsigned long n = 0, p;
    BHVFile *file = NULL;
    BHModel *model;
    if (!context || !name || !context->textures || !context->allocator.allocate)
    {
        fail(context, BH_BSP_ARGUMENT);
        return NULL;
    }
    while (n < 256 && name[n])
        ++n;
    p = (unsigned long)strlen(context->prefix);
    if (!n || n == 256 || p + n >= 256)
    {
        fail(context, BH_BSP_LIMIT);
        return NULL;
    }
    memcpy(path, context->prefix, p);
    memcpy(path + p, name, n + 1);
    switch (context->search)
    {
    case BH_BSP_PREFIX_FIRST:
        file = bh_vfile_open(path, "rb");
        if (!file)
            file = bh_vfile_open(name, "rb");
        break;
    case BH_BSP_BARE_FIRST:
        file = bh_vfile_open(name, "rb");
        if (!file)
            file = bh_vfile_open(path, "rb");
        break;
    case BH_BSP_PREFIX_ONLY:
        file = bh_vfile_open(path, "rb");
        break;
    case BH_BSP_BARE_ONLY:
        file = bh_vfile_open(name, "rb");
        break;
    }
    if (!file)
    {
        fail(context, BH_BSP_IO);
        return NULL;
    }
    model = (BHModel *)context->allocator.allocate(sizeof(*model), context->allocator.user);
    if (!model)
    {
        bh_vfile_close(file);
        fail(context, BH_BSP_MEMORY);
        return NULL;
    }
    memset(model, 0, sizeof(*model));
    if (!bh_model_read(context, file, model))
    {
        bh_vfile_close(file);
        bh_model_delete(context, model);
        return NULL;
    }
    if (bh_vfile_close(file))
    {
        bh_model_delete(context, model);
        fail(context, BH_BSP_IO);
        return NULL;
    }
    return model;
}
