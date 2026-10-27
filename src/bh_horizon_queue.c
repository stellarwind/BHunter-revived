#include "bh_horizon_queue.h"
#include <string.h>

int __cdecl bh_horizon_queue(BHDrawQueue *q, unsigned long texture, float texture_scale, float height,
                             float outer_radius, float inner_radius, unsigned long color)
{
    unsigned long tail, index;
    BHDrawCommand *command;
    if (!q || !q->commands || q->count < 9 || q->count > q->capacity)
    {
        if (q)
            q->error = BH_DRAW_ARGUMENT;
        return -1;
    }
    if (q->count == q->capacity)
    {
        q->error = BH_DRAW_FULL;
        return 0;
    }
    tail = q->commands[0].resource;
    if (tail >= q->count || (q->commands[tail].next != BH_DRAW_END && q->commands[tail].next >= q->count))
    {
        q->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    index = q->count++;
    command = q->commands + index;
    command->next = q->commands[tail].next;
    q->commands[tail].next = index;
    q->commands[0].resource = index;
    command->resource = texture;
    memcpy(command->words, &texture_scale, 4);
    memcpy(command->words + 4, &height, 4);
    memcpy(command->words + 5, &outer_radius, 4);
    command->kind = BH_DRAW_HORIZON;
    memcpy(command->words + 6, &inner_radius, 4);
    command->words[7] = color;
    q->error = BH_DRAW_OK;
    return 1;
}
int __cdecl bh_horizon_queue_execute(BHHorizonQueueContext *c, const BHDrawQueue *q, const BHDrawCommand *command,
                                     unsigned long *next)
{
    BHTextureImage *image;
    float texture_scale, height, outer_radius, inner_radius;
    int ok;
    if (!c || !c->draw || !c->draw->state || !c->lookup || !q || !q->commands || !command || !next ||
        command->kind != BH_DRAW_HORIZON || (command->next != BH_DRAW_END && command->next >= q->count))
        return 0;
    ok = bh_render_mode(c->draw->state, 0);
    image = c->lookup(command->resource, c->lookup_user);
    memcpy(&texture_scale, command->words, 4);
    memcpy(&height, command->words + 4, 4);
    memcpy(&outer_radius, command->words + 5, 4);
    memcpy(&inner_radius, command->words + 6, 4);
    if (!image ||
        !bh_horizon_draw(c->draw, image, texture_scale, height, outer_radius, inner_radius, command->words[7]))
        ok = 0;
    *next = command->next;
    return ok;
}
