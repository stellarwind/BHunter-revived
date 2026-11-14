#include "bh_material_queue.h"

int bh_material_queue_words(BHDrawQueue *queue, unsigned long bucket, unsigned long image, unsigned long word0,
                            unsigned long word1, unsigned long word2, unsigned long word3, unsigned long word4,
                            unsigned long word5, unsigned long word6)
{
    static const unsigned long mapping[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long header, tail, index;
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
    header = bucket < 9 ? mapping[bucket] : 0;
    tail = queue->commands[header].resource;
    if (tail >= queue->count || (queue->commands[tail].next != BH_DRAW_END && queue->commands[tail].next >= queue->count))
    {
        queue->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    index = queue->count++;
    command = queue->commands + index;
    command->next = queue->commands[tail].next;
    queue->commands[tail].next = index;
    queue->commands[header].resource = index;
    command->resource = image;
    command->words[0] = word0;
    command->words[1] = word1;
    command->words[2] = word2;
    command->words[3] = word3;
    command->words[4] = word4;
    command->words[5] = word5;
    command->words[6] = word6;
    command->kind = BH_DRAW_MATERIAL_WORDS;
    queue->error = BH_DRAW_OK;
    return 1;
}

int bh_material_queue_execute(BHDrawImageLookup lookup, void *user, const BHDrawCommand *command, unsigned long *next)
{
    BHTextureImage *image;
    int result;
    if (!lookup || !command || !next || command->kind != BH_DRAW_MATERIAL_WORDS)
        return 0;
    image = lookup(command->resource, user);
    result = bh_texture_image_set_words(image, command->words[0], command->words[1], command->words[2],
                                        command->words[3], command->words[4], command->words[5], command->words[6]);
    *next = command->next;
    return result;
}
