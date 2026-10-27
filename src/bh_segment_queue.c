#include "bh_segment_queue.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>

static int fail(BHSegmentQueueContext *c, BHSegmentQueueError error)
{
    if (c)
        c->error = error;
    return 0;
}
static float word(const unsigned long *p)
{
    float value;
    memcpy(&value, p, 4);
    return value;
}
static int binding(const BHSegmentQueueContext *c)
{
    return c && c->camera && c->draw && c->draw->path && c->draw->capacity && c->draw->capacity <= 0x7fffffffUL &&
           c->draw->used <= c->draw->capacity;
}
int __cdecl bh_segment_queue_append(BHSegmentQueueContext *c, const BHVec3 *p, float width, unsigned long flags,
                                    float modulation)
{
    unsigned long index;
    BHSpriteOrientedVertex *out;
    BHVec3 local;
    if (!binding(c) || !p)
        return fail(c, BH_SEGMENT_QUEUE_ARGUMENT);
    if (!_finite(p->x) || !_finite(p->y) || !_finite(p->z) || !_finite(width) || !_finite(modulation))
        return fail(c, BH_SEGMENT_QUEUE_NUMERICAL);
    if (c->draw->used == c->draw->capacity)
        return fail(c, BH_SEGMENT_QUEUE_CAPACITY);
    index = c->draw->used++;
    out = c->draw->path + index;
    out->previous = out->next = NULL;
    if (index)
    {
        out->previous = out - 1;
        out[-1].next = out;
    }
    local = *p;
    bh_mat34_transform_point(&c->camera->projected, &local, &out->point);
    out->width = width;
    out->flags = flags;
    out->texture_start = modulation;
    bh_round_float(&out->texture_end, (double)(long)(c->draw->used - 1));
    if (!_finite(out->point.x) || !_finite(out->point.y) || !_finite(out->point.z))
        return fail(c, BH_SEGMENT_QUEUE_NUMERICAL);
    ++c->accepted;
    c->error = BH_SEGMENT_QUEUE_OK;
    return 1;
}
int __cdecl bh_segment_queue_execute(BHSegmentQueueContext *c, const BHDrawCommand *command, unsigned long *next)
{
    BHVec3 point;
    BHTextureImage *image;
    int ok = 1;
    if (!binding(c) || !command || !next || !c->lookup || command->kind != BH_DRAW_SEGMENT)
        return fail(c, BH_SEGMENT_QUEUE_ARGUMENT);
    c->accepted = 0;
    c->error = BH_SEGMENT_QUEUE_OK;
    memcpy(&point, command->words, 12);
    if (!bh_segment_queue_append(c, &point, word(command->words + 6), command->words[10], word(command->words + 8)))
        ok = 0;
    if (ok)
    {
        memcpy(&point, command->words + 3, 12);
        if (!bh_segment_queue_append(c, &point, word(command->words + 7), command->words[10], word(command->words + 9)))
            ok = 0;
    }
    if (ok)
    {
        image = c->lookup(command->resource, c->lookup_user);
        if (!image)
        {
            fail(c, BH_SEGMENT_QUEUE_IMAGE);
            ok = 0;
        }
        else
        {
            c->draw->image = image;
            if (!bh_sprite_oriented_draw(c->draw))
            {
                fail(c, BH_SEGMENT_QUEUE_DRAW);
                ok = 0;
            }
        }
    }
    *next = command->next;
    return ok;
}
