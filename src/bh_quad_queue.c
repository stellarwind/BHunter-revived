#include "bh_quad_queue.h"
#include <string.h>
int __cdecl bh_quad_queue(BHDrawQueue *q, unsigned long bucket, unsigned long image, const BHVec3 *center,
                          const BHVec3 *up, const BHVec3 *normal, float width, float height, float angle,
                          unsigned long flags, float modulation, float u, float u_extent, float v, float v_extent)
{
    static const unsigned long headers[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long header, tail, index;
    BHDrawCommand *command;
    if (!q || !q->commands || !center || !up || !normal || bucket > 8 || q->count < 9 || q->count > q->capacity)
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
    header = headers[bucket];
    tail = q->commands[header].resource;
    if (tail >= q->count || (q->commands[tail].next != BH_DRAW_END && q->commands[tail].next >= q->count))
    {
        q->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    index = q->count++;
    command = q->commands + index;
    command->next = q->commands[tail].next;
    q->commands[tail].next = index;
    q->commands[header].resource = index;
    command->resource = image;
    memcpy(command->words, center, 12);
    memcpy(command->words + 3, up, 12);
    memcpy(command->words + 6, normal, 12);
    memcpy(command->words + 9, &width, 4);
    memcpy(command->words + 10, &height, 4);
    memcpy(command->words + 11, &angle, 4);
    memcpy(command->words + 12, &modulation, 4);
    command->words[13] = flags;
    memcpy(command->words + 14, &u, 4);
    memcpy(command->words + 15, &u_extent, 4);
    memcpy(command->words + 16, &v, 4);
    memcpy(command->words + 17, &v_extent, 4);
    command->kind = BH_DRAW_QUAD;
    q->error = BH_DRAW_OK;
    return 1;
}
int __cdecl bh_quad_queue_execute(BHQuadQueueContext *c, const BHDrawCommand *command, unsigned long *next)
{
    BHTextureImage *image;
    float values[18];
    BHVec3 center, up, normal;
    int ok;
    if (!c || !c->draw || !c->lookup || !command || !next || command->kind != BH_DRAW_QUAD)
        return 0;
    image = c->lookup(command->resource, c->lookup_user);
    if (!image)
    {
        *next = command->next;
        return 0;
    }
    memcpy(values, command->words, sizeof(values));
    memcpy(&center, values, 12);
    memcpy(&up, values + 3, 12);
    memcpy(&normal, values + 6, 12);
    ok = bh_sprite_quad_draw(c->draw, image, &center, &normal, &up, values[9], values[10], values[11],
                             (unsigned char)command->words[13], values[12], values[14], values[15], values[16],
                             values[17]);
    *next = command->next;
    return ok;
}
