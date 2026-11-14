#include "bh_sprite_oriented.h"
#include "../../Math/bh_round.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

static_assert(sizeof(BHSpriteOrientedVertex) == 0x54, "BHSpriteOrientedVertex size");
static_assert(offsetof(BHSpriteOrientedVertex, previous) == 0x3c, "BHSpriteOrientedVertex.previous offset");
static_assert(offsetof(BHSpriteOrientedVertex, next) == 0x40, "BHSpriteOrientedVertex.next offset");
static_assert(offsetof(BHSpriteOrientedVertex, flags) == 0x44, "BHSpriteOrientedVertex.flags offset");
static_assert(offsetof(BHSpriteOrientedVertex, width) == 0x4c, "BHSpriteOrientedVertex.width offset");

static int fail(BHSpriteOrientedContext *context, BHSpriteOrientedError e)
{
    context->error = e;
    return 0;
}
static int in_pool(const BHSpriteOrientedContext *context, const BHSpriteOrientedVertex *p)
{
    return p >= context->path && p < context->path + context->used;
}
static float interpolate(float a, float b, float za, float zb, float near_z)
{
    float out;
    bh_round_float(&out, (((double)near_z - zb) * a + ((double)za - near_z) * b) / ((double)za - zb));
    return out;
}
static BHSpriteOrientedVertex *intersection(BHSpriteOrientedContext *context, BHSpriteOrientedVertex *first_sprite_oriented_vertex,
                                            BHSpriteOrientedVertex *second_sprite_oriented_vertex)
{
    BHSpriteOrientedVertex *sprite_oriented_vertex;
    float az = first_sprite_oriented_vertex->point.z, bz = second_sprite_oriented_vertex->point.z;
    if (context->used >= context->capacity || az == bz)
        return NULL;
    sprite_oriented_vertex = context->path + context->used++;
    sprite_oriented_vertex->point.x = interpolate(first_sprite_oriented_vertex->point.x, second_sprite_oriented_vertex->point.x, az, bz, context->near_z);
    sprite_oriented_vertex->point.y = interpolate(first_sprite_oriented_vertex->point.y, second_sprite_oriented_vertex->point.y, az, bz, context->near_z);
    sprite_oriented_vertex->point.z = context->near_z;
    sprite_oriented_vertex->texture_start = interpolate(first_sprite_oriented_vertex->texture_start, second_sprite_oriented_vertex->texture_start, az, bz, context->near_z);
    sprite_oriented_vertex->width = interpolate(first_sprite_oriented_vertex->width, second_sprite_oriented_vertex->width, az, bz, context->near_z);
    sprite_oriented_vertex->texture_end = interpolate(first_sprite_oriented_vertex->texture_end, second_sprite_oriented_vertex->texture_end, az, bz, context->near_z);
    sprite_oriented_vertex->flags = first_sprite_oriented_vertex->flags;
    sprite_oriented_vertex->previous = sprite_oriented_vertex->next = NULL;
    return sprite_oriented_vertex;
}
static void set_scalar(BHClipVertex *vertex, float value)
{
    memcpy(&vertex->unknown28, &value, 4);
}
static void set_vertex(BHClipVertex *vertex, float x, float y, float z, float u, float t, float scalar)
{
    memset(vertex, 0, sizeof(*vertex));
    vertex->sx = x;
    vertex->sy = y;
    vertex->point.z = z;
    vertex->u = u;
    vertex->v = t;
    set_scalar(vertex, scalar);
}
static int polygon(BHSpriteOrientedContext *context, BHClipVertex *vertices, const unsigned char *indices,
                   unsigned long count)
{
    unsigned long i;
    BHClipVertex *ring;
    for (i = 0; i < count; ++i)
    {
        BHClipVertex *vertex = vertices + indices[i];
        vertex->next = vertices + indices[(i + 1) % count];
        vertex->previous = vertices + indices[(i + count - 1) % count];
    }
    ring = bh_attribute_clip_polygon(context->clip, vertices + indices[0], 0);
    if (!ring)
    {
        if (context->clip->error != BH_CLIP_OK)
            return fail(context, BH_SPRITE_ORIENTED_CLIP);
        return 1;
    }
    if (!context->emit(ring, context->image, context->color, context->emit_user))
        return fail(context, BH_SPRITE_ORIENTED_EMIT);
    ++context->polygons_emitted;
    return 1;
}
static int emit_segment(BHSpriteOrientedContext *c, BHSpriteOrientedVertex *first_sprite_oriented_vertex, BHSpriteOrientedVertex *second_sprite_oriented_vertex)
{
    BHClipVertex v[5];
    float near_t, far_t, near_side, far_side, middle;
    unsigned long flags = first_sprite_oriented_vertex->flags;
    unsigned char quad[4] = {0, 1, 3, 2};
    unsigned char narrow_far_0[3] = {4, 0, 1}, narrow_far_1[3] = {4, 1, 3}, narrow_far_2[3] = {0, 4, 2};
    unsigned char narrow_near_0[3] = {4, 2, 0}, narrow_near_1[3] = {4, 3, 2}, narrow_near_2[3] = {4, 1, 3};
    if (!_finite(first_sprite_oriented_vertex->texture_end) || (double)first_sprite_oriented_vertex->texture_end < -2147483648.0 || (double)first_sprite_oriented_vertex->texture_end >= 2147483648.0)
        return fail(c, BH_SPRITE_ORIENTED_NUMERICAL);
    near_t = first_sprite_oriented_vertex->texture_end - (float)(int)first_sprite_oriented_vertex->texture_end;
    far_t = second_sprite_oriented_vertex->texture_end - (float)(int)first_sprite_oriented_vertex->texture_end;
    if (flags & 1)
    {
        near_t = 1.0f - near_t;
        far_t = 1.0f - far_t;
    }
    near_side = (flags & 2) ? 1.0f : 0.0f;
    far_side = 1.0f - near_side;
    if (flags & 4)
    {
        set_vertex(v + 0, first_sprite_oriented_vertex->corners[0].x, first_sprite_oriented_vertex->corners[0].y, first_sprite_oriented_vertex->point.z, near_side, near_t, first_sprite_oriented_vertex->texture_start);
        set_vertex(v + 1, first_sprite_oriented_vertex->corners[1].x, first_sprite_oriented_vertex->corners[1].y, first_sprite_oriented_vertex->point.z, far_side, near_t, first_sprite_oriented_vertex->texture_start);
        set_vertex(v + 2, first_sprite_oriented_vertex->corners[3].x, first_sprite_oriented_vertex->corners[3].y, second_sprite_oriented_vertex->point.z, near_side, far_t, second_sprite_oriented_vertex->texture_start);
        set_vertex(v + 3, first_sprite_oriented_vertex->corners[2].x, first_sprite_oriented_vertex->corners[2].y, second_sprite_oriented_vertex->point.z, far_side, far_t, second_sprite_oriented_vertex->texture_start);
    }
    else
    {
        set_vertex(v + 0, first_sprite_oriented_vertex->corners[0].x, first_sprite_oriented_vertex->corners[0].y, first_sprite_oriented_vertex->point.z, near_t, near_side, first_sprite_oriented_vertex->texture_start);
        set_vertex(v + 1, first_sprite_oriented_vertex->corners[1].x, first_sprite_oriented_vertex->corners[1].y, first_sprite_oriented_vertex->point.z, near_t, far_side, first_sprite_oriented_vertex->texture_start);
        set_vertex(v + 2, first_sprite_oriented_vertex->corners[3].x, first_sprite_oriented_vertex->corners[3].y, second_sprite_oriented_vertex->point.z, far_t, near_side, second_sprite_oriented_vertex->texture_start);
        set_vertex(v + 3, first_sprite_oriented_vertex->corners[2].x, first_sprite_oriented_vertex->corners[2].y, second_sprite_oriented_vertex->point.z, far_t, far_side, second_sprite_oriented_vertex->texture_start);
    }
    if (first_sprite_oriented_vertex->width <= 2.0f * second_sprite_oriented_vertex->width)
    {
        if (second_sprite_oriented_vertex->width <= first_sprite_oriented_vertex->width)
            return polygon(c, v, quad, 4);
        /* Original 00455f70 introduces a far-edge midpoint and emits three
           rings for a widening segment. */
        middle = 0.5f;
        set_vertex(v + 4, (v[2].sx + v[3].sx) * middle, (v[2].sy + v[3].sy) * middle, second_sprite_oriented_vertex->point.z,
                   (v[2].u + v[3].u) * middle, (v[2].v + v[3].v) * middle, second_sprite_oriented_vertex->texture_start);
        return polygon(c, v, narrow_far_0, 3) && polygon(c, v, narrow_far_1, 3) && polygon(c, v, narrow_far_2, 3);
    }
    /* A sharply narrowing segment uses a near-edge midpoint. */
    middle = 0.5f;
    set_vertex(v + 4, (v[0].sx + v[1].sx) * middle, (v[0].sy + v[1].sy) * middle, first_sprite_oriented_vertex->point.z,
               (v[0].u + v[1].u) * middle, (v[0].v + v[1].v) * middle, first_sprite_oriented_vertex->texture_start);
    return polygon(c, v, narrow_near_0, 3) && polygon(c, v, narrow_near_1, 3) && polygon(c, v, narrow_near_2, 3);
}
int bh_sprite_oriented_ribbon(BHSpriteOrientedContext *context, BHSpriteOrientedVertex *head)
{
    BHSpriteOrientedVertex *sprite_oriented_vertex, *other_sprite_oriented_vertex;
    unsigned long n = 0;
    if (!context || !head || !context->path || !context->used || context->used > context->capacity || !context->clip || !context->emit ||
        !_finite(context->projection_scale))
        return context ? fail(context, BH_SPRITE_ORIENTED_ARGUMENT) : 0;
    for (sprite_oriented_vertex = head; sprite_oriented_vertex; sprite_oriented_vertex = sprite_oriented_vertex->next)
    {
        if (!in_pool(context, sprite_oriented_vertex) || ++n > context->used || !_finite(sprite_oriented_vertex->point.x) || !_finite(sprite_oriented_vertex->point.y) || !_finite(sprite_oriented_vertex->point.z) ||
            sprite_oriented_vertex->point.z == 0 || !_finite(sprite_oriented_vertex->width) || !_finite(sprite_oriented_vertex->texture_start) || !_finite(sprite_oriented_vertex->texture_end))
            return fail(context, BH_SPRITE_ORIENTED_TOPOLOGY);
        bh_round_float(&sprite_oriented_vertex->point.x, (double)sprite_oriented_vertex->point.x / sprite_oriented_vertex->point.z);
        bh_round_float(&sprite_oriented_vertex->point.y, (double)sprite_oriented_vertex->point.y / sprite_oriented_vertex->point.z);
        bh_round_float(&sprite_oriented_vertex->width, ((double)context->projection_scale * 0.5 / sprite_oriented_vertex->point.z) * sprite_oriented_vertex->width);
    }
    for (sprite_oriented_vertex = head; sprite_oriented_vertex && sprite_oriented_vertex->next; sprite_oriented_vertex = sprite_oriented_vertex->next)
    {
        BHVec3 dir;
        float dx, dy, aw, bw;
        other_sprite_oriented_vertex = sprite_oriented_vertex->next;
        dir.x = other_sprite_oriented_vertex->point.x - sprite_oriented_vertex->point.x;
        dir.y = other_sprite_oriented_vertex->point.y - sprite_oriented_vertex->point.y;
        dir.z = 0;
        /* 0045601b ignores the normalization result. A zero projected edge
           leaves dir zero and still builds its degenerate finite corners. */
        bh_vec3_normalize(&dir);
        dx = -dir.y;
        dy = dir.x;
        aw = sprite_oriented_vertex->width;
        bw = other_sprite_oriented_vertex->width;
        sprite_oriented_vertex->corners[0].x = sprite_oriented_vertex->point.x - dx * aw;
        sprite_oriented_vertex->corners[0].y = sprite_oriented_vertex->point.y - dy * aw;
        sprite_oriented_vertex->corners[0].z = 0;
        sprite_oriented_vertex->corners[1].x = sprite_oriented_vertex->point.x + dx * aw;
        sprite_oriented_vertex->corners[1].y = sprite_oriented_vertex->point.y + dy * aw;
        sprite_oriented_vertex->corners[1].z = 0;
        sprite_oriented_vertex->corners[2].x = other_sprite_oriented_vertex->point.x + dx * bw;
        sprite_oriented_vertex->corners[2].y = other_sprite_oriented_vertex->point.y + dy * bw;
        sprite_oriented_vertex->corners[2].z = 0;
        sprite_oriented_vertex->corners[3].x = other_sprite_oriented_vertex->point.x - dx * bw;
        sprite_oriented_vertex->corners[3].y = other_sprite_oriented_vertex->point.y - dy * bw;
        sprite_oriented_vertex->corners[3].z = 0;
    }
    for (sprite_oriented_vertex = head; sprite_oriented_vertex && sprite_oriented_vertex->next && sprite_oriented_vertex->next->next; sprite_oriented_vertex = sprite_oriented_vertex->next)
    {
        BHVec3 center, u, v, sum;
        double dot, den, scale, offset_z;
        float offset_x, offset_y;
        other_sprite_oriented_vertex = sprite_oriented_vertex->next;
        bh_round_float(&center.x, ((double)sprite_oriented_vertex->corners[3].x + sprite_oriented_vertex->corners[2].x) * 0.5);
        bh_round_float(&center.y, ((double)sprite_oriented_vertex->corners[3].y + sprite_oriented_vertex->corners[2].y) * 0.5);
        bh_round_float(&center.z, ((double)sprite_oriented_vertex->corners[3].z + sprite_oriented_vertex->corners[2].z) * 0.5);
        bh_round_float(&u.x, (double)sprite_oriented_vertex->corners[2].x - center.x);
        bh_round_float(&u.y, (double)sprite_oriented_vertex->corners[2].y - center.y);
        bh_round_float(&u.z, (double)sprite_oriented_vertex->corners[2].z - center.z);
        bh_round_float(&v.x, (double)other_sprite_oriented_vertex->corners[1].x - center.x);
        bh_round_float(&v.y, (double)other_sprite_oriented_vertex->corners[1].y - center.y);
        bh_round_float(&v.z, (double)other_sprite_oriented_vertex->corners[1].z - center.z);
        den = sqrt((double)u.x * u.x + (double)u.y * u.y) * sqrt((double)v.x * v.x + (double)v.y * v.y);
        dot = (((double)u.z * v.z + (double)u.y * v.y) + (double)u.x * v.x) / den;
        /* 00456238 FCOM /0045626e tests C0 alone: unordered 0/0 also takes
           the float clamp. Keep the actual masked division/sticky exception. */
        if (!(dot >= -0.8))
            dot = (float)-0.8;
        scale = 1.0 / (dot + 1.0);
        bh_round_float(&sum.x, (double)u.x + v.x);
        bh_round_float(&sum.y, (double)u.y + v.y);
        bh_round_float(&sum.z, (double)u.z + v.z);
        bh_round_float(&offset_x, (double)sum.x * scale);
        bh_round_float(&offset_y, (double)sum.y * scale);
        offset_z = (double)sum.z * scale;
        bh_round_float(&sprite_oriented_vertex->corners[2].x, (double)center.x + offset_x);
        bh_round_float(&sprite_oriented_vertex->corners[2].y, (double)center.y + offset_y);
        bh_round_float(&sprite_oriented_vertex->corners[2].z, (double)center.z + offset_z);
        other_sprite_oriented_vertex->corners[1] = sprite_oriented_vertex->corners[2];
        bh_round_float(&sprite_oriented_vertex->corners[3].x, (double)center.x - offset_x);
        bh_round_float(&sprite_oriented_vertex->corners[3].y, (double)center.y - offset_y);
        bh_round_float(&sprite_oriented_vertex->corners[3].z, (double)center.z - offset_z);
        other_sprite_oriented_vertex->corners[0] = sprite_oriented_vertex->corners[3];
    }
    for (sprite_oriented_vertex = head; sprite_oriented_vertex && sprite_oriented_vertex->next; sprite_oriented_vertex = sprite_oriented_vertex->next)
        if (!emit_segment(context, sprite_oriented_vertex, sprite_oriented_vertex->next))
            return 0;
    ++context->paths_emitted;
    return 1;
}
int bh_sprite_oriented_draw(BHSpriteOrientedContext *context)
{
    BHSpriteOrientedVertex *sprite_oriented_vertex, *next, *first, *input_sprite_oriented_vertex;
    unsigned long original, steps = 0, back_steps;
    if (!context || !context->path || !context->capacity || context->used > context->capacity || !context->clip || !context->emit || !_finite(context->near_z) ||
        context->near_z <= 0 || !_finite(context->projection_scale))
        return context ? fail(context, BH_SPRITE_ORIENTED_ARGUMENT) : 0;
    context->error = BH_SPRITE_ORIENTED_OK;
    context->paths_emitted = context->polygons_emitted = 0;
    if (!context->used)
        return 1;
    original = context->used;
    sprite_oriented_vertex = context->path;
    while (sprite_oriented_vertex)
    {
        if (!in_pool(context, sprite_oriented_vertex) || ++steps > original + context->capacity)
            return fail(context, BH_SPRITE_ORIENTED_TOPOLOGY);
        next = sprite_oriented_vertex->next;
        if (!next)
            break;
        if (!in_pool(context, next) || !_finite(sprite_oriented_vertex->point.z) || !_finite(next->point.z))
            return fail(context, BH_SPRITE_ORIENTED_TOPOLOGY);
        if (sprite_oriented_vertex->point.z < context->near_z && next->point.z >= context->near_z)
        {
            input_sprite_oriented_vertex = intersection(context, sprite_oriented_vertex, next);
            if (!input_sprite_oriented_vertex)
                return fail(context, BH_SPRITE_ORIENTED_CAPACITY);
            input_sprite_oriented_vertex->next = next;
            next->previous = input_sprite_oriented_vertex;
            sprite_oriented_vertex = next;
        }
        else if (sprite_oriented_vertex->point.z >= context->near_z && next->point.z < context->near_z)
        {
            input_sprite_oriented_vertex = intersection(context, sprite_oriented_vertex, next);
            if (!input_sprite_oriented_vertex)
                return fail(context, BH_SPRITE_ORIENTED_CAPACITY);
            input_sprite_oriented_vertex->previous = sprite_oriented_vertex;
            sprite_oriented_vertex->next = input_sprite_oriented_vertex;
            next->previous = NULL;
            first = sprite_oriented_vertex;
            back_steps = 0;
            while (first->previous)
            {
                if (!in_pool(context, first->previous) || ++back_steps > context->used)
                    return fail(context, BH_SPRITE_ORIENTED_TOPOLOGY);
                first = first->previous;
            }
            if (!bh_sprite_oriented_ribbon(context, first))
                return 0;
            context->used = original;
            sprite_oriented_vertex = next;
        }
        else
            sprite_oriented_vertex = next;
    }
    if (sprite_oriented_vertex && sprite_oriented_vertex->previous && sprite_oriented_vertex->point.z >= context->near_z)
    {
        first = sprite_oriented_vertex;
        back_steps = 0;
        while (first->previous)
        {
            if (!in_pool(context, first->previous) || ++back_steps > context->used)
                return fail(context, BH_SPRITE_ORIENTED_TOPOLOGY);
            first = first->previous;
        }
        if (!bh_sprite_oriented_ribbon(context, first))
            return 0;
    }
    context->used = 0;
    return 1;
}
