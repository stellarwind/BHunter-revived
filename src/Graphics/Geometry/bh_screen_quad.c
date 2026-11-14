#include "bh_screen_quad.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <float.h>
#include <string.h>
int bh_screen_quad_draw(BHScreenQuadContext *context, BHTextureImage *image, float x, float y, float width, float height,
                        float angle, unsigned char flags, float modulation)
{
    BHClipVertex v[4], *ring, *vertex;
    double w, h, r, cs, sn, hs, hc, conversion;
    float hw, hh, radius, cw, sw, negative_hs, stored_hc, bx, by, depth;
    float u0, u1, v0, v1;
    unsigned long i, n, color;
    long shade;
    if (!context || !context->view || !context->clip || !context->emit || !image || !_finite(x) || !_finite(y) || !_finite(width) ||
        !_finite(height) || !_finite(angle) || !_finite(modulation) || !_finite(context->view->near_z))
        return 0;
    context->emitted = context->culled = 0;
    w = (double)width * .5;
    h = (double)height * .5;
    bh_round_float(&hw, w);
    bh_round_float(&hh, h);
    r = sqrt(w * hw + h * hh);
    bh_round_float(&radius, r);
    if (!(context->view->left < r + x && x - radius < context->view->right && context->view->top < radius + y &&
          y - radius < context->view->bottom))
    {
        context->culled = 1;
        return 1;
    }
    memset(v, 0, sizeof(v));
    u0 = (flags & 1) ? 1.0f : 0;
    u1 = 1 - u0;
    v0 = (flags & 2) ? 1.0f : 0;
    v1 = 1 - v0;
    v[0].u = u0;
    v[0].v = v0;
    v[2].u = u1;
    v[2].v = v1;
    v[1].u = (flags & 4) ? u1 : u0;
    v[1].v = (flags & 4) ? v0 : v1;
    v[3].u = (flags & 4) ? u0 : u1;
    v[3].v = (flags & 4) ? v1 : v0;
    if (angle == 0)
    {
        bh_round_float(&v[0].sx, (double)x - hw);
        bh_round_float(&v[0].sy, (double)y - hh);
        v[1].sx = v[0].sx;
        bh_round_float(&v[1].sy, (double)y + hh);
        bh_round_float(&v[2].sx, (double)x + hw);
        v[2].sy = v[1].sy;
        v[3].sx = v[2].sx;
        v[3].sy = v[0].sy;
    }
    else
    {
        cs = cos((double)angle);
        sn = sin((double)angle);
        bh_round_float(&cw, (double)hw * cs);
        bh_round_float(&sw, (double)hw * sn);
        hs = -(double)hh * sn;
        hc = (double)hh * cs;
        bh_round_float(&negative_hs, hs);
        bh_round_float(&stored_hc, hc);
        bh_round_float(&bx, (double)x - cw);
        bh_round_float(&by, (double)y - sw);
        bh_round_float(&v[0].sx, ((double)x - cw) - negative_hs);
        bh_round_float(&v[0].sy, ((double)y - sw) - stored_hc);
        bh_round_float(&v[1].sx, (double)negative_hs + bx);
        bh_round_float(&v[1].sy, (double)stored_hc + by);
        bh_round_float(&v[2].sx, (hs + cw) + x);
        bh_round_float(&v[2].sy, (hc + sw) + y);
        bh_round_float(&v[3].sx, ((double)cw + x) - negative_hs);
        bh_round_float(&v[3].sy, ((double)sw + y) - stored_hc);
    }
    for (i = 0; i < 4; ++i)
    {
        v[i].next = v + (i + 1) % 4;
        v[i].previous = v + (i + 3) % 4;
    }
    ring = bh_screen_clip_polygon(context->clip, v, context->view->near_z);
    if (!ring)
    {
        if (context->clip->error != BH_CLIP_OK)
            return 0;
        context->culled = 1;
        return 1;
    }
    conversion = (double)modulation * -255.0;
    if (!_finite(conversion) || conversion < -2147483648.0 || conversion > 2147483647.0)
        return 0;
    shade = (long)conversion;
    color = context->base_color - (unsigned long)shade * 0x10101UL;
    bh_round_float(&depth, (double)context->view->near_z - (-.01));
    vertex = ring;
    n = 0;
    do
    {
        if (++n > 12)
            return 0;
        vertex->point.z = depth;
        vertex = vertex->next;
    } while (vertex != ring);
    if (!context->emit(ring, image, color, context->emit_user))
        return 0;
    context->emitted = 1;
    return 1;
}
int bh_screen_quad_queue(BHDrawQueue *queue, unsigned long bucket, unsigned long image, float x, float y, float width,
                         float height, float angle, unsigned long flags, float modulation)
{
    static const unsigned long headers[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
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
    header = bucket < 9 ? headers[bucket] : 0;
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
    memcpy(command->words, &x, 4);
    memcpy(command->words + 1, &y, 4);
    memcpy(command->words + 2, &width, 4);
    memcpy(command->words + 3, &height, 4);
    memcpy(command->words + 4, &angle, 4);
    command->kind = BH_DRAW_SCREEN_QUAD;
    command->words[6] = flags;
    memcpy(command->words + 5, &modulation, 4);
    queue->error = BH_DRAW_OK;
    return 1;
}
int bh_screen_quad_execute(BHScreenQuadSubmit *screen_quad_submit, const BHDrawCommand *command, unsigned long *next)
{
    float f[6];
    BHTextureImage *image;
    int ok;
    if (!screen_quad_submit || !screen_quad_submit->draw || !screen_quad_submit->lookup || !command || !next || command->kind != BH_DRAW_SCREEN_QUAD)
        return 0;
    image = screen_quad_submit->lookup(command->resource, screen_quad_submit->lookup_user);
    memcpy(f, command->words, sizeof(f));
    ok = image
             ? bh_screen_quad_draw(screen_quad_submit->draw, image, f[0], f[1], f[2], f[3], f[4], (unsigned char)command->words[6], f[5])
             : 0;
    *next = command->next;
    return ok;
}
