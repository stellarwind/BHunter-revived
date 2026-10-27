#include "bh_model_draw.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef char draw_model_vertices[offsetof(BHModel, vertices) == 0x44 ? 1 : -1];
typedef char draw_model_faces[offsetof(BHModel, faces) == 0x54 ? 1 : -1];
typedef char draw_model_nodes[offsetof(BHModel, nodes) == 0x5c ? 1 : -1];
typedef char draw_model_leaves[offsetof(BHModel, leaves) == 0x64 ? 1 : -1];
typedef char draw_model_images[offsetof(BHModel, images) == 0x68 ? 1 : -1];
typedef char draw_vertex_size[sizeof(BHBspVertex) == sizeof(BHClipVertex) && sizeof(BHClipVertex) == 60 ? 1 : -1];
typedef char
    draw_vertex_frame[offsetof(BHClipVertex, frame) == 0x34 && offsetof(BHClipVertex, projected) == 0x38 ? 1 : -1];
typedef char draw_image_flags[offsetof(BHTextureImage, flags) == 0x50 ? 1 : -1];
static void *__cdecl allocate(unsigned long bytes, void *user)
{
    return malloc(bytes);
}
static void __cdecl release(void *p, void *user)
{
    free(p);
}
static int fail(BHModelDrawContext *c, BHModelDrawError error)
{
    if (c)
        c->error = error;
    return 0;
}
static unsigned long vertex_frame(const BHClipVertex *v)
{
    unsigned long frame;
    memcpy(&frame, &v->frame, 4);
    return frame;
}
static void set_frame(BHClipVertex *v, unsigned long frame)
{
    memcpy(&v->frame, &frame, 4);
}
void __cdecl bh_model_draw_cache_dispose(BHModelDrawCache *cache)
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
void __cdecl bh_model_draw_cache_reset(BHModelDrawCache *cache)
{
    unsigned long i;
    if (!cache)
        return;
    for (i = 0; i < cache->vertex_count; ++i)
        set_frame(cache->vertices + i, 0);
    if (cache->face_frames)
        memset(cache->face_frames, 0, cache->face_count * 4);
}
int __cdecl bh_model_draw_cache_init(BHModelDrawCache *cache, const BHModel *model, const BHSceneAllocator *a)
{
    BHModelDrawCache next;
    unsigned long i;
    if (!cache || !model || (a && (!a->allocate || !a->release)) || model->vertex_count > 65535 ||
        model->face_count > 65535 || model->node_count > 65535 || (model->vertex_count && !model->vertices) ||
        (model->face_count && !model->faces) || (model->edge_count && !model->edges) ||
        (model->node_count && !model->nodes) || (model->leaf_count && !model->leaves) ||
        (model->material_count && !model->images))
        return 0;
    memset(&next, 0, sizeof(next));
    if (a)
        next.allocator = *a;
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
int __cdecl bh_model_draw_context_init(BHModelDrawContext *c, const BHModel *model, BHModelDrawCache *cache,
                                       const BHViewCamera *view, BHClipContext *clip, unsigned long frame,
                                       unsigned long texture_mask, unsigned long color, BHModelPolygonEmit emit,
                                       void *user)
{
    if (!c || !model || !cache || cache->model != model || !view || !clip || !frame || !emit ||
        !bh_clip_set_camera(clip, &view->projected))
        return 0;
    memset(c, 0, sizeof(*c));
    c->model = model;
    c->cache = cache;
    c->view = view;
    c->clip = clip;
    c->frame = frame;
    c->texture_mask = texture_mask;
    c->color = color;
    c->emit = emit;
    c->user = user;
    return 1;
}
int __cdecl bh_model_draw_face(BHModelDrawContext *c, long face)
{
    const BHModel *m;
    const BHBspFace *f;
    const BHBspEdge *edge;
    BHTextureImage *image;
    BHClipVertex *first = NULL, *previous = NULL, *v, *ring;
    long e;
    unsigned long steps = 0;
    if (!c || !c->model || !c->cache || face < 0 || (unsigned long)face >= c->model->face_count)
        return fail(c, BH_MODEL_DRAW_ARGUMENT);
    m = c->model;
    f = m->faces + face;
    if (c->cache->face_frames[face] == c->frame)
        return 1;
    c->cache->face_frames[face] = c->frame;
    ++c->faces_marked;
    if (f->material < 0 || (unsigned long)f->material >= m->material_count)
        return fail(c, BH_MODEL_DRAW_TOPOLOGY);
    image = m->images[f->material];
    if (!image || !(c->texture_mask & image->flags))
        return 1;
    e = f->first_edge;
    do
    {
        if (e < 0 || (unsigned long)e >= m->edge_count || ++steps > m->edge_count)
            return fail(c, BH_MODEL_DRAW_TOPOLOGY);
        edge = m->edges + e;
        if (edge->vertex < 0 || (unsigned long)edge->vertex >= c->cache->vertex_count)
            return fail(c, BH_MODEL_DRAW_TOPOLOGY);
        v = c->cache->vertices + edge->vertex;
        if (!first)
            first = v;
        v->u = edge->u;
        v->v = edge->v;
        if (vertex_frame(v) != c->frame)
        {
            set_frame(v, c->frame);
            v->projected = 0;
            bh_mat34_transform_point(&c->view->projected, &v->local, &v->point);
            ++c->vertices_transformed;
        }
        if (previous)
        {
            previous->next = v;
            v->previous = previous;
        }
        previous = v;
        e = edge->next;
    } while (e >= 0);
    first->previous = previous;
    previous->next = first;
    ring = bh_clip_polygon(c->clip, previous, image);
    if (!ring)
    {
        if (c->clip->error != BH_CLIP_OK)
            return fail(c, BH_MODEL_DRAW_CLIP);
        return 1;
    }
    if (!c->emit(ring, image, c->color, c->user))
        return fail(c, BH_MODEL_DRAW_EMIT);
    ++c->polygons_emitted;
    return 1;
}
int __cdecl bh_model_draw_leaf_chain(BHModelDrawContext *c, long leaf)
{
    unsigned long steps = 0;
    const BHBspLeaf *p;
    if (!c || !c->model)
        return fail(c, BH_MODEL_DRAW_ARGUMENT);
    while (leaf >= 0)
    {
        if ((unsigned long)leaf >= c->model->leaf_count || ++steps > c->model->leaf_count)
            return fail(c, BH_MODEL_DRAW_TOPOLOGY);
        p = c->model->leaves + leaf;
        if (!bh_model_draw_face(c, p->face))
            return 0;
        leaf = p->next;
    }
    return 1;
}
/* Full sign-tree reduction retains x87 addition ordering. The exceptional
   branch finishes with Z+(X+Y), all others with X+(Y+Z). */
static double support(const BHBspNode *node, const BHViewPlane *p, int maximum)
{
    double x, y, z;
    float nx = p->normal.x, ny = p->normal.y, nz = p->normal.z;
    x = (double)nx * ((maximum ? (nx >= 0) : (nx < 0)) ? node->maximum[0] : node->minimum[0]);
    y = (double)ny * ((maximum ? (ny >= 0) : (ny < 0)) ? node->maximum[1] : node->minimum[1]);
    z = (double)nz * ((maximum ? (nz >= 0) : (nz < 0)) ? node->maximum[2] : node->minimum[2]);
    if (maximum ? (nx >= 0 && nz < 0) : (nx < 0 && nz >= 0))
        return (z + (x + y)) - p->offset;
    return (x + (y + z)) - p->offset;
}
int __cdecl bh_model_draw_unlit(BHModelDrawContext *c, long node, unsigned long mask)
{
    unsigned long count = 1, slot, bit;
    BHModelDrawTask *task;
    const BHBspNode *n;
    double side;
    if (!c || !c->model || !c->cache || !c->cache->tasks || !c->view)
        return fail(c, BH_MODEL_DRAW_ARGUMENT);
    c->error = BH_MODEL_DRAW_OK;
    c->cache->tasks[0].node = node;
    c->cache->tasks[0].mask = mask;
    c->cache->tasks[0].phase = 0;
    while (count)
    {
        task = c->cache->tasks + count - 1;
        if (task->phase)
        {
            if (!bh_model_draw_leaf_chain(c, task->leaf))
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
        if ((unsigned long)task->node >= c->model->node_count)
            return fail(c, BH_MODEL_DRAW_TOPOLOGY);
        n = c->model->nodes + task->node;
        ++c->nodes_visited;
        for (slot = 0, bit = 1; slot < 6; ++slot, bit *= 2)
            if (task->mask & bit)
                if (support(n, c->view->planes + slot, 0) >= 0)
                    break;
        if (slot < 6)
        {
            --count;
            continue;
        }
        for (slot = 0, bit = 1; slot < 6; ++slot, bit *= 2)
            if (task->mask & bit)
                if (support(n, c->view->planes + slot, 1) < 0)
                    task->mask &= (0xffffUL ^ bit);
        side = ((double)n->normal[2] * c->view->position.z + (double)n->normal[1] * c->view->position.y) +
               (double)n->normal[0] * c->view->position.x + n->plane;
        task->phase = 1;
        if (side < 0)
        {
            task->leaf = n->back_leaf;
            task->tail = n->back_node;
            node = n->front_node;
        }
        else
        {
            task->leaf = n->front_leaf;
            task->tail = n->front_node;
            node = n->back_node;
        }
        if (count >= c->cache->task_capacity)
            return fail(c, BH_MODEL_DRAW_TOPOLOGY);
        c->cache->tasks[count].node = node;
        c->cache->tasks[count].mask = task->mask;
        c->cache->tasks[count].phase = 0;
        ++count;
    }
    return 1;
}
