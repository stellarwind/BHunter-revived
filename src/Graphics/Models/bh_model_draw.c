#include "bh_model_draw.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
static_assert(offsetof(BHModel, vertices) == 0x44, "BHModel.vertices offset");
static_assert(offsetof(BHModel, faces) == 0x54, "BHModel.faces offset");
static_assert(offsetof(BHModel, nodes) == 0x5c, "BHModel.nodes offset");
static_assert(offsetof(BHModel, leaves) == 0x64, "BHModel.leaves offset");
static_assert(offsetof(BHModel, images) == 0x68, "BHModel.images offset");
static_assert(sizeof(BHBspVertex) == sizeof(BHClipVertex) && sizeof(BHClipVertex) == 60, "draw vertex size");
static_assert(offsetof(BHClipVertex, frame) == 0x34 && offsetof(BHClipVertex, projected) == 0x38, "draw vertex frame");
static_assert(offsetof(BHTextureImage, flags) == 0x50, "BHTextureImage.flags offset");
static void *allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void release(void *p, void *user)
{
    free(p);
}
static int fail(BHModelDrawContext *context, BHModelDrawError error)
{
    if (context)
        context->error = error;
    return 0;
}
static unsigned long vertex_frame(const BHClipVertex *vertex)
{
    unsigned long frame;
    memcpy(&frame, &vertex->frame, 4);
    return frame;
}
static void set_frame(BHClipVertex *vertex, unsigned long frame)
{
    memcpy(&vertex->frame, &frame, 4);
}
void bh_model_draw_cache_dispose(BHModelDrawCache *cache)
{
    if (!cache)
        return;
    if (cache->allocator.release)
    {
        if (cache->vertices)
            cache->allocator.release(cache->vertices, cache->allocator.user);
        if (cache->face_frames)
            cache->allocator.release(cache->face_frames, cache->allocator.user);
        if (cache->tasks)
            cache->allocator.release(cache->tasks, cache->allocator.user);
    }
    memset(cache, 0, sizeof(*cache));
}
void bh_model_draw_cache_reset(BHModelDrawCache *cache)
{
    unsigned long i;
    if (!cache)
        return;
    for (i = 0; i < cache->vertex_count; ++i)
        set_frame(cache->vertices + i, 0);
    if (cache->face_frames)
        memset(cache->face_frames, 0, cache->face_count * 4);
}
int bh_model_draw_cache_init(BHModelDrawCache *cache, const BHModel *model, const BHSceneAllocator *scene_allocator)
{
    BHModelDrawCache next;
    unsigned long i;
    if (!cache || !model || (scene_allocator && (!scene_allocator->allocate || !scene_allocator->release)) || model->vertex_count > 65535 ||
        model->face_count > 65535 || model->node_count > 65535 || (model->vertex_count && !model->vertices) ||
        (model->face_count && !model->faces) || (model->edge_count && !model->edges) ||
        (model->node_count && !model->nodes) || (model->leaf_count && !model->leaves) ||
        (model->material_count && !model->images))
        return 0;
    memset(&next, 0, sizeof(next));
    if (scene_allocator)
        next.allocator = *scene_allocator;
    else
    {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    next.model = model;
    next.vertex_count = model->vertex_count;
    next.face_count = model->face_count;
    next.task_capacity = model->node_count + 1;
    if (next.vertex_count)
        next.vertices =
            (BHClipVertex *)next.allocator.allocate(next.vertex_count * sizeof(*next.vertices), next.allocator.user);
    if (next.face_count)
        next.face_frames = (unsigned long *)next.allocator.allocate(next.face_count * 4, next.allocator.user);
    next.tasks =
        (BHModelDrawTask *)next.allocator.allocate(next.task_capacity * sizeof(*next.tasks), next.allocator.user);
    if ((next.vertex_count && !next.vertices) || (next.face_count && !next.face_frames) || !next.tasks)
    {
        bh_model_draw_cache_dispose(&next);
        return 0;
    }
    for (i = 0; i < next.vertex_count; ++i)
    {
        memcpy(next.vertices + i, model->vertices + i, 60);
        next.vertices[i].next = next.vertices[i].previous = NULL;
    }
    bh_model_draw_cache_reset(&next);
    bh_model_draw_cache_dispose(cache);
    *cache = next;
    return 1;
}
int bh_model_draw_context_init(BHModelDrawContext *context, const BHModel *model, BHModelDrawCache *cache,
                               const BHViewCamera *view, BHClipContext *clip, unsigned long frame,
                               unsigned long texture_mask, unsigned long color, BHModelPolygonEmit emit, void *user)
{
    if (!context || !model || !cache || cache->model != model || !view || !clip || !frame || !emit ||
        !bh_clip_set_camera(clip, &view->projected))
        return 0;
    memset(context, 0, sizeof(*context));
    context->model = model;
    context->cache = cache;
    context->view = view;
    context->clip = clip;
    context->frame = frame;
    context->texture_mask = texture_mask;
    context->color = color;
    context->emit = emit;
    context->user = user;
    return 1;
}
int bh_model_draw_face(BHModelDrawContext *context, long face)
{
    const BHModel *model;
    const BHBspFace *bsp_face;
    const BHBspEdge *edge;
    BHTextureImage *image;
    BHClipVertex *first = NULL, *previous = NULL, *vertex, *ring;
    long e;
    unsigned long steps = 0;
    if (!context || !context->model || !context->cache || face < 0 || (unsigned long)face >= context->model->face_count)
        return fail(context, BH_MODEL_DRAW_ARGUMENT);
    model = context->model;
    bsp_face = model->faces + face;
    if (context->cache->face_frames[face] == context->frame)
        return 1;
    context->cache->face_frames[face] = context->frame;
    ++context->faces_marked;
    if (bsp_face->material < 0 || (unsigned long)bsp_face->material >= model->material_count)
        return fail(context, BH_MODEL_DRAW_TOPOLOGY);
    image = model->images[bsp_face->material];
    if (!image || !(context->texture_mask & image->flags))
        return 1;
    e = bsp_face->first_edge;
    do
    {
        if (e < 0 || (unsigned long)e >= model->edge_count || ++steps > model->edge_count)
            return fail(context, BH_MODEL_DRAW_TOPOLOGY);
        edge = model->edges + e;
        if (edge->vertex < 0 || (unsigned long)edge->vertex >= context->cache->vertex_count)
            return fail(context, BH_MODEL_DRAW_TOPOLOGY);
        vertex = context->cache->vertices + edge->vertex;
        if (!first)
            first = vertex;
        vertex->u = edge->u;
        vertex->v = edge->v;
        if (vertex_frame(vertex) != context->frame)
        {
            set_frame(vertex, context->frame);
            vertex->projected = 0;
            bh_mat34_transform_point(&context->view->projected, &vertex->local, &vertex->point);
            ++context->vertices_transformed;
        }
        if (previous)
        {
            previous->next = vertex;
            vertex->previous = previous;
        }
        previous = vertex;
        e = edge->next;
    } while (e >= 0);
    first->previous = previous;
    previous->next = first;
    ring = bh_clip_polygon(context->clip, previous, image);
    if (!ring)
    {
        if (context->clip->error != BH_CLIP_OK)
            return fail(context, BH_MODEL_DRAW_CLIP);
        return 1;
    }
    if (!context->emit(ring, image, context->color, context->user))
        return fail(context, BH_MODEL_DRAW_EMIT);
    ++context->polygons_emitted;
    return 1;
}
int bh_model_draw_leaf_chain(BHModelDrawContext *context, long leaf)
{
    unsigned long steps = 0;
    const BHBspLeaf *bsp_leaf;
    if (!context || !context->model)
        return fail(context, BH_MODEL_DRAW_ARGUMENT);
    while (leaf >= 0)
    {
        if ((unsigned long)leaf >= context->model->leaf_count || ++steps > context->model->leaf_count)
            return fail(context, BH_MODEL_DRAW_TOPOLOGY);
        bsp_leaf = context->model->leaves + leaf;
        if (!bh_model_draw_face(context, bsp_leaf->face))
            return 0;
        leaf = bsp_leaf->next;
    }
    return 1;
}
/* Full sign-tree reduction retains x87 addition ordering. The exceptional
   branch finishes with Z+(X+Y), all others with X+(Y+Z). */
static double support(const BHBspNode *node, const BHViewPlane *view_plane, int maximum)
{
    double x, y, z;
    float nx = view_plane->normal.x, ny = view_plane->normal.y, nz = view_plane->normal.z;
    x = (double)nx * ((maximum ? (nx >= 0) : (nx < 0)) ? node->maximum[0] : node->minimum[0]);
    y = (double)ny * ((maximum ? (ny >= 0) : (ny < 0)) ? node->maximum[1] : node->minimum[1]);
    z = (double)nz * ((maximum ? (nz >= 0) : (nz < 0)) ? node->maximum[2] : node->minimum[2]);
    if (maximum ? (nx >= 0 && nz < 0) : (nx < 0 && nz >= 0))
        return (z + (x + y)) - view_plane->offset;
    return (x + (y + z)) - view_plane->offset;
}
int bh_model_draw_unlit(BHModelDrawContext *context, long node, unsigned long mask)
{
    unsigned long count = 1, slot, bit;
    BHModelDrawTask *task;
    const BHBspNode *bsp_node;
    double side;
    if (!context || !context->model || !context->cache || !context->cache->tasks || !context->view)
        return fail(context, BH_MODEL_DRAW_ARGUMENT);
    context->error = BH_MODEL_DRAW_OK;
    context->cache->tasks[0].node = node;
    context->cache->tasks[0].mask = mask;
    context->cache->tasks[0].phase = 0;
    while (count)
    {
        task = context->cache->tasks + count - 1;
        if (task->phase)
        {
            if (!bh_model_draw_leaf_chain(context, task->leaf))
                return 0;
            task->node = task->tail;
            task->phase = 0;
            continue;
        }
        if (task->node < 0)
        {
            --count;
            continue;
        }
        if ((unsigned long)task->node >= context->model->node_count)
            return fail(context, BH_MODEL_DRAW_TOPOLOGY);
        bsp_node = context->model->nodes + task->node;
        ++context->nodes_visited;
        for (slot = 0, bit = 1; slot < 6; ++slot, bit *= 2)
            if (task->mask & bit)
                if (support(bsp_node, context->view->planes + slot, 0) >= 0)
                    break;
        if (slot < 6)
        {
            --count;
            continue;
        }
        for (slot = 0, bit = 1; slot < 6; ++slot, bit *= 2)
            if (task->mask & bit)
                if (support(bsp_node, context->view->planes + slot, 1) < 0)
                    task->mask &= (0xffffUL ^ bit);
        side = ((double)bsp_node->normal[2] * context->view->position.z + (double)bsp_node->normal[1] * context->view->position.y) +
               (double)bsp_node->normal[0] * context->view->position.x + bsp_node->plane;
        task->phase = 1;
        if (side < 0)
        {
            task->leaf = bsp_node->back_leaf;
            task->tail = bsp_node->back_node;
            node = bsp_node->front_node;
        }
        else
        {
            task->leaf = bsp_node->front_leaf;
            task->tail = bsp_node->front_node;
            node = bsp_node->back_node;
        }
        if (count >= context->cache->task_capacity)
            return fail(context, BH_MODEL_DRAW_TOPOLOGY);
        context->cache->tasks[count].node = node;
        context->cache->tasks[count].mask = task->mask;
        context->cache->tasks[count].phase = 0;
        ++count;
    }
    return 1;
}
