#include "bh_native_primitive.h"
#include "Textures/bh_texture_draw.h"
#include "../Math/bh_round.h"
#include <string.h>
#include <float.h>
static int fail(BHTextureContext *texture_context, BHTextureError error)
{
    if (texture_context)
        texture_context->error = error;
    return 0;
}
int bh_native_primitive_emit(BHClipVertex *ring, BHTextureImage *image, unsigned long color, void *user)
{
    BHNativePrimitive *native = (BHNativePrimitive *)user;
    BHTextureContext *texture_context;
    BHClipVertex *vertex;
    D3DTLVERTEX *vertices;
    unsigned long count = 0, i;
    int ok;
    if (!native || !(texture_context = native->textures))
        return 0;
    if (texture_context->software)
        return fail(texture_context, BH_TEXTURE_BACKEND);
    if (!ring || !image || !texture_context->allocator.allocate || !texture_context->allocator.release || !_finite(native->near_z) ||
        native->near_z <= 0)
        return fail(texture_context, BH_TEXTURE_ARGUMENT);
    vertex = ring;
    do
    {
        if (!vertex || !vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++count > 2048)
            return fail(texture_context, BH_TEXTURE_TOPOLOGY);
        if (!_finite(vertex->sx) || !_finite(vertex->sy) || !_finite(vertex->point.z) || vertex->point.z <= 0 || !_finite(vertex->u) ||
            !_finite(vertex->v))
            return fail(texture_context, BH_TEXTURE_ARGUMENT);
        vertex = vertex->next;
    } while (vertex != ring);
    if (count < 3)
        return fail(texture_context, BH_TEXTURE_TOPOLOGY);
    vertices = (D3DTLVERTEX *)texture_context->allocator.allocate(count * sizeof(*vertices), texture_context->allocator.user);
    if (!vertices)
        return fail(texture_context, BH_TEXTURE_MEMORY);
    memset(vertices, 0, count * sizeof(*vertices));
    vertex = ring;
    for (i = 0; i < count; ++i)
    {
        vertices[i].sx = vertex->sx;
        vertices[i].sy = vertex->sy;
        bh_round_float(&vertices[i].sz, (double)native->near_z / vertex->point.z);
        bh_round_float(&vertices[i].rhw, 1.0 / vertex->point.z);
        vertices[i].color = color;
        vertices[i].tu = vertex->u;
        vertices[i].tv = vertex->v;
        vertex = vertex->next;
    }
    ok = bh_texture_draw_polygon(texture_context, image, vertices, count);
    texture_context->allocator.release(vertices, texture_context->allocator.user);
    if (ok)
    {
        ++native->polygons;
        native->vertices += count;
    }
    return ok;
}
