#include "bh_horizon_queue.h"
#include <string.h>

int bh_horizon_queue(BHDrawQueue *queue, unsigned long texture, float texture_scale, float height, float outer_radius,
                     float inner_radius, unsigned long color)
{
    unsigned long tail, index;
    BHDrawCommand *command;
    if (!queue || !queue->commands || queue->count < 9 || queue->count > queue->capacity)
    {
        if (queue)
            queue->error = BH_DRAW_ARGUMENT;
        return -1;
    }
    if (queue->count == queue->capacity)
    {
        queue->error = BH_DRAW_FULL;
        return 0;
    }
    tail = queue->commands[0].resource;
    if (tail >= queue->count || (queue->commands[tail].next != BH_DRAW_END && queue->commands[tail].next >= queue->count))
    {
        queue->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    index = queue->count++;
    command = queue->commands + index;
    command->next = queue->commands[tail].next;
    queue->commands[tail].next = index;
    queue->commands[0].resource = index;
    command->resource = texture;
    memcpy(command->words, &texture_scale, 4);
    memcpy(command->words + 4, &height, 4);
    memcpy(command->words + 5, &outer_radius, 4);
    command->kind = BH_DRAW_HORIZON;
    memcpy(command->words + 6, &inner_radius, 4);
    command->words[7] = color;
    queue->error = BH_DRAW_OK;
    return 1;
}
int bh_horizon_queue_execute(BHHorizonQueueContext *context, const BHDrawQueue *queue, const BHDrawCommand *command,
                             unsigned long *next)
{
    BHTextureImage *image;
    float texture_scale, height, outer_radius, inner_radius;
    int ok;
    if (!context || !context->draw || !context->draw->state || !context->lookup || !queue || !queue->commands || !command || !next ||
        command->kind != BH_DRAW_HORIZON || (command->next != BH_DRAW_END && command->next >= queue->count))
        return 0;
    ok = bh_render_mode(context->draw->state, 0);
    image = context->lookup(command->resource, context->lookup_user);
    memcpy(&texture_scale, command->words, 4);
    memcpy(&height, command->words + 4, 4);
    memcpy(&outer_radius, command->words + 5, 4);
    memcpy(&inner_radius, command->words + 6, 4);
    if (!image ||
        !bh_horizon_draw(context->draw, image, texture_scale, height, outer_radius, inner_radius, command->words[7]))
        ok = 0;
    *next = command->next;
    return ok;
}
