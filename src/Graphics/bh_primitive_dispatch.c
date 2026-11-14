#include "bh_renderer.h"
#include "bh_primitive_dispatch.h"
#include "Textures/bh_texture_draw.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <string.h>
#define ENTRY(P, N) bh_raster_##P##_##N,
static BHRasterKernel kernels[28] = {BH_RASTER_KERNELS(ENTRY, indexed) BH_RASTER_KERNELS(ENTRY, packed)};
#undef ENTRY
static int fail(BHPrimitiveDispatch *primitive_dispatch, BHPrimitiveError e)
{
    if (primitive_dispatch)
        primitive_dispatch->error = e;
    return 0;
}
static int leaf(BHClipVertex *ring, void *user)
{
    BHPrimitiveDispatch *primitive_dispatch = (BHPrimitiveDispatch *)user;
    return primitive_dispatch->selected(&primitive_dispatch->raster, ring);
}
int bh_primitive_dispatch_init(BHPrimitiveDispatch *primitive_dispatch, BHRenderState *state, const BHSoftwareRaster *frame, float left,
                               float right, float top, float bottom)
{
    if (!primitive_dispatch || !state || !state->textures || !frame || !_finite(frame->near_z) || frame->near_z <= 0)
        return 0;
    memset(primitive_dispatch, 0, sizeof(*primitive_dispatch));
    primitive_dispatch->state = state;
    primitive_dispatch->raster = *frame;
    primitive_dispatch->key_enabled = 1;
    primitive_dispatch->base_color = 0x80000000UL;
    primitive_dispatch->selected_index = 0xffffffffUL;
    return bh_depth_split_init(&primitive_dispatch->split, left, right, top, bottom, leaf, primitive_dispatch);
}
static unsigned long shade(unsigned long color)
{
    unsigned long r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
    if ((r >> 5) == (g >> 5) && (r >> 6) == (b >> 6))
        return (r + g + b) / 3;
    return 256 + (((r >> 5) * 8 + (g >> 5)) * 4 + (b >> 6));
}
static int select_kernel(BHPrimitiveDispatch *primitive_dispatch, BHTextureImage *image, unsigned long color)
{
    unsigned long r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
    unsigned long packed = primitive_dispatch->raster.tables->bytes_per_pixel == 1 ? 0 : 14, index, mode = primitive_dispatch->state->mode, bucket;
    int shaded = r + g + b <= 764 || primitive_dispatch->raster.attribute, masked = primitive_dispatch->key_enabled && image->format->kind;
    if (mode == 0)
    {
        if (shaded)
            primitive_dispatch->raster.shade_offset = shade(color) * 256;
        if (primitive_dispatch->state->depth <= 2)
        {
            index = packed + (shaded ? 6 : 0) + (masked ? 3 : 0) + primitive_dispatch->state->depth;
            primitive_dispatch->selected = kernels[index];
            primitive_dispatch->selected_index = index;
        }
    }
    else
    {
        if (mode <= 3)
            primitive_dispatch->raster.blend_plane = mode - 1;
        else if (mode == 4)
        {
            bucket = ((color >> 24) * 7) >> 8;
            if (!bucket)
                return 0;
            primitive_dispatch->raster.blend_plane = bucket + 2;
            if (bucket == 6)
            {
                primitive_dispatch->raster.shade_offset = shade(color) * 256;
                primitive_dispatch->selected_index = packed + 10;
                primitive_dispatch->selected = kernels[primitive_dispatch->selected_index];
                return 1;
            }
        }
        if (shaded)
            primitive_dispatch->raster.shade_offset = shade(color) * 256;
        primitive_dispatch->selected_index = packed + 12 + (shaded ? 1 : 0);
        primitive_dispatch->selected = kernels[primitive_dispatch->selected_index];
    }
    return primitive_dispatch->selected ? 1 : -1;
}
int bh_software_polygon(BHPrimitiveDispatch *primitive_dispatch, BHTextureImage *image, BHClipVertex *ring, unsigned long color)
{
    DDSURFACEDESC locked;
    HRESULT hr;
    int selected, ok = 1;
    if (!primitive_dispatch || !primitive_dispatch->state || !primitive_dispatch->state->textures || !image || !image->source || !image->format || !ring ||
        !primitive_dispatch->raster.tables || (primitive_dispatch->raster.tables->bytes_per_pixel != 1 && primitive_dispatch->raster.tables->bytes_per_pixel != 2))
        return fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
    primitive_dispatch->error = BH_PRIMITIVE_OK;
    memset(&locked, 0, sizeof(locked));
    locked.dwSize = sizeof(locked);
    ++primitive_dispatch->locks;
    primitive_dispatch->last_result = bh_renderer_surface_lock(image->source, &locked);
    if (primitive_dispatch->last_result != DD_OK)
        return fail(primitive_dispatch, BH_PRIMITIVE_BACKEND);
    if (!locked.lpSurface || locked.dwWidth != image->format->width || locked.dwHeight < locked.dwWidth ||
        locked.lPitch != (long)locked.dwWidth)
    {
        fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
        ok = 0;
    }
    else
    {
        primitive_dispatch->raster.texels = (const unsigned char *)locked.lpSurface;
        primitive_dispatch->raster.texture_width = image->format->width;
        primitive_dispatch->raster.image = image;
        selected = select_kernel(primitive_dispatch, image, color);
        if (selected < 0)
        {
            fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
            ok = 0;
        }
        else if (selected)
        {
            ok = primitive_dispatch->raster.attribute ? bh_depth_subdivide_scalar(&primitive_dispatch->split, ring) : bh_depth_subdivide(&primitive_dispatch->split, ring);
            if (!ok)
                fail(primitive_dispatch, BH_PRIMITIVE_RASTER);
        }
    }
    ++primitive_dispatch->unlocks;
    hr = bh_renderer_surface_unlock(image->source);
    primitive_dispatch->last_result = hr;
    if (hr != DD_OK)
    {
        fail(primitive_dispatch, BH_PRIMITIVE_BACKEND);
        ok = 0;
    }
    /* Lock storage is borrowed only until Unlock; never retain a stale pixel
       pointer as an apparently live source texture. */
    primitive_dispatch->raster.texels = NULL;
    if (ok)
        ++primitive_dispatch->polygons;
    return ok;
}
static int ring_info(BHPrimitiveDispatch *c, BHClipVertex *ring, unsigned long *count, double *sum, int attribute)
{
    BHClipVertex *vertex = ring;
    float scalar;
    *count = 0;
    *sum = 0;
    if (!vertex)
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    do
    {
        if (!vertex || !vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++*count > 1024)
            return fail(c, BH_PRIMITIVE_ARGUMENT);
        if (!_finite(vertex->point.z) || vertex->point.z <= 0 || !_finite(vertex->sx) || !_finite(vertex->sy) || !_finite(vertex->u) ||
            !_finite(vertex->v))
            return fail(c, BH_PRIMITIVE_ARGUMENT);
        if (attribute)
        {
            memcpy(&scalar, &vertex->unknown28, 4);
            if (!_finite(scalar))
                return fail(c, BH_PRIMITIVE_ARGUMENT);
            *sum += scalar;
        }
        vertex = vertex->next;
    } while (vertex != ring);
    return *count >= 3;
}
static int native_emit(BHPrimitiveDispatch *primitive_dispatch, BHTextureImage *image, BHClipVertex *ring, unsigned long color, int kind)
{
    BHClipVertex *vertex = ring;
    BHTextureContext *textures = primitive_dispatch->state->textures;
    D3DTLVERTEX *out;
    unsigned long n;
    double sum, component;
    long converted;
    float scalar;
    if (!image || !ring_info(primitive_dispatch, ring, &n, &sum, kind != 0))
        return 0;
    do
    {
        component = 0;
        if (kind)
        {
            memcpy(&scalar, &vertex->unknown28, 4);
            component = (double)scalar * -255.0;
        }
        if (kind && (component < -2147483648.0 || component > 2147483647.0))
            return fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
        converted = kind ? (long)component : 0;
        out = bh_texture_draw_vertex(textures, image);
        if (!out)
            return fail(primitive_dispatch, BH_PRIMITIVE_QUEUE);
        out->sx = vertex->sx;
        out->sy = vertex->sy;
        bh_round_float(&out->sz, (double)primitive_dispatch->raster.near_z / vertex->point.z);
        bh_round_float(&out->rhw, 1.0 / vertex->point.z);
        out->tu = vertex->u;
        out->tv = vertex->v;
        out->color = kind == 2 ? 0xffffffUL : kind == 1 ? primitive_dispatch->base_color - (unsigned long)converted * 0x10101UL : color;
        out->specular = kind == 2 ? 0xff000000UL + ((unsigned long)converted << 24) : 0;
        ++primitive_dispatch->vertices;
        vertex = vertex->next;
    } while (vertex != ring);
    image->runs[image->run_count++] = image->run_serial;
    image->run_serial = 0;
    ++primitive_dispatch->polygons;
    return 1;
}
int bh_primitive_emit(BHPrimitiveDispatch *primitive_dispatch, BHTextureImage *image, BHClipVertex *ring, unsigned long color)
{
    if (!primitive_dispatch || !primitive_dispatch->state || !primitive_dispatch->state->textures)
        return fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
    primitive_dispatch->error = BH_PRIMITIVE_OK;
    if (primitive_dispatch->state->textures->software)
        return bh_software_polygon(primitive_dispatch, image, ring, color);
    return native_emit(primitive_dispatch, image, ring, color, 0);
}
int bh_primitive_emit_scalar(BHPrimitiveDispatch *primitive_dispatch, BHTextureImage *image, BHClipVertex *ring)
{
    unsigned long n;
    double sum, scaled;
    long value;
    if (!primitive_dispatch || !primitive_dispatch->state || !primitive_dispatch->state->textures)
        return fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
    primitive_dispatch->error = BH_PRIMITIVE_OK;
    if (!primitive_dispatch->state->textures->software)
        return native_emit(primitive_dispatch, image, ring, 0, 1);
    if (!ring_info(primitive_dispatch, ring, &n, &sum, 1))
        return 0;
    scaled = (sum / n) * 255.0;
    if (scaled < -2147483648.0 || scaled > 2147483647.0)
        return fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
    value = (long)scaled;
    return bh_software_polygon(primitive_dispatch, image, ring, primitive_dispatch->base_color + (unsigned long)value * 0x10101UL);
}
int bh_primitive_emit_gradient(BHPrimitiveDispatch *primitive_dispatch, BHTextureImage *image, BHClipVertex *ring, unsigned long color)
{
    int ok;
    if (!primitive_dispatch || !primitive_dispatch->state || !primitive_dispatch->state->textures)
        return fail(primitive_dispatch, BH_PRIMITIVE_ARGUMENT);
    primitive_dispatch->error = BH_PRIMITIVE_OK;
    if (!primitive_dispatch->state->textures->software)
        return native_emit(primitive_dispatch, image, ring, color, 2);
    primitive_dispatch->raster.attribute = 1;
    ok = bh_software_polygon(primitive_dispatch, image, ring, color);
    primitive_dispatch->raster.attribute = 0;
    return ok;
}
