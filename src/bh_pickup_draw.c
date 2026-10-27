#include "bh_pickup_draw.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

typedef char pickup_position_offset[offsetof(BHEntity, position) == 0x0c ? 1 : -1];
typedef char pickup_phase_offset[offsetof(BHEntity, payload) + 0x18 == 0xfc ? 1 : -1];

static float payload_float(const BHEntity *entity, unsigned long offset)
{
    float out;
    memcpy(&out, entity->payload + offset, 4);
    return out;
}
static long payload_long(const BHEntity *entity, unsigned long offset)
{
    long out;
    memcpy(&out, entity->payload + offset, 4);
    return out;
}
static unsigned long payload_word(const BHEntity *entity, unsigned long offset)
{
    unsigned long out;
    memcpy(&out, entity->payload + offset, 4);
    return out;
}
static void put_float(unsigned long *out, float value)
{
    memcpy(out, &value, 4);
}
static float get_float(const unsigned long *in)
{
    float value;
    memcpy(&value, in, 4);
    return value;
}

/* 004558d0. The input pointers model its in-place projected world point. */
int __cdecl bh_pickup_project(const BHPickupDrawContext *c, float *x, float *y, float *z, float *scale)
{
    BHVec3 world, projected;
    if (!c || !c->camera || !c->view || !x || !y || !z || !scale)
        return 0;
    world.x = *x;
    world.y = *y;
    world.z = *z;
    bh_mat34_transform_point(&c->camera->projected, &world, &projected);
    if (projected.z < c->view->near_z)
        return 0;
    *x = projected.x / projected.z;
    *y = projected.y / projected.z;
    *z = projected.z;
    *scale = c->camera->focal / projected.z;
    return 1;
}

/* 00478f60's inlined bucket append. Original command field offsets are
   +04 resource,+08 x..angle,+1c alpha,+20..+2c UV,+30 depth,+34 flags. */
int __cdecl bh_pickup_rect_queue(BHDrawQueue *q, unsigned long bucket, unsigned long image, float x, float y,
                                 float width, float height, float angle, unsigned long flags, float alpha, float u,
                                 float u_extent, float v, float v_extent, float depth)
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
    command->kind = BH_DRAW_RECT;
    command->resource = image;
    put_float(command->words + 0, x);
    put_float(command->words + 1, y);
    put_float(command->words + 2, width);
    put_float(command->words + 3, height);
    put_float(command->words + 4, angle);
    put_float(command->words + 5, alpha);
    put_float(command->words + 6, u);
    put_float(command->words + 7, u_extent);
    put_float(command->words + 8, v);
    put_float(command->words + 9, v_extent);
    put_float(command->words + 10, depth);
    command->words[11] = flags;
    q->error = BH_DRAW_OK;
    return 1;
}

static void vertex(BHClipVertex *p, float x, float y, float u, float v)
{
    p->sx = x;
    p->sy = y;
    p->u = u;
    p->v = v;
}

int __cdecl bh_pickup_rect_draw(BHPickupDrawContext *c, BHTextureImage *image, float x, float y, float width,
                                float height, float angle, unsigned long flags, float alpha, float u, float u_extent,
                                float v, float v_extent, float depth)
{
    BHClipVertex quad[4], *ring, *p;
    float half_width, half_height, radius;
    float u0, u1, v0, v1, cs, sn, xw, yw, wx, hy;
    double conversion;
    long shade;
    unsigned long i, n, color;
    if (!c || !image || !c->view || !c->clip || !c->emit || !_finite(x) || !_finite(y) || !_finite(width) ||
        !_finite(height) || !_finite(angle) || !_finite(alpha) || !_finite(u) || !_finite(u_extent) || !_finite(v) ||
        !_finite(v_extent) || !_finite(depth))
        return 0;
    c->rectangles++;
    half_width = width * .5f;
    half_height = height * .5f;
    bh_round_float(&radius, sqrt((double)half_width * half_width + (double)half_height * half_height));
    if (!(c->view->left < radius + x && x - radius < c->view->right && c->view->top < radius + y &&
          y - radius < c->view->bottom))
    {
        c->culled++;
        return 1;
    }
    u0 = (flags & 1) ? 1.0f - u : u;
    u1 = (flags & 1) ? 1.0f - (u + u_extent) : u + u_extent;
    v0 = (flags & 2) ? 1.0f - v : v;
    v1 = (flags & 2) ? 1.0f - (v + v_extent) : v + v_extent;
    memset(quad, 0, sizeof(quad));
    vertex(quad + 0, 0, 0, u0, v0);
    if (flags & 4)
    {
        vertex(quad + 1, 0, 0, u1, v0);
        vertex(quad + 2, 0, 0, u1, v1);
        vertex(quad + 3, 0, 0, u0, v1);
    }
    else
    {
        vertex(quad + 1, 0, 0, u0, v1);
        vertex(quad + 2, 0, 0, u1, v1);
        vertex(quad + 3, 0, 0, u1, v0);
    }
    if (angle == 0.0f)
    {
        quad[0].sx = x - half_width;
        quad[0].sy = y - half_height;
        quad[1].sx = quad[0].sx;
        quad[1].sy = y + half_height;
        quad[2].sx = x + half_width;
        quad[2].sy = quad[1].sy;
        quad[3].sx = quad[2].sx;
        quad[3].sy = quad[0].sy;
    }
    else
    {
        bh_round_float(&cs, cos((double)angle));
        bh_round_float(&sn, sin((double)angle));
        xw = half_width * cs;
        hy = half_height * cs;
        wx = half_width * sn;
        yw = -half_height * sn;
        quad[0].sx = (x - xw) - yw;
        quad[0].sy = (y - wx) - hy;
        quad[1].sx = (x - xw) + yw;
        quad[1].sy = (y - wx) + hy;
        quad[2].sx = (x + xw) + yw;
        quad[2].sy = (y + wx) + hy;
        quad[3].sx = (x + xw) - yw;
        quad[3].sy = (y + wx) - hy;
    }
    for (i = 0; i < 4; ++i)
    {
        quad[i].next = quad + (i + 1) % 4;
        quad[i].previous = quad + (i + 3) % 4;
    }
    ring = bh_screen_clip_polygon(c->clip, quad, c->view->near_z);
    if (!ring)
    {
        if (c->clip->error != BH_CLIP_OK)
            return 0;
        c->culled++;
        return 1;
    }
    conversion = (double)alpha * -255.0;
    if (!_finite(conversion) || conversion < -2147483648.0 || conversion > 2147483647.0)
        return 0;
    shade = (long)conversion;
    color = 0x80000000UL - (unsigned long)shade * 0x10101UL;
    p = ring;
    n = 0;
    do
    {
        if (++n > 12)
            return 0;
        p->point.z = depth;
        p = p->next;
    } while (p != ring);
    return c->emit(ring, image, color, c->emit_user);
}

int __cdecl bh_pickup_rect_execute(BHPickupDrawContext *c, BHDrawImageLookup lookup, void *lookup_user,
                                   const BHDrawCommand *command, unsigned long *next)
{
    BHTextureImage *image;
    float a[11];
    unsigned long i, flags;
    int result;
    if (!c || !lookup || !command || !next || command->kind != BH_DRAW_RECT)
        return 0;
    for (i = 0; i < 11; ++i)
        a[i] = get_float(command->words + i);
    flags = command->words[11] & 255UL;
    image = lookup(command->resource, lookup_user);
    result =
        image ? bh_pickup_rect_draw(c, image, a[0], a[1], a[2], a[3], a[4], flags, a[5], a[6], a[7], a[8], a[9], a[10])
              : 0;
    *next = command->next;
    return result;
}

static int rect(BHPickupDrawContext *c, float x, float y, float w, float h, float alpha, float u, float u_extent,
                float v, float v_extent, float z)
{
    return bh_pickup_rect_queue(c->queue, 8, c->atlas_image, x, y, w, h, 0, 0, alpha, u, u_extent, v, v_extent, z);
}
static void digits(long value, long out[8], int *first)
{
    long divisor = 10000000, remainder = value;
    int i;
    *first = 7;
    for (i = 0; i < 8; ++i)
    {
        out[i] = remainder / divisor;
        remainder -= divisor * out[i];
        if (i < *first && out[i] != 0)
            *first = i;
        divisor /= 10;
    }
}
static float digit_u(long digit)
{
    return (float)((.5 - (double)digit * (-8.0)) * .0078125);
}

int __cdecl bh_pickup_label(BHPickupDrawContext *c, const BHEntity *entity, long total, long count)
{
    float x, y, z, s, scale, alpha, step, w, u, v, du;
    long ds[8];
    int first, i, result = 1, r;
    long kind;
    if (!c || !c->queue || !c->camera || !c->view || !entity || !c->display_depth)
        return -1;
    kind = payload_long(entity, 0x1c);
    x = entity->position.x;
    y = entity->position.y - 2.5f;
    z = entity->position.z;
    if (bh_pickup_project(c, &x, &y, &z, &s))
    {
        z -= 2.0f;
        scale = (float)c->display_depth * s * (float).00009765625;
        if (z < c->view->near_z - .01f)
            z = c->view->near_z - .01f;
        if (z * .01f >= .5f)
            return result;
        {
            alpha = (float)(.5 - (double)z * .01);
            if (kind == 21)
            {
                r = rect(c, x, y, scale * 38.0f, scale * 12.0f, alpha, .70703125f, .2890625f, .72265625f, .0859375f, z);
                if (r < 0)
                    return -1;
                result &= r;
            }
            else
            {
                digits(total, ds, &first);
                x -= (float)(8.0 - (double)(8 - first) * (-4.0)) * scale;
                r = rect(c, x - scale * -5.0f, y, scale * 10.0f, scale * 12.0f, alpha, .08203125f, .0703125f,
                         .72265625f, .0859375f, z);
                if (r < 0)
                    return -1;
                result &= r;
                x -= scale * -16.0f;
                step = scale * -8.0f;
                w = scale * 8.0f;
                for (i = first; i < 8; ++i)
                {
                    r = rect(c, x, y, w, scale * 12.0f, alpha, digit_u(ds[i]), .0546875f, .09765625f, .0859375f, z);
                    if (r < 0)
                        return -1;
                    result &= r;
                    x -= step;
                }
            }
        }
    }
    x = entity->position.x;
    y = entity->position.y + 2.5f;
    z = entity->position.z;
    if (!bh_pickup_project(c, &x, &y, &z, &s))
        return result;
    z -= 2.0f;
    scale = (float)c->display_depth * s * (float).00009765625;
    if (z < c->view->near_z - .01f)
        z = c->view->near_z - .01f;
    if (!(z * .01f < .5f))
        return result;
    alpha = (float)(.5 - (double)z * .01);
    if (kind == 21)
    {
        r = rect(c, x, y, scale * 23.0f, scale * 12.0f, alpha, .52734375f, .171875f, .72265625f, .0859375f, z);
        if (r < 0)
            return -1;
        return result & r;
    }
    digits(count, ds, &first);
    if (kind == 20)
        w = scale * 31.5f;
    else if (count == 1)
        w = scale * 23.0f;
    else
        w = scale * 18.5f;
    x -= ((float)(8 - first) * scale * 4.0f) + w;
    for (i = first; i < 8; ++i)
    {
        r = rect(c, x - scale * -4.0f, y, scale * 8.0f, scale * 12.0f, alpha, digit_u(ds[i]), .0546875f, .09765625f,
                 .0859375f, z);
        if (r < 0)
            return -1;
        result &= r;
        x -= scale * -8.0f;
    }
    if (kind == 20)
    {
        r = rect(c, x - scale * -5.0f, y, scale * 10.0f, scale * 12.0f, alpha, .00390625f, .0703125f, .72265625f,
                 .0859375f, z);
        if (r < 0)
            return -1;
        result &= r;
        x -= scale * -16.0f;
        w = scale * 47.0f;
        x -= scale * -23.5f;
        u = .16015625f;
        v = .359375f;
        du = .72265625f;
    }
    else if (count == 1)
    {
        x -= scale * -6.0f;
        w = scale * 31.0f;
        x -= scale * -15.5f;
        u = .69140625f;
        v = .234375f;
        du = .81640625f;
    }
    else
    {
        x -= scale * -6.0f;
        w = scale * 40.0f;
        x -= scale * -20.0f;
        u = .69140625f;
        v = .3046875f;
        du = .81640625f;
    }
    r = rect(c, x, y, w, scale * 12.0f, alpha, u, v, du, .0859375f, z);
    if (r < 0)
        return -1;
    return result & r;
}

int __cdecl bh_pickup_draw(BHPickupDrawContext *c, BHEntity *entity)
{
    BHVec3 angles;
    float phase, size, alpha;
    double oscillation;
    long count, total;
    unsigned long resource;
    int r0, r1, r2;
    if (!c || !c->queue || !entity)
        return -1;
    phase = payload_float(entity, 0x18);
    resource = payload_word(entity, 0x30);
    angles.x = phase + phase;
    angles.y = (float)(sin((double)phase * .87) * .3);
    angles.z = 0;
    memcpy(entity->opaque0 + 0x24, &angles, sizeof(angles));
    r0 = bh_draw_submit_resource(c->queue, 0, resource, &entity->position, &angles, 1.0f);
    oscillation = sin((double)phase * 5.17);
    size = (float)(oscillation - (-6.0));
    alpha = (float)(.5 - oscillation * (-.25));
    r1 = bh_draw_billboard(c->queue, 3, c->glow_image, &entity->position, size, size, 0, 0, alpha);
    count = payload_long(entity, 0x28);
    total = payload_long(entity, 0x2c);
    r2 = total ? bh_pickup_label(c, entity, (long)((unsigned long)total * (unsigned long)count), count) : 1;
    if (r0 < 0 || r1 < 0 || r2 < 0)
        return -1;
    return r0 && r1 && r2;
}

int __cdecl bh_pickup_render_bind(BHPickupContext *context, BHPickupRenderBinding *binding)
{
    if (!context || !binding || !binding->default_draw || !binding->special_draw)
        return 0;
    context->draw_user = binding;
    context->default_draw = bh_pickup_default_step;
    context->special_draw = bh_pickup_special_step;
    return 1;
}
void __cdecl bh_pickup_default_step(BHEntity *entity)
{
    BHPickupContext *owner = bh_pickup_context(entity);
    BHPickupRenderBinding *binding = owner ? (BHPickupRenderBinding *)owner->draw_user : NULL;
    if (binding && binding->default_draw)
        bh_pickup_draw(binding->default_draw, entity);
}
void __cdecl bh_pickup_special_step(BHEntity *entity)
{
    BHPickupContext *owner = bh_pickup_context(entity);
    BHPickupRenderBinding *binding = owner ? (BHPickupRenderBinding *)owner->draw_user : NULL;
    if (binding && binding->special_draw)
        bh_entity_resource_draw_special(binding->special_draw, entity);
}
