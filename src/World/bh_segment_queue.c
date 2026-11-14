#include "bh_segment_queue.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <string.h>

static int fail(BHSegmentQueueContext *context, BHSegmentQueueError error)
{
    if (context)
        context->error = error;
    return 0;
}
static float word(const unsigned long *p)
{
    float value;
    memcpy(&value, p, 4);
    return value;
}
static int binding(const BHSegmentQueueContext *context)
{
    return context && context->camera && context->draw && context->draw->path && context->draw->capacity && context->draw->capacity <= 0x7fffffffUL &&
           context->draw->used <= context->draw->capacity;
}
int bh_segment_queue_append(BHSegmentQueueContext *context, const BHVec3 *point, float width, unsigned long flags,
                            float modulation)
{
    unsigned long index;
    BHSpriteOrientedVertex *out;
    BHVec3 local;
    if (!binding(context) || !point)
        return fail(context, BH_SEGMENT_QUEUE_ARGUMENT);
    if (!_finite(point->x) || !_finite(point->y) || !_finite(point->z) || !_finite(width) || !_finite(modulation))
        return fail(context, BH_SEGMENT_QUEUE_NUMERICAL);
    if (context->draw->used == context->draw->capacity)
        return fail(context, BH_SEGMENT_QUEUE_CAPACITY);
    index = context->draw->used++;
    out = context->draw->path + index;
    out->previous = out->next = NULL;
    if (index)
    {
        out->previous = out - 1;
        out[-1].next = out;
    }
    local = *point;
    bh_mat34_transform_point(&context->camera->projected, &local, &out->point);
    out->width = width;
    out->flags = flags;
    out->texture_start = modulation;
    bh_round_float(&out->texture_end, (double)(long)(context->draw->used - 1));
    if (!_finite(out->point.x) || !_finite(out->point.y) || !_finite(out->point.z))
        return fail(context, BH_SEGMENT_QUEUE_NUMERICAL);
    ++context->accepted;
    context->error = BH_SEGMENT_QUEUE_OK;
    return 1;
}
int bh_segment_queue_execute(BHSegmentQueueContext *context, const BHDrawCommand *command, unsigned long *next)
{
    BHVec3 point;
    BHTextureImage *image;
    int ok = 1;
    if (!binding(context) || !command || !next || !context->lookup || command->kind != BH_DRAW_SEGMENT)
        return fail(context, BH_SEGMENT_QUEUE_ARGUMENT);
    context->accepted = 0;
    context->error = BH_SEGMENT_QUEUE_OK;
    memcpy(&point, command->words, 12);
    if (!bh_segment_queue_append(context, &point, word(command->words + 6), command->words[10], word(command->words + 8)))
        ok = 0;
    if (ok)
    {
        memcpy(&point, command->words + 3, 12);
        if (!bh_segment_queue_append(context, &point, word(command->words + 7), command->words[10], word(command->words + 9)))
            ok = 0;
    }
    if (ok)
    {
        image = context->lookup(command->resource, context->lookup_user);
        if (!image)
        {
            fail(context, BH_SEGMENT_QUEUE_IMAGE);
            ok = 0;
        }
        else
        {
            context->draw->image = image;
            if (!bh_sprite_oriented_draw(context->draw))
            {
                fail(context, BH_SEGMENT_QUEUE_DRAW);
                ok = 0;
            }
        }
    }
    *next = command->next;
    return ok;
}
