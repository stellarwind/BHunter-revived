#include "bh_path_queue.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>

static int fail(BHPathQueueContext *c, BHPathQueueError error)
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
static void point(const BHDrawCommand *p, BHVec3 *out)
{
    out->x = word(&p->resource);
    out->y = word(p->words);
    out->z = word(p->words + 1);
}
static int binding(const BHPathQueueContext *c)
{
    return c && c->draw && c->draw->path && c->camera && c->view && c->draw->used <= c->draw->capacity &&
           c->draw->capacity && _finite(c->view->near_z) && c->view->near_z > 0;
}
int __cdecl bh_path_queue_append(BHPathQueueContext *c, const BHVec3 *p, float width, unsigned long flags,
                                 float texture_start, float texture_end)
{
    BHSpriteOrientedVertex *out;
    BHVec3 local;
    unsigned long index;
    if (!binding(c) || !p)
        return fail(c, BH_PATH_QUEUE_ARGUMENT);
    if (!_finite(p->x) || !_finite(p->y) || !_finite(p->z) || !_finite(width) || !_finite(texture_start) ||
        !_finite(texture_end))
        return fail(c, BH_PATH_QUEUE_NUMERICAL);
    if (c->draw->used >= c->draw->capacity)
        return fail(c, BH_PATH_QUEUE_CAPACITY);
    local = *p;
    index = c->draw->used++;
    out = c->draw->path + index;
    out->previous = out->next = NULL;
    if (index)
    {
        out->previous = out - 1;
        out[-1].next = out;
    }
    bh_mat34_transform_point(&c->camera->projected, &local, &out->point);
    out->width = width;
    out->flags = flags;
    out->texture_start = texture_start;
    out->texture_end = texture_end;
    if (!_finite(out->point.x) || !_finite(out->point.y) || !_finite(out->point.z))
        return fail(c, BH_PATH_QUEUE_NUMERICAL);
    ++c->accepted;
    return 1;
}
static int append_command(BHPathQueueContext *c, const BHDrawCommand *p)
{
    BHVec3 position;
    point(p, &position);
    return bh_path_queue_append(c, &position, word(p->words + 2), 0, word(p->words + 3), word(p->words + 4));
}
static long select_path(BHPathQueueContext *c, const BHDrawCommand *points, unsigned long spans, unsigned long mask,
                        unsigned long depth)
{
    BHVec3 a, b, mid;
    float radius, stored_depth;
    double adjusted;
    unsigned long half;
    long first, second;
    int accept = spans == 1;
    if (depth > c->deepest)
        c->deepest = depth;
    if (!accept)
    {
        point(points, &a);
        point(points + spans, &b);
        bh_round_float(&radius, (double)word(points->words + 5) - word(points[spans].words + 5));
        if (!_finite(radius) || radius < 0)
        {
            fail(c, BH_PATH_QUEUE_NUMERICAL);
            return -1;
        }
        bh_round_float(&mid.x, ((double)a.x + b.x) * .5);
        bh_round_float(&mid.y, ((double)a.y + b.y) * .5);
        bh_round_float(&mid.z, ((double)b.z + a.z) * .5);
        mask = bh_view_cull_sphere(c->view, c->camera, mid.x, mid.y, mid.z, radius, mask, &stored_depth);
        if ((long)mask < 0)
        {
            ++c->culled;
            return 0;
        }
        adjusted = (double)stored_depth - (double)radius * .5f;
        /* A visible zero-radius sphere has positive adjusted depth. Execute
           the actual masked division: +infinity and its sticky status bit
           are both part of the original path selector's numerical behavior. */
        if ((double)radius < adjusted && adjusted / (double)radius > 16.0f)
            accept = 1;
        if (!spans && !accept)
        {
            fail(c, BH_PATH_QUEUE_NUMERICAL);
            return -1;
        }
    }
    if (accept)
        return append_command(c, points) ? 1 : -1;
    half = spans / 2;
    first = select_path(c, points, half, mask, depth + 1);
    if (first < 0)
        return -1;
    second = select_path(c, points + half, spans - half, mask, depth + 1);
    if (second < 0)
        return -1;
    return first + second;
}
long __cdecl bh_path_queue_select(BHPathQueueContext *c, const BHDrawCommand *points, unsigned long spans,
                                  unsigned long plane_mask)
{
    if (!binding(c) || !points || spans > 65534)
    {
        fail(c, BH_PATH_QUEUE_ARGUMENT);
        return -1;
    }
    return select_path(c, points, spans, plane_mask, 0);
}
int __cdecl bh_path_queue_execute(BHPathQueueContext *c, const BHDrawQueue *q, const BHDrawCommand *header,
                                  unsigned long *next)
{
    unsigned long index, count, i, half;
    long first, second;
    int ok = 1;
    const BHDrawCommand *last;
    BHTextureImage *image;
    if (!binding(c) || !q || !q->commands || !header || !next || !c->lookup || header->kind != BH_DRAW_PATH_HEADER)
        return fail(c, BH_PATH_QUEUE_ARGUMENT);
    c->accepted = c->culled = c->deepest = 0;
    c->error = BH_PATH_QUEUE_OK;
    /* The API borrows a command in this real queue allocation. Integer range
       tests avoid subtracting a foreign pointer before its bounds are known. */
    if ((unsigned long)header < (unsigned long)q->commands ||
        ((unsigned long)header - (unsigned long)q->commands) % sizeof(*header))
        return fail(c, BH_PATH_QUEUE_TOPOLOGY);
    index = ((unsigned long)header - (unsigned long)q->commands) / sizeof(*header);
    if (index >= q->count || q->count > q->capacity)
        return fail(c, BH_PATH_QUEUE_TOPOLOGY);
    count = header->words[0];
    if (!count || count >= q->count - index || header->next != index + 1)
        return fail(c, BH_PATH_QUEUE_TOPOLOGY);
    last = header + count;
    for (i = 1; i <= count; ++i)
        if (header[i].kind != BH_DRAW_PATH_POINT || (i < count && header[i].next != index + i + 1))
            return fail(c, BH_PATH_QUEUE_TOPOLOGY);
    image = c->lookup(header->resource, c->lookup_user);
    if (!image)
    {
        *next = last->next;
        return fail(c, BH_PATH_QUEUE_IMAGE);
    }
    half = (count - 1) / 2;
    first = select_path(c, header + 1, half, 0x3f, 0);
    second = first < 0 ? -1 : select_path(c, header + 1 + half, count - 1 - half, 0x3f, 0);
    if (first < 0 || second < 0)
        ok = 0;
    else if (first + second)
    {
        if (!append_command(c, last))
            ok = 0;
        else
        {
            c->draw->image = image;
            if (!bh_sprite_oriented_draw(c->draw))
            {
                fail(c, BH_PATH_QUEUE_DRAW);
                ok = 0;
            }
        }
    }
    *next = last->next;
    return ok;
}
