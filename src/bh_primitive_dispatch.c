#include "bh_primitive_dispatch.h"
#include "bh_texture_draw.h"
#include "bh_round.h"
#include <float.h>
#include <string.h>
#define ENTRY(P, N) bh_raster_##P##_##N,
static BHRasterKernel kernels[28] = {BH_RASTER_KERNELS(ENTRY, indexed) BH_RASTER_KERNELS(ENTRY, packed)};
#undef ENTRY
static int fail(BHPrimitiveDispatch *c, BHPrimitiveError e)
{
    if (c)
        c->error = e;
    return 0;
}
static int leaf(BHClipVertex *ring, void *user)
{
    BHPrimitiveDispatch *c = (BHPrimitiveDispatch *)user;
    return c->selected(&c->raster, ring);
}
int __cdecl bh_primitive_dispatch_init(BHPrimitiveDispatch *c, BHRenderState *state, const BHSoftwareRaster *frame,
                                       float left, float right, float top, float bottom)
{
    if (!c || !state || !state->textures || !frame || !_finite(frame->near_z) || frame->near_z <= 0)
        return 0;
    memset(c, 0, sizeof(*c));
    c->state = state;
    c->raster = *frame;
    c->key_enabled = 1;
    c->base_color = 0x80000000UL;
    c->selected_index = 0xffffffffUL;
    return bh_depth_split_init(&c->split, left, right, top, bottom, leaf, c);
}
static unsigned long shade(unsigned long color)
{
    unsigned long r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
    if ((r >> 5) == (g >> 5) && (r >> 6) == (b >> 6))
        return (r + g + b) / 3;
    return 256 + (((r >> 5) * 8 + (g >> 5)) * 4 + (b >> 6));
}
static int select_kernel(BHPrimitiveDispatch *c, BHTextureImage *image, unsigned long color)
{
    unsigned long r = (color >> 16) & 255, g = (color >> 8) & 255, b = color & 255;
    unsigned long packed = c->raster.tables->bytes_per_pixel == 1 ? 0 : 14, index, mode = c->state->mode, bucket;
    int shaded = r + g + b <= 764 || c->raster.attribute, masked = c->key_enabled && image->format->kind;
    if (mode == 0)
    {
        if (shaded)
            c->raster.shade_offset = shade(color) * 256;
        if (c->state->depth <= 2)
        {
            index = packed + (shaded ? 6 : 0) + (masked ? 3 : 0) + c->state->depth;
            c->selected = kernels[index];
            c->selected_index = index;
        }
    }
    else
    {
        if (mode <= 3)
            c->raster.blend_plane = mode - 1;
        else if (mode == 4)
        {
            bucket = ((color >> 24) * 7) >> 8;
            if (!bucket)
                return 0;
            c->raster.blend_plane = bucket + 2;
            if (bucket == 6)
            {
                c->raster.shade_offset = shade(color) * 256;
                c->selected_index = packed + 10;
                c->selected = kernels[c->selected_index];
                return 1;
            }
        }
        if (shaded)
            c->raster.shade_offset = shade(color) * 256;
        c->selected_index = packed + 12 + (shaded ? 1 : 0);
        c->selected = kernels[c->selected_index];
    }
    return c->selected ? 1 : -1;
}
int __cdecl bh_software_polygon(BHPrimitiveDispatch *c, BHTextureImage *image, BHClipVertex *ring, unsigned long color)
{
    DDSURFACEDESC locked;
    HRESULT hr;
    int selected, ok = 1;
    if (!c || !c->state || !c->state->textures || !image || !image->source || !image->format || !ring ||
        !c->raster.tables || (c->raster.tables->bytes_per_pixel != 1 && c->raster.tables->bytes_per_pixel != 2))
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    c->error = BH_PRIMITIVE_OK;
    memset(&locked, 0, sizeof(locked));
    locked.dwSize = sizeof(locked);
    ++c->locks;
    c->last_result = IDirectDrawSurface_Lock(image->source, NULL, &locked, 0, NULL);
    if (c->last_result != DD_OK)
        return fail(c, BH_PRIMITIVE_BACKEND);
    if (!locked.lpSurface || locked.dwWidth != image->format->width || locked.dwHeight < locked.dwWidth ||
        locked.lPitch != (long)locked.dwWidth)
    {
        fail(c, BH_PRIMITIVE_ARGUMENT);
        ok = 0;
    }
    else
    {
        c->raster.texels = (const unsigned char *)locked.lpSurface;
        c->raster.texture_width = image->format->width;
        c->raster.image = image;
        selected = select_kernel(c, image, color);
        if (selected < 0)
        {
            fail(c, BH_PRIMITIVE_ARGUMENT);
            ok = 0;
        }
        else if (selected)
        {
            ok = c->raster.attribute ? bh_depth_subdivide_scalar(&c->split, ring) : bh_depth_subdivide(&c->split, ring);
            if (!ok)
                fail(c, BH_PRIMITIVE_RASTER);
        }
    }
    ++c->unlocks;
    hr = IDirectDrawSurface_Unlock(image->source, NULL);
    c->last_result = hr;
    if (hr != DD_OK)
    {
        fail(c, BH_PRIMITIVE_BACKEND);
        ok = 0;
    }
    /* Lock storage is borrowed only until Unlock; never retain a stale pixel
       pointer as an apparently live source texture. */
    c->raster.texels = NULL;
    if (ok)
        ++c->polygons;
    return ok;
}
static int ring_info(BHPrimitiveDispatch *c, BHClipVertex *ring, unsigned long *count, double *sum, int attribute)
{
    BHClipVertex *p = ring;
    float scalar;
    *count = 0;
    *sum = 0;
    if (!p)
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    do
    {
        if (!p || !p->next || !p->previous || p->next->previous != p || p->previous->next != p || ++*count > 1024)
            return fail(c, BH_PRIMITIVE_ARGUMENT);
        if (!_finite(p->point.z) || p->point.z <= 0 || !_finite(p->sx) || !_finite(p->sy) || !_finite(p->u) ||
            !_finite(p->v))
            return fail(c, BH_PRIMITIVE_ARGUMENT);
        if (attribute)
        {
            memcpy(&scalar, &p->unknown28, 4);
            if (!_finite(scalar))
                return fail(c, BH_PRIMITIVE_ARGUMENT);
            *sum += scalar;
        }
        p = p->next;
    } while (p != ring);
    return *count >= 3;
}
static int native_emit(BHPrimitiveDispatch *c, BHTextureImage *image, BHClipVertex *ring, unsigned long color, int kind)
{
    BHClipVertex *p = ring;
    BHTextureContext *textures = c->state->textures;
    D3DTLVERTEX *out;
    unsigned long n;
    double sum, component;
    long converted;
    float scalar;
    if (!image || !ring_info(c, ring, &n, &sum, kind != 0))
        return 0;
    do
    {
        component = 0;
        if (kind)
        {
            memcpy(&scalar, &p->unknown28, 4);
            component = (double)scalar * -255.0;
        }
        if (kind && (component < -2147483648.0 || component > 2147483647.0))
            return fail(c, BH_PRIMITIVE_ARGUMENT);
        converted = kind ? (long)component : 0;
        out = bh_texture_draw_vertex(textures, image);
        if (!out)
            return fail(c, BH_PRIMITIVE_QUEUE);
        out->sx = p->sx;
        out->sy = p->sy;
        bh_round_float(&out->sz, (double)c->raster.near_z / p->point.z);
        bh_round_float(&out->rhw, 1.0 / p->point.z);
        out->tu = p->u;
        out->tv = p->v;
        out->color = kind == 2 ? 0xffffffUL : kind == 1 ? c->base_color - (unsigned long)converted * 0x10101UL : color;
        out->specular = kind == 2 ? 0xff000000UL + ((unsigned long)converted << 24) : 0;
        ++c->vertices;
        p = p->next;
    } while (p != ring);
    image->runs[image->run_count++] = image->run_serial;
    image->run_serial = 0;
    ++c->polygons;
    return 1;
}
int __cdecl bh_primitive_emit(BHPrimitiveDispatch *c, BHTextureImage *image, BHClipVertex *ring, unsigned long color)
{
    if (!c || !c->state || !c->state->textures)
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    c->error = BH_PRIMITIVE_OK;
    if (c->state->textures->software)
        return bh_software_polygon(c, image, ring, color);
    return native_emit(c, image, ring, color, 0);
}
int __cdecl bh_primitive_emit_scalar(BHPrimitiveDispatch *c, BHTextureImage *image, BHClipVertex *ring)
{
    unsigned long n;
    double sum, scaled;
    long value;
    if (!c || !c->state || !c->state->textures)
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    c->error = BH_PRIMITIVE_OK;
    if (!c->state->textures->software)
        return native_emit(c, image, ring, 0, 1);
    if (!ring_info(c, ring, &n, &sum, 1))
        return 0;
    scaled = (sum / n) * 255.0;
    if (scaled < -2147483648.0 || scaled > 2147483647.0)
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    value = (long)scaled;
    return bh_software_polygon(c, image, ring, c->base_color + (unsigned long)value * 0x10101UL);
}
int __cdecl bh_primitive_emit_gradient(BHPrimitiveDispatch *c, BHTextureImage *image, BHClipVertex *ring,
                                       unsigned long color)
{
    int ok;
    if (!c || !c->state || !c->state->textures)
        return fail(c, BH_PRIMITIVE_ARGUMENT);
    c->error = BH_PRIMITIVE_OK;
    if (!c->state->textures->software)
        return native_emit(c, image, ring, color, 2);
    c->raster.attribute = 1;
    ok = bh_software_polygon(c, image, ring, color);
    c->raster.attribute = 0;
    return ok;
}
