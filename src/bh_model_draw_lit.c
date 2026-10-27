#include "bh_model_draw_lit.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>
static int fail(BHModelDrawLitContext *c, BHModelDrawLitError error)
{
    if (c)
        c->error = error;
    return 0;
}
int __cdecl bh_model_draw_lit_context_init(BHModelDrawLitContext *c, BHModelDrawContext *draw,
                                           const BHLightContext *prepared, const BHLightConfig *directional,
                                           const BHMat34 *world_to_model, long point_enabled, float scale,
                                           unsigned long base_color)
{
    BHModelDrawLitContext next;
    unsigned long i;
    if (!c || !draw || !draw->model || !draw->cache || !draw->view || !directional || !_finite(scale) ||
        (point_enabled && !prepared))
        return 0;
    memset(&next, 0, sizeof(next));
    next.draw = draw;
    next.prepared = prepared;
    next.directional = directional;
    next.point_enabled = point_enabled;
    next.scale = scale;
    next.base_color = base_color;
    if (directional->enabled)
    {
        if (!world_to_model)
            return 0;
        for (i = 0; i < 12; ++i)
            if (!_finite(((const float *)world_to_model)[i]))
                return 0;
        if (!_finite(directional->direction.x) || !_finite(directional->direction.y) ||
            !_finite(directional->direction.z))
            return 0;
        for (i = 0; i < 3; ++i)
            if (!_finite(directional->directional[i]) || !_finite(directional->ambient[i]))
                return 0;
        bh_mat34_transform_direction(world_to_model, &directional->direction, &next.local_direction);
        if (!_finite(next.local_direction.x) || !_finite(next.local_direction.y) || !_finite(next.local_direction.z))
            return 0;
    }
    *c = next;
    return 1;
}
/* This owner has different x87 sum scheduling from00458470. */
static double lit_support(const BHBspNode *node, const BHViewPlane *p, int maximum)
{
    double x, y, z;
    float nx = p->normal.x, ny = p->normal.y, nz = p->normal.z;
    x = (double)nx * ((maximum ? (nx >= 0) : (nx < 0)) ? node->maximum[0] : node->minimum[0]);
    y = (double)ny * ((maximum ? (ny >= 0) : (ny < 0)) ? node->maximum[1] : node->minimum[1]);
    z = (double)nz * ((maximum ? (nz >= 0) : (nz < 0)) ? node->maximum[2] : node->minimum[2]);
    if (maximum ? (ny < 0 && nz >= 0) : (ny >= 0 && nz < 0))
        return (z + (x + y)) - p->offset;
    return (y + (x + z)) - p->offset;
}
static int truncate_channel(double value, unsigned long *out)
{
    int n;
    if (!_finite(value) || value < -2147483648.0 || value > 2147483647.0)
        return 0;
    n = (int)value;
    *out = n < 0 ? 0 : n > 255 ? 255 : (unsigned long)n;
    return 1;
}
static int shade(BHModelDrawLitContext *c, const BHBspNode *node)
{
    BHVec3 point, normal;
    unsigned long dynamic = 0, red = 0, green = 0, blue = 0;
    double alignment, side;
    BHModelDrawContext *draw = c->draw;
    normal.x = node->normal[0];
    normal.y = node->normal[1];
    normal.z = node->normal[2];
    if (c->point_enabled)
    {
        bh_round_float(&point.x, ((double)node->minimum[0] + node->maximum[0]) * .5);
        bh_round_float(&point.y, ((double)node->minimum[1] + node->maximum[1]) * .5);
        bh_round_float(&point.z, ((double)node->minimum[2] + node->maximum[2]) * .5);
        if (!bh_light_evaluate_prepared(c->prepared, &point, &normal, c->scale, &dynamic))
            return fail(c, BH_MODEL_DRAW_LIT_LIGHT);
        red = (dynamic >> 16) & 255;
        green = (dynamic >> 8) & 255;
        blue = dynamic & 255;
    }
    if (c->directional->enabled)
    {
        alignment = (((double)normal.x * c->local_direction.x + (double)normal.y * c->local_direction.y) +
                     (double)normal.z * c->local_direction.z) *
                    c->scale;
        side = ((double)normal.z * draw->view->position.z + (double)normal.y * draw->view->position.y) +
               (double)normal.x * draw->view->position.x + node->plane;
        /* 0045a403 takes the negative-side branch to0045a5d8; that path
           negates the dot at0045a6b6. Nonnegative side has no FCHS. */
        if (side < 0)
            alignment = -alignment;
        if (alignment < 0)
            alignment = 0;
        if (!truncate_channel((alignment * c->directional->directional[0] + red) + c->directional->ambient[0], &red) ||
            !truncate_channel((alignment * c->directional->directional[1] + green) + c->directional->ambient[1],
                              &green) ||
            !truncate_channel((alignment * c->directional->directional[2] + blue) + c->directional->ambient[2], &blue))
            return fail(c, BH_MODEL_DRAW_LIT_NUMERICAL);
    }
    draw->color = ((red * 256 + green) * 256 + blue) + c->base_color;
    ++c->nodes_shaded;
    return 1;
}
int __cdecl bh_model_draw_lit(BHModelDrawLitContext *c, long node, unsigned long mask)
{
    BHModelDrawContext *draw;
    unsigned long count = 1, slot, bit;
    BHModelDrawTask *task;
    const BHBspNode *n;
    double side;
    if (!c || !c->draw || !c->directional)
        return fail(c, BH_MODEL_DRAW_LIT_ARGUMENT);
    draw = c->draw;
    if (!draw->model || !draw->cache || !draw->cache->tasks || !draw->view)
        return fail(c, BH_MODEL_DRAW_LIT_ARGUMENT);
    c->error = BH_MODEL_DRAW_LIT_OK;
    draw->error = BH_MODEL_DRAW_OK;
    draw->cache->tasks[0].node = node;
    draw->cache->tasks[0].mask = mask;
    draw->cache->tasks[0].phase = 0;
    while (count)
    {
        task = draw->cache->tasks + count - 1;
        if (task->phase)
        {
            n = draw->model->nodes + task->node;
            if (!shade(c, n))
                return 0;
            if (!bh_model_draw_leaf_chain(draw, task->leaf))
                return fail(c, BH_MODEL_DRAW_LIT_DRAW);
            task->node = task->tail;
            task->phase = 0;
            continue;
        }
        if (task->node < 0)
        {
            --count;
            continue;
        }
        if ((unsigned long)task->node >= draw->model->node_count)
            return fail(c, BH_MODEL_DRAW_LIT_ARGUMENT);
        n = draw->model->nodes + task->node;
        ++draw->nodes_visited;
        for (slot = 0, bit = 1; slot < 6; ++slot, bit *= 2)
            if (task->mask & bit)
                if (lit_support(n, draw->view->planes + slot, 0) >= 0)
                    break;
        if (slot < 6)
        {
            --count;
            continue;
        }
        for (slot = 0, bit = 1; slot < 6; ++slot, bit *= 2)
            if (task->mask & bit)
                if (lit_support(n, draw->view->planes + slot, 1) < 0)
                    task->mask &= (0xffffUL ^ bit);
        side = ((double)n->normal[2] * draw->view->position.z + (double)n->normal[1] * draw->view->position.y) +
               (double)n->normal[0] * draw->view->position.x + n->plane;
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
        if (count >= draw->cache->task_capacity)
            return fail(c, BH_MODEL_DRAW_LIT_ARGUMENT);
        draw->cache->tasks[count].node = node;
        draw->cache->tasks[count].mask = task->mask;
        draw->cache->tasks[count].phase = 0;
        ++count;
    }
    return 1;
}
