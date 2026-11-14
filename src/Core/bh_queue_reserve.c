#include "bh_queue_reserve.h"
#include <stddef.h>
static_assert(sizeof(BHDrawCommand) == 84, "BHDrawCommand size");
static_assert(offsetof(BHDrawCommand, next) == 0x50, "BHDrawCommand.next offset");
BHDrawCommand *bh_queue_reserve(BHDrawQueue *queue, long bucket)
{
    static const unsigned long headers[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
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
    header = (unsigned long)bucket < 9 ? headers[(unsigned long)bucket] : 0;
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
int bh_queue_mode(BHDrawQueue *queue, long bucket, unsigned long mode)
{
    BHDrawCommand *command = bh_queue_reserve(queue, bucket);
    if (!command)
        return queue && queue->error == BH_DRAW_FULL ? 0 : -1;
    command->kind = BH_DRAW_STATE;
    command->words[0] = mode;
    return 1;
}
int bh_queue_mode_execute(BHRenderState *state, const BHDrawCommand *command, unsigned long *next)
{
    int ok;
    if (!state || !command || !next)
        return 0;
    ok = bh_render_mode(state, command->words[0]);
    *next = command->next;
    return ok;
}
