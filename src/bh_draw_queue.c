#include "bh_draw_queue.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char command_size[sizeof(BHDrawCommand) == 0x54 ? 1 : -1];
typedef char command_link[offsetof(BHDrawCommand, next) == 0x50 ? 1 : -1];
static void *__cdecl allocate(unsigned long size, void *user)
{
    return malloc(size);
}
static void __cdecl release(void *pointer, void *user)
{
    free(pointer);
}
int __cdecl bh_draw_queue_init(BHDrawQueue *q, unsigned long capacity, const BHSceneAllocator *allocator)
{
    BHDrawQueue next;
    if (!q || capacity < 9 || capacity > 65535 || (allocator && (!allocator->allocate || !allocator->release)))
        return 0;
    memset(&next, 0, sizeof(next));
    if (allocator)
        next.allocator = *allocator;
    else
    {
        next.allocator.allocate = allocate;
        next.allocator.release = release;
    }
    next.capacity = capacity;
    next.commands = (BHDrawCommand *)next.allocator.allocate(capacity * sizeof(BHDrawCommand), next.allocator.user);
    if (!next.commands)
        return 0;
    memset(next.commands, 0, capacity * sizeof(BHDrawCommand));
    *q = next;
    return bh_draw_queue_begin(q);
}
int __cdecl bh_draw_queue_dispose(BHDrawQueue *q)
{
    if (!q)
        return 0;
    if (q->commands)
        q->allocator.release(q->commands, q->allocator.user);
    memset(q, 0, sizeof(*q));
    return 1;
}
int __cdecl bh_draw_queue_begin(BHDrawQueue *q)
{
    unsigned long i;
    if (!q || !q->commands || q->capacity < 9)
        return 0;
    q->count = 9;
    q->error = BH_DRAW_OK;
    for (i = 0; i < 9; ++i)
    {
        q->commands[i].kind = (i == 0 || i == 6 || i == 8) ? BH_DRAW_NOP : BH_DRAW_STATE;
        q->commands[i].resource = i; /* original header's tail pointer, as an index */
        q->commands[i].next = i == 8 ? BH_DRAW_END : i + 1;
        if (i >= 1 && i <= 5)
            q->commands[i].words[0] = i - 1;
        if (i == 7)
            q->commands[i].words[0] = 3;
    }
    return 1;
}
static BHDrawCommand *append(BHDrawQueue *q, unsigned long bucket)
{
    static const unsigned long mapping[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long header, tail, index;
    BHDrawCommand *command;
    if (!q || !q->commands || q->count < 9 || q->count > q->capacity)
    {
        if (q)
            q->error = BH_DRAW_ARGUMENT;
        return NULL;
    }
    if (q->count == q->capacity)
    {
        q->error = BH_DRAW_FULL;
        return NULL;
    }
    header = bucket < 9 ? mapping[bucket] : 0;
    tail = q->commands[header].resource;
    if (tail >= q->count || (q->commands[tail].next != BH_DRAW_END && q->commands[tail].next >= q->count))
    {
        q->error = BH_DRAW_TOPOLOGY;
        return NULL;
    }
    index = q->count++;
    command = q->commands + index;
    command->next = q->commands[tail].next;
    q->commands[tail].next = index;
    q->commands[header].resource = index;
    q->error = BH_DRAW_OK;
    return command;
}
static void put(unsigned long *out, float value)
{
    memcpy(out, &value, 4);
}
static void point(unsigned long *out, const BHVec3 *value)
{
    memcpy(out, value, 12);
}
static int unavailable(const BHDrawQueue *q)
{
    return q && q->error == BH_DRAW_FULL ? 0 : -1;
}
int __cdecl bh_draw_billboard(BHDrawQueue *q, unsigned long bucket, unsigned long resource, const BHVec3 *position,
                              float width, float height, float angle, unsigned long frame, float alpha)
{
    BHDrawCommand *command;
    if (!position)
        return -1;
    command = append(q, bucket);
    if (!command)
        return unavailable(q);
    command->kind = BH_DRAW_BILLBOARD;
    command->resource = resource;
    point(command->words, position);
    put(command->words + 3, width);
    put(command->words + 4, height);
    put(command->words + 5, angle);
    command->words[7] = frame;
    put(command->words + 6, alpha);
    return 1;
}
int __cdecl bh_draw_segment(BHDrawQueue *q, unsigned long bucket, unsigned long resource, const BHVec3 *from,
                            const BHVec3 *to, float first_width, float second_width, float first_alpha,
                            float second_alpha, unsigned long control)
{
    BHDrawCommand *command;
    if (!from || !to)
        return -1;
    command = append(q, bucket);
    if (!command)
        return unavailable(q);
    command->kind = BH_DRAW_SEGMENT;
    command->resource = resource;
    point(command->words, from);
    point(command->words + 3, to);
    put(command->words + 6, first_width);
    put(command->words + 7, second_width);
    put(command->words + 8, first_alpha);
    put(command->words + 9, second_alpha);
    command->words[10] = control;
    return 1;
}
int __cdecl bh_draw_oriented(BHDrawQueue *q, unsigned long bucket, unsigned long resource, const BHVec3 *position,
                             const BHVec3 *up, const BHVec3 *normal, float width, float height, float angle,
                             unsigned long control, float alpha)
{
    BHDrawCommand *command;
    if (!position || !up || !normal)
        return -1;
    command = append(q, bucket);
    if (!command)
        return unavailable(q);
    command->resource = resource;
    point(command->words, position);
    point(command->words + 3, up);
    point(command->words + 6, normal);
    put(command->words + 9, width);
    put(command->words + 10, height);
    put(command->words + 11, angle);
    command->kind = BH_DRAW_ORIENTED;
    command->words[13] = control;
    put(command->words + 12, alpha);
    return 1;
}
