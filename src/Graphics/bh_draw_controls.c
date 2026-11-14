#include "bh_draw_controls.h"
#include <string.h>
static BHDrawCommand *append(BHDrawQueue *queue, unsigned long bucket)
{
    static const unsigned long mapping[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long header, tail, index;
    BHDrawCommand *command;
    if (!queue || !queue->commands || queue->count < 9 || queue->count > queue->capacity)
    {
        if (queue)
            queue->error = BH_DRAW_ARGUMENT;
        return NULL;
    }
    if (queue->count == queue->capacity)
    {
        queue->error = BH_DRAW_FULL;
        return NULL;
    }
    header = bucket < 9 ? mapping[bucket] : 0;
    tail = queue->commands[header].resource;
    if (tail >= queue->count || (queue->commands[tail].next != BH_DRAW_END && queue->commands[tail].next >= queue->count))
    {
        queue->error = BH_DRAW_TOPOLOGY;
        return NULL;
    }
    index = queue->count++;
    command = queue->commands + index;
    command->next = queue->commands[tail].next;
    queue->commands[tail].next = index;
    queue->commands[header].resource = index;
    queue->error = BH_DRAW_OK;
    return command;
}
int bh_draw_controls_state(BHDrawQueue *queue, unsigned long bucket, long value)
{
    BHDrawCommand *command = append(queue, bucket);
    if (!command)
        return queue && queue->error == BH_DRAW_FULL ? 0 : -1;
    command->kind = BH_DRAW_TEXTURE_MASK;
    command->resource = (unsigned long)value;
    return 1;
}
int bh_draw_controls_clip_rect(BHDrawQueue *queue, unsigned long bucket, float x, float y, float width, float height)
{
    BHDrawCommand *command = append(queue, bucket);
    float values[4];
    if (!command)
        return queue && queue->error == BH_DRAW_FULL ? 0 : -1;
    values[0] = x;
    values[1] = y;
    values[2] = width;
    values[3] = height;
    command->kind = BH_DRAW_CLIP_RECT;
    command->resource = 0;
    memcpy(command->words, values, sizeof(values));
    return 1;
}
int bh_draw_controls_portal_clip(BHDrawQueue *queue, unsigned long bucket, const BHVec3 corners[4], unsigned long parent,
                                 float near_z, float far_z)
{
    BHDrawCommand *command;
    if (!corners || (parent != BH_DRAW_END && (!queue || parent >= queue->count)))
        return -1;
    command = append(queue, bucket);
    if (!command)
        return queue && queue->error == BH_DRAW_FULL ? 0 : -1;
    command->kind = BH_DRAW_CLIP_RECT;
    command->resource = BH_PORTAL_CLIP_WORLD;
    memcpy(command->words, corners, 4 * sizeof(BHVec3));
    command->words[12] = parent;
    memcpy(command->words + 13, &near_z, 4);
    memcpy(command->words + 14, &far_z, 4);
    command->words[15] = 0;
    return 1;
}
int bh_draw_controls_texture_mask(BHDrawControlContext *context, unsigned long mask)
{
    unsigned long i;
    if (!context)
        return 0;
    context->texture_mask = mask;
    /* Explicit binding adapter: original 00491778 was shared by every model. */
    if (context->resources)
    {
        context->resources->prototype.texture_mask = mask;
        for (i = 1; i <= context->resources->count; ++i)
        {
            const BHDrawResource *draw_resource = bh_render_resources_lookup(i, context->resources);
            if (!draw_resource || draw_resource->tag != BH_RESOURCE_TAG_MODEL || !draw_resource->data.model.instance)
                return 0;
            draw_resource->data.model.instance->texture_mask = mask;
        }
    }
    return 1;
}
int bh_draw_controls_execute(BHDrawControlContext *context, const BHDrawQueue *queue, const BHDrawCommand *command,
                             unsigned long *next)
{
    float v[4];
    int ok;
    unsigned long following;
    if (!context || !queue || !queue->commands || !command || !next)
        return 0;
    following = command->next;
    if (following != BH_DRAW_END && following >= queue->count)
        return 0;
    if (command->kind == BH_DRAW_TEXTURE_MASK)
    {
        ok = bh_draw_controls_texture_mask(context, command->resource);
        *next = command->next;
        return ok;
    }
    if (command->kind != BH_DRAW_CLIP_RECT)
        return 0;
    if (following != BH_DRAW_END && queue->commands[following].kind == BH_DRAW_CLIP_RECT)
    {
        *next = following;
        return 1;
    }
    if (!context->view)
        return 0;
    if (command->resource == BH_PORTAL_CLIP_WORLD)
    {
        float bounds[4];
        if (!bh_portal_clip_project(queue, (unsigned long)(command - queue->commands), context->camera, context->view, context->portal_cache,
                                    bounds))
            return 0;
        v[0] = bounds[0];
        v[1] = bounds[1];
        v[2] = bounds[2] - bounds[0];
        v[3] = bounds[3] - bounds[1];
    }
    else
        memcpy(v, command->words, sizeof(v));
    ok = bh_view_clip_rect(context->view, v[0], v[1], v[2], v[3]);
    if (ok)
    {
        if (context->model_clip)
        {
            context->model_clip->left = context->view->left;
            context->model_clip->right = context->view->right;
            context->model_clip->top = context->view->top;
            context->model_clip->bottom = context->view->bottom;
        }
        if (context->screen_clip)
        {
            context->screen_clip->left = context->view->left;
            context->screen_clip->right = context->view->right;
            context->screen_clip->top = context->view->top;
            context->screen_clip->bottom = context->view->bottom;
        }
        if (context->attribute_clip)
        {
            context->attribute_clip->left = context->view->left;
            context->attribute_clip->right = context->view->right;
            context->attribute_clip->top = context->view->top;
            context->attribute_clip->bottom = context->view->bottom;
        }
    }
    *next = command->next;
    return ok;
}
