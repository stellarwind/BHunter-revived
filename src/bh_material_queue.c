#include "bh_material_queue.h"

int __cdecl bh_material_queue_words(BHDrawQueue *q, unsigned long bucket, unsigned long image, unsigned long word0,
                                    unsigned long word1, unsigned long word2, unsigned long word3, unsigned long word4,
                                    unsigned long word5, unsigned long word6)
{
    static const unsigned long mapping[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long header, tail, index;
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
    header = bucket < 9 ? mapping[bucket] : 0;
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
    command->words[0] = word0;
    command->words[1] = word1;
    command->words[2] = word2;
    command->words[3] = word3;
    command->words[4] = word4;
    command->words[5] = word5;
    command->words[6] = word6;
    command->kind = BH_DRAW_MATERIAL_WORDS;
    q->error = BH_DRAW_OK;
    return 1;
}

int __cdecl bh_material_queue_execute(BHDrawImageLookup lookup, void *user, const BHDrawCommand *command,
                                      unsigned long *next)
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
