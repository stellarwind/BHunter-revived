#include "../bh_renderer.h"
#include "bh_texture_draw.h"
#include <string.h>
static int fail(BHTextureContext *texture_context, BHTextureError error)
{
    if (texture_context)
        texture_context->error = error;
    return 0;
}
static int native(BHTextureContext *texture_context, HRESULT h)
{
    texture_context->last_hresult = h;
    if (FAILED(h))
    {
        texture_context->error = BH_TEXTURE_BACKEND;
        return 0;
    }
    return 1;
}
static int valid(const BHTextureImage *image)
{
    return image && image->runs && image->vertices && image->vertex_capacity >= 3 &&
           image->vertex_capacity <= 0x1000000UL && image->vertex_count <= image->vertex_capacity &&
           image->run_count <= image->vertex_count / 3 && image->run_serial <= image->vertex_count;
}
D3DTLVERTEX *bh_texture_draw_vertex(BHTextureContext *texture_context, BHTextureImage *image)
{
    unsigned long size, capacity;
    unsigned long *runs;
    D3DTLVERTEX *vertices, *out;
    if (!texture_context || !texture_context->allocator.allocate || !valid(image))
    {
        fail(texture_context, BH_TEXTURE_ARGUMENT);
        return NULL;
    }
    if (image->vertex_count == image->vertex_capacity)
    {
        size = (image->vertex_capacity * 5) / 12;
        capacity = size * 3;
        if (capacity <= image->vertex_capacity || capacity > 0x1000000UL)
        {
            fail(texture_context, BH_TEXTURE_MEMORY);
            return NULL;
        }
        runs = (unsigned long *)texture_context->allocator.allocate(size * 4, texture_context->allocator.user);
        vertices = (D3DTLVERTEX *)texture_context->allocator.allocate(size * 96, texture_context->allocator.user);
        if (!runs || !vertices)
        {
            if (runs)
                texture_context->allocator.release(runs, texture_context->allocator.user);
            if (vertices)
                texture_context->allocator.release(vertices, texture_context->allocator.user);
            fail(texture_context, BH_TEXTURE_MEMORY);
            return NULL;
        }
        memcpy(runs, image->runs, image->run_count * 4);
        memcpy(vertices, image->vertices, image->vertex_count * 32);
        texture_context->allocator.release(image->runs, texture_context->allocator.user);
        texture_context->allocator.release(image->vertices, texture_context->allocator.user);
        image->runs = runs;
        image->vertices = vertices;
        image->vertex_capacity = capacity;
    }
    out = image->vertices + image->vertex_count;
    ++image->vertex_count;
    ++image->run_serial;
    return out;
}
int bh_texture_draw_flush(BHTextureContext *texture_context, BHTextureImage *image, unsigned long *runs_drawn)
{
    D3DTEXTUREHANDLE handle;
    unsigned long i, at = 0;
    int ok = 1;
    if (!texture_context || !image)
        return fail(texture_context, BH_TEXTURE_ARGUMENT);
    if (texture_context->software || !image->run_count)
        return 1;
    if (!texture_context->device || !valid(image))
        return fail(texture_context, BH_TEXTURE_TOPOLOGY);
    /* Source safety guard only: the original trusts its completed run table. */
    for (i = 0; i < image->run_count; ++i)
    {
        if (image->runs[i] < 3 || image->runs[i] > image->vertex_count - at)
            return fail(texture_context, BH_TEXTURE_TOPOLOGY);
        at += image->runs[i];
    }
    handle = bh_texture_resident(texture_context, image);
    if (texture_context->error != BH_TEXTURE_OK)
        ok = 0;
    if (!native(texture_context, bh_renderer_set_state(texture_context->device, D3DRENDERSTATE_TEXTUREHANDLE, handle)))
        ok = 0;
    if (runs_drawn)
        *runs_drawn += image->run_count;
    at = 0;
    for (i = 0; i < image->run_count; ++i)
    {
        if (!native(texture_context, bh_renderer_draw_fan(texture_context->device, image->vertices + at, image->runs[i])))
            ok = 0;
        at += image->runs[i];
    }
    image->run_serial = image->vertex_count = image->run_count = 0;
    return ok;
}
int bh_texture_draw_polygon(BHTextureContext *texture_context, BHTextureImage *image, const D3DTLVERTEX *vertices,
                            unsigned long count)
{
    unsigned long i, old_count, old_serial;
    unsigned long *runs;
    D3DTLVERTEX *arena, *out;
    if (!texture_context || !texture_context->allocator.allocate || !image || !vertices || count < 3 || count > 0x1000000UL)
        return fail(texture_context, BH_TEXTURE_ARGUMENT);
    if (!image->vertex_capacity && !image->runs && !image->vertices && !image->vertex_count && !image->run_count &&
        !image->run_serial)
    {
        runs = (unsigned long *)texture_context->allocator.allocate(0x80, texture_context->allocator.user);
        arena = (D3DTLVERTEX *)texture_context->allocator.allocate(0xc00, texture_context->allocator.user);
        if (!runs || !arena)
        {
            if (runs)
                texture_context->allocator.release(runs, texture_context->allocator.user);
            if (arena)
                texture_context->allocator.release(arena, texture_context->allocator.user);
            return fail(texture_context, BH_TEXTURE_MEMORY);
        }
        image->runs = runs;
        image->vertices = arena;
        image->vertex_capacity = 96;
    }
    if (!valid(image) || image->run_serial || count > 0x1000000UL - image->vertex_count)
        return fail(texture_context, BH_TEXTURE_TOPOLOGY);
    old_count = image->vertex_count;
    old_serial = image->run_serial;
    for (i = 0; i < count; ++i)
    {
        out = bh_texture_draw_vertex(texture_context, image);
        if (!out)
        {
            image->vertex_count = old_count;
            image->run_serial = old_serial;
            return 0;
        }
        *out = vertices[i];
    }
    image->runs[image->run_count++] = image->run_serial;
    image->run_serial = 0;
    return 1;
}
