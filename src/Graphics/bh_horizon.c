#include "bh_renderer.h"
#include "bh_horizon.h"
#include "../Math/bh_round.h"
#include "Textures/bh_texture_draw.h"
#include <math.h>
#include <float.h>
#include <string.h>
static void error(BHHorizonContext *context, BHHorizonError e)
{
    if (context->error == BH_HORIZON_OK)
        context->error = e;
}
static int native(BHHorizonContext *context, HRESULT h)
{
    ++context->state->native_calls;
    context->state->last_hresult = h;
    if (FAILED(h))
    {
        ++context->state->failures;
        error(context, BH_HORIZON_BACKEND);
        return 0;
    }
    return 1;
}
static void state(BHHorizonContext *context, D3DRENDERSTATETYPE type, unsigned long value)
{
    native(context, bh_renderer_set_state(context->state->textures->device, type, value));
}
int bh_horizon_init(BHHorizonContext *context, const BHMainCamera *camera, BHAttributeClipContext *clip,
                    BHRenderState *render, BHHorizonColor *colors, BHHorizonEmit emit, void *user)
{
    if (!context || !camera || !clip || !render || !render->textures || !emit ||
        (render->textures->software && (!colors || colors->textures != render->textures)))
        return 0;
    memset(context, 0, sizeof(*context));
    context->camera = camera;
    context->clip = clip;
    context->state = render;
    context->colors = colors;
    context->emit = emit;
    context->user = user;
    context->color_mode = 1;
    return 1;
}
/* UV normally consumes retained world sums before their binary32 stores.
   First inner X and last inner Z instead reload the stored local coordinate. */
static void vertex(BHClipVertex *clip_vertex, const BHMat34 *projected, double x, double y, double z, float inverse_scale,
                   int reload_x, int reload_z, unsigned long alpha)
{
    BHVec3 world;
    bh_round_float(&world.x, x);
    bh_round_float(&world.y, y);
    bh_round_float(&world.z, z);
    bh_round_float(&clip_vertex->u, (reload_x ? (double)world.x : x) * inverse_scale);
    bh_round_float(&clip_vertex->v, (reload_z ? (double)world.z : z) * inverse_scale);
    bh_mat34_transform_point(projected, &world, &clip_vertex->point);
    clip_vertex->unknown28 = alpha;
    clip_vertex->projected = 0;
}
static void polygon(BHHorizonContext *context, BHTextureImage *image, BHClipVertex **v, unsigned long n, int gradient)
{
    BHClipVertex *ring;
    unsigned long i;
    for (i = 0; i < n; ++i)
    {
        v[i]->next = v[(i + 1) % n];
        v[i]->previous = v[(i + n - 1) % n];
    }
    ++context->attempted;
    ring = bh_attribute_clip_polygon(context->clip, v[0], 1);
    if (!ring)
    {
        if (context->clip->error != BH_CLIP_OK)
        {
            error(context, BH_HORIZON_CLIP);
        }
        return;
    }
    if (!context->emit(ring, image, gradient, context->rgb, context->user))
        error(context, BH_HORIZON_EMIT);
    else
        ++context->emitted;
}
int bh_horizon_draw(BHHorizonContext *context, BHTextureImage *image, float texture_scale, float height, float outer_radius,
                    float inner_radius, unsigned long color)
{
    BHTextureContext *texture_context;
    BHTextureImage *im, *slow, *fast;
    BHClipVertex v[9], *ring[4];
    BHVec3 inner[2], outer[2];
    float sine, cosine, negative_sine, ratio_stored, factor[2][3], inverse_scale;
    double ratio, sx, sz, x, z;
    unsigned long old_depth, i, j;
    long signed_color = (long)color;
    if (!context || !context->camera || !context->clip || !context->state || !(texture_context = context->state->textures) || !context->emit || !image ||
        (!texture_context->software && !texture_context->device) || (texture_context->software && (!context->colors || context->colors->textures != texture_context)) ||
        !_finite(texture_scale) || !texture_scale || !_finite(height) || !_finite(outer_radius) || !outer_radius ||
        !_finite(inner_radius))
        return 0;
    context->error = BH_HORIZON_OK;
    context->attempted = context->emitted = 0;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)&context->camera->projected)[i]))
        {
            error(context, BH_HORIZON_ARGUMENT);
            return 0;
        }
    if (!_finite(context->camera->position.x) || !_finite(context->camera->position.z) || !_finite(context->camera->angles.x))
    {
        error(context, BH_HORIZON_ARGUMENT);
        return 0;
    }
    if (!texture_context->software)
    {
        slow = fast = texture_context->images;
        while (fast && fast->next)
        {
            slow = slow->next;
            fast = fast->next->next;
            if (slow == fast)
            {
                error(context, BH_HORIZON_ARGUMENT);
                return 0;
            }
        }
    }
    old_depth = context->state->depth;
    bh_round_float(&sine, sin((double)context->camera->angles.x));
    bh_round_float(&cosine, cos((double)context->camera->angles.x));
    negative_sine = -sine;
    if (!bh_render_depth(context->state, 2))
        error(context, BH_HORIZON_BACKEND);
    context->rgb[0] = (float)((unsigned long)(signed_color / 65536L) & 255UL);
    context->rgb[1] = (float)((unsigned long)(signed_color / 256L) & 255UL);
    context->rgb[2] = (float)(color & 255UL);
    if (texture_context->software && !bh_horizon_color_prepare(context->colors, context->color_mode, color))
        error(context, BH_HORIZON_COLOR);
    ratio = (double)height / outer_radius;
    bh_round_float(&ratio_stored, ratio);
    bh_round_float(&inverse_scale, 1.0 / texture_scale);
    sx = (double)sine - cosine;
    sz = (double)cosine - negative_sine;
    bh_round_float(&factor[0][0], sx);
    factor[0][1] = ratio_stored;
    bh_round_float(&factor[0][2], sz);
    bh_round_float(&inner[0].x, sx * inner_radius);
    bh_round_float(&inner[0].y, ratio * inner_radius);
    bh_round_float(&inner[0].z, sz * inner_radius);
    sx = (double)cosine + sine;
    sz = (double)negative_sine + cosine;
    bh_round_float(&factor[1][0], sx);
    factor[1][1] = ratio_stored;
    bh_round_float(&factor[1][2], sz);
    bh_round_float(&inner[1].x, sx * inner_radius);
    bh_round_float(&inner[1].y, ratio * inner_radius);
    bh_round_float(&inner[1].z, sz * inner_radius);
    for (i = 0; i < 2; ++i)
    {
        bh_round_float(&outer[i].x, (double)factor[i][0] * outer_radius);
        bh_round_float(&outer[i].y, (double)factor[i][1] * outer_radius);
        bh_round_float(&outer[i].z, (double)factor[i][2] * outer_radius);
    }
    memset(v, 0, sizeof(v));
    x = context->camera->position.x;
    z = context->camera->position.z;
    vertex(v, &context->camera->projected, x, -height, z, inverse_scale, 0, 0, 0);
    for (i = 0; i < 4; ++i)
    {
        j = i & 1;
        vertex(v + 1 + i, &context->camera->projected, i < 2 ? x + inner[j].x : x - inner[j].x,
               i < 2 ? (double)-height + inner[j].y : (double)-height - inner[j].y,
               i < 2 ? z + inner[j].z : z - inner[j].z, inverse_scale, i == 0, i == 3, 0);
        vertex(v + 5 + i, &context->camera->projected, i < 2 ? x + outer[j].x : x - outer[j].x,
               i < 2 ? (double)-height + outer[j].y : (double)-height - outer[j].y,
               i < 2 ? z + outer[j].z : z - outer[j].z, inverse_scale, 0, 0, 0x3f800000UL);
    }
    if (!texture_context->software)
    {
        state(context, D3DRENDERSTATE_FOGENABLE, 1);
        state(context, D3DRENDERSTATE_FOGCOLOR, color);
    }
    for (i = 0; i < 4; ++i)
    {
        ring[0] = v + 1 + i;
        ring[1] = v + 1 + (i + 1) % 4;
        ring[2] = v;
        polygon(context, image, ring, 3, !texture_context->software);
    }
    for (i = 0; i < 4; ++i)
    {
        ring[0] = v + 5 + i;
        ring[1] = v + 5 + (i + 1) % 4;
        ring[2] = v + 1 + (i + 1) % 4;
        ring[3] = v + 1 + i;
        polygon(context, image, ring, 4, 1);
    }
    if (!texture_context->software)
    {
        texture_context->frame = (long)((unsigned long)texture_context->frame + 1);
        ++context->state->barriers;
        for (im = texture_context->images; im; im = im->next)
            if (!bh_texture_draw_flush(texture_context, im, NULL))
                error(context, BH_HORIZON_BACKEND);
        native(context, bh_renderer_record_draws(texture_context->device));
    }
    if (!bh_render_depth(context->state, old_depth))
        error(context, BH_HORIZON_BACKEND);
    if (!texture_context->software)
        state(context, D3DRENDERSTATE_FOGENABLE, 0);
    return context->error == BH_HORIZON_OK;
}
int bh_horizon_native_emit(BHClipVertex *ring, BHTextureImage *image, int gradient, const float rgb[3], void *user)
{
    BHHorizonNative *horizon_native = (BHHorizonNative *)user;
    BHTextureContext *texture_context;
    BHClipVertex *vertex;
    D3DTLVERTEX *vertices;
    unsigned long count = 0, i;
    float scalar;
    double fog;
    int ok;
    if (!horizon_native || !(texture_context = horizon_native->textures) || texture_context->software || !_finite(horizon_native->near_z) || horizon_native->near_z <= 0 || !texture_context->allocator.allocate ||
        !texture_context->allocator.release || !ring || !image || !gradient)
        return 0;
    vertex = ring;
    do
    {
        if (!vertex || !vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++count > 2048)
            return 0;
        memcpy(&scalar, &vertex->unknown28, 4);
        fog = (double)scalar * -255.0;
        if (!_finite(vertex->sx) || !_finite(vertex->sy) || !_finite(vertex->point.z) || vertex->point.z <= 0 || !_finite(vertex->u) ||
            !_finite(vertex->v) || !_finite(fog) || fog < -2147483648.0 || fog > 2147483647.0)
            return 0;
        vertex = vertex->next;
    } while (vertex != ring);
    if (count < 3)
        return 0;
    vertices = (D3DTLVERTEX *)texture_context->allocator.allocate(count * sizeof(*vertices), texture_context->allocator.user);
    if (!vertices)
        return 0;
    memset(vertices, 0, count * sizeof(*vertices));
    vertex = ring;
    for (i = 0; i < count; ++i)
    {
        vertices[i].sx = vertex->sx;
        vertices[i].sy = vertex->sy;
        bh_round_float(&vertices[i].sz, (double)horizon_native->near_z / vertex->point.z);
        bh_round_float(&vertices[i].rhw, 1.0 / vertex->point.z);
        memcpy(&scalar, &vertex->unknown28, 4);
        fog = (double)scalar * -255.0;
        vertices[i].color = 0xffffffUL;
        vertices[i].specular = 0xff000000UL + (unsigned long)(long)fog * 0x01000000UL;
        vertices[i].tu = vertex->u;
        vertices[i].tv = vertex->v;
        vertex = vertex->next;
    }
    ok = bh_texture_draw_polygon(texture_context, image, vertices, count);
    texture_context->allocator.release(vertices, texture_context->allocator.user);
    if (ok)
    {
        ++horizon_native->polygons;
        horizon_native->vertices += count;
    }
    return ok;
}
