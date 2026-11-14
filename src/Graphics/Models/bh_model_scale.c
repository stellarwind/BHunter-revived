#include "bh_model_scale.h"
#include "../../Math/bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>

static int negative_or_unordered(double n)
{
    return n < 0.0 || _isnan(n);
}
static int edge_index(const BHModel *model, long e)
{
    return e >= 0 && (unsigned long)e < model->edge_count;
}
long bh_model_reverse_edges(BHModel *model, long first, long count)
{
    long left, right, after, remaining, i;
    BHBspEdge saved;
    if (count == 0)
        return first;
    if (!model || count < 0 || (unsigned long)count > model->edge_count || model->edge_count > 65535 || !model->edges)
        return LONG_MIN;
    after = first;
    for (i = 0; i < count; ++i)
    {
        if (!edge_index(model, after))
            return LONG_MIN;
        after = model->edges[after].next;
    }
    /* Stackless equivalent with the original innermost-first swap order; the source
       loader permits65535 slots, exceeding a safe recursive native stack. */
    for (remaining = count / 2; remaining > 0; --remaining)
    {
        left = first;
        for (i = 1; i < remaining; ++i)
            left = model->edges[left].next;
        right = left;
        for (i = 0; i < count - remaining * 2 + 1; ++i)
            right = model->edges[right].next;
        memcpy(&saved, model->edges + left, sizeof(saved));
        model->edges[left].vertex = model->edges[right].vertex;
        memcpy(&model->edges[left].u, &model->edges[right].u, 8);
        model->edges[right].vertex = saved.vertex;
        memcpy(&model->edges[right].u, &saved.u, 8);
    }
    return after;
}
static int bindings(BHModel *model, BHModelDrawCache *const *caches, unsigned long count, int reflect)
{
    unsigned long i, j, n;
    long e;
    BHModelDrawCache *model_draw_cache;
    if (model->vertex_count > 65535 || model->edge_count > 65535 || model->face_count > 65535 || !model->node_count ||
        model->node_count > 65535 || !model->nodes || (model->vertex_count && !model->vertices) ||
        (reflect && model->face_count && !model->faces) || (reflect && model->edge_count && !model->edges) || count > 65535 ||
        (count && !caches))
        return 0;
    for (i = 0; i < count; ++i)
    {
        model_draw_cache = caches[i];
        if (!model_draw_cache)
            return 0;
        for (j = 0; j < i; ++j)
            if (caches[j] == model_draw_cache)
                return 0;
        if (model_draw_cache->model && model_draw_cache->model != model)
            return 0;
        if ((model_draw_cache->vertices || model_draw_cache->face_frames || model_draw_cache->tasks) && (model_draw_cache->model != model || !model_draw_cache->allocator.release))
            return 0;
    }
    if (reflect)
        for (i = 0; i < model->face_count; ++i)
        {
            e = model->faces[i].first_edge;
            n = 0;
            while (e >= 0)
            {
                if (!edge_index(model, e) || ++n > model->edge_count)
                    return 0;
                e = model->edges[e].next;
            }
        }
    return 1;
}
static double magnitude(double value)
{
    return negative_or_unordered(value) ? -value : value;
}
static double extent(float lo, float hi)
{
    double a = magnitude(lo), b = magnitude(hi);
    return a < b ? b : a;
}
int bh_model_scale(BHModel *model, float sx, float sy, float sz, BHModelDrawCache *const *caches, unsigned long count)
{
    unsigned long i, j;
    int reflect;
    BHBspNode *bsp_node;
    float px, py, pz;
    double x, y, z, oldx, oldy, oldz, plane, det;
    long e, length;
    if (!model)
        return 0;
    if (model->frame)
        return 1;
    det = ((double)sz * sy) * sx;
    reflect = negative_or_unordered(det);
    if (!bindings(model, caches, count, reflect))
        return 0;
    for (i = 0; i < model->vertex_count; ++i)
    {
        bh_round_float(&model->vertices[i].x, (double)sx * model->vertices[i].x);
        bh_round_float(&model->vertices[i].y, (double)sy * model->vertices[i].y);
        bh_round_float(&model->vertices[i].z, (double)sz * model->vertices[i].z);
    }
    for (i = 0; i < model->node_count; ++i)
    {
        bsp_node = model->nodes + i;
        oldx = bsp_node->normal[0];
        oldy = bsp_node->normal[1];
        oldz = bsp_node->normal[2];
        plane = -(double)bsp_node->plane;
        bh_round_float(&bsp_node->normal[0], oldx / sx);
        bh_round_float(&bsp_node->normal[1], oldy / sy);
        bh_round_float(&bsp_node->normal[2], oldz / sz);
        bh_round_float(&px, oldx * plane);
        bh_round_float(&py, oldy * plane);
        bh_round_float(&pz, oldz * plane);
        bh_vec3_normalize((BHVec3 *)bsp_node->normal);
        x = ((double)px * sx) * bsp_node->normal[0];
        z = ((double)pz * sz) * bsp_node->normal[2];
        y = ((double)py * sy) * bsp_node->normal[1];
        bh_round_float(&bsp_node->plane, -((z + x) + y));
        bh_round_float(bsp_node->minimum, (double)sx * bsp_node->minimum[0]);
        bh_round_float(bsp_node->maximum, (double)sx * bsp_node->maximum[0]);
        bh_round_float(bsp_node->minimum + 1, (double)sy * bsp_node->minimum[1]);
        bh_round_float(bsp_node->maximum + 1, (double)sy * bsp_node->maximum[1]);
        bh_round_float(bsp_node->minimum + 2, (double)sz * bsp_node->minimum[2]);
        bh_round_float(bsp_node->maximum + 2, (double)sz * bsp_node->maximum[2]);
        for (j = 0; j < 3; ++j)
            if (negative_or_unordered(j == 0 ? sx : j == 1 ? sy : sz))
            {
                float swap = bsp_node->minimum[j];
                bsp_node->minimum[j] = bsp_node->maximum[j];
                bsp_node->maximum[j] = swap;
            }
    }
    if (reflect)
        for (i = 0; i < model->face_count; ++i)
        {
            length = 0;
            e = model->faces[i].first_edge;
            while (e >= 0)
            {
                ++length;
                e = model->edges[e].next;
            }
            bh_model_reverse_edges(model, model->faces[i].first_edge, length);
        }
    for (i = 0; i < count; ++i)
        bh_model_draw_cache_dispose(caches[i]);
    memset(model->runtime, 0, sizeof(model->runtime));
    x = extent(model->nodes[0].minimum[0], model->nodes[0].maximum[0]);
    y = extent(model->nodes[0].minimum[1], model->nodes[0].maximum[1]);
    z = extent(model->nodes[0].minimum[2], model->nodes[0].maximum[2]);
    bh_round_float(&model->radius, sqrt((x * x + y * y) + z * z));
    return 1;
}
