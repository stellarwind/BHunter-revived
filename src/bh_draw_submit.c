#include "bh_draw_submit.h"
#include <string.h>

/* 00478170 inlines this insertion before writing its resource payload.
   Keep its source owner here while the earlier queue producers retain their
   admitted translation unit and source variant. */
static BHDrawCommand *append(BHDrawQueue *queue, unsigned long bucket)
{
    static const unsigned long mapping[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long header, tail, index;
    BHDrawCommand *command;
    if (!queue || !queue->commands || queue->count < 9 || queue->count > queue->capacity)
    {
        if (queue)
            queue->error = BH_DRAW_ARGUMENT;
        return 0;
    }
    if (queue->count == queue->capacity)
    {
        queue->error = BH_DRAW_FULL;
        return 0;
    }
    header = bucket < 9 ? mapping[bucket] : 0;
    tail = queue->commands[header].resource;
    if (tail >= queue->count ||
        (queue->commands[tail].next != BH_DRAW_END && queue->commands[tail].next >= queue->count))
    {
        queue->error = BH_DRAW_TOPOLOGY;
        return 0;
    }
    index = queue->count++;
    command = queue->commands + index;
    command->next = queue->commands[tail].next;
    queue->commands[tail].next = index;
    queue->commands[header].resource = index;
    queue->error = BH_DRAW_OK;
    return command;
}

int __cdecl bh_draw_submit_resource(BHDrawQueue *queue, unsigned long bucket, unsigned long handle,
                                    const BHVec3 *position, const BHVec3 *angles, float modulation)
{
    BHDrawCommand *command;
    if (!position || !angles)
    {
        if (queue)
            queue->error = BH_DRAW_ARGUMENT;
        return -1;
    }
    command = append(queue, bucket);
    if (!command)
        return queue && queue->error == BH_DRAW_FULL ? 0 : -1;
    command->kind = BH_DRAW_MODEL_RESOURCE;
    command->resource = handle;
    memcpy(command->words, position, 3 * sizeof(float));
    memcpy(command->words + 3, angles, 3 * sizeof(float));
    memcpy(command->words + 6, &modulation, sizeof(float));
    return 1;
}

int __cdecl bh_draw_submit_execute(BHDrawSubmitContext *context, const BHDrawCommand *command, unsigned long *next)
{
    const BHDrawResource *resource;
    unsigned long handle;
    float pose[6], modulation;
    int status;
    if (!context || !context->draw || !context->lookup || !command || !next || command->kind != BH_DRAW_MODEL_RESOURCE)
        return 0;
    handle = command->resource;
    memcpy(pose, command->words, sizeof(pose));
    memcpy(&modulation, command->words + 6, sizeof(modulation));
    resource = context->lookup(handle, context->lookup_user);
    if (!resource)
    {
        *next = command->next;
        return 0;
    }
    status = bh_draw_resource(context->draw, resource, (double)pose[0], (double)pose[1], (double)pose[2],
                              (double)pose[3], (double)pose[4], (double)pose[5], modulation);
    *next = command->next;
    return status;
}

int __cdecl bh_draw_submit_billboard_execute(BHDrawBillboardSubmitContext *context, const BHDrawCommand *command,
                                             unsigned long *next)
{
    BHTextureImage *image;
    unsigned long handle, flags;
    float payload[7];
    int status;
    if (!context || !context->draw || !context->lookup || !command || !next || command->kind != BH_DRAW_BILLBOARD)
        return 0;
    handle = command->resource;
    memcpy(payload, command->words, sizeof(payload));
    flags = command->words[7] & 255UL;
    image = context->lookup(handle, context->lookup_user);
    if (!image)
    {
        *next = command->next;
        return 0;
    }
    status = bh_sprite_billboard_draw(context->draw, image, payload[0], payload[1], payload[2], payload[3], payload[4],
                                      payload[5], flags, payload[6]);
    *next = command->next;
    return status;
}
