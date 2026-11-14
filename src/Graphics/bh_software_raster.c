#include "bh_software_raster.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <float.h>
#include <string.h>
#include <stddef.h>

static_assert(offsetof(BHClipVertex, sx) == 0x20 && offsetof(BHClipVertex, sy) == 0x24, "raster screen layout");
static_assert(offsetof(BHClipVertex, u) == 0x18 && offsetof(BHClipVertex, v) == 0x1c, "raster uv layout");
typedef struct RasterEdge
{
    BHClipVertex *below;
    float x, u, v, dx, du, dv, dz;
    double z;
} RasterEdge;
typedef struct RasterGradient
{
    long z, u, v;
} RasterGradient;
static int fail(BHSoftwareRaster *software_raster, BHRasterError e)
{
    if (software_raster)
        software_raster->error = e;
    return 0;
}
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static long add32(long a, long b)
{
    return (long)((unsigned long)a + (unsigned long)b);
}
static long sub32(long a, long b)
{
    return (long)((unsigned long)a - (unsigned long)b);
}
static long integer(BHSoftwareRaster *software_raster, double x)
{
    __int64 value;
    /* VC5 __ftol returns signed64, and these owners store EAX only. A masked
       invalid conversion returns 80000000:00000000, so its low word is zero.
       Valid clipped vertical edges can produce such gradients. Preserve that
       value contract without an undefined C conversion; x87 sticky status is
       outside this source adapter's existing control-only FP contract. */
    if (!_finite(x) || x < -9223372036854775808.0 || x >= 9223372036854775808.0)
    {
        software_raster->rejected_value = x;
        ++software_raster->masked_conversions;
        return 0;
    }
    value = (__int64)x;
    return (long)value;
}
static int gradient(BHSoftwareRaster *software_raster, BHClipVertex *left, BHClipVertex *right, BHClipVertex *la, BHClipVertex *ra,
                    int bottom, RasterGradient *out)
{
    double d, divisor, dy_left, dy_right, z, nu, nv;
    float rounded_d;
    double scale = (double)(software_raster->texture_width - 1);
    if (la == ra)
    {
        dy_left = (double)left->sy - la->sy;
        dy_right = stored((double)right->sy - ra->sy);
        if (!bottom)
            dy_left = stored(dy_left);
        d = ((double)right->sx - ra->sx) * dy_left - ((double)left->sx - la->sx) * dy_right;
        rounded_d = stored(d);
        z = (((double)software_raster->near_z / right->point.z - (double)software_raster->near_z / ra->point.z) * dy_left -
             ((double)software_raster->near_z / left->point.z - (double)software_raster->near_z / la->point.z) * dy_right) *
            16777216.0 / d;
        nu = ((double)right->u - ra->u) * dy_left - ((double)left->u - la->u) * dy_right;
        nv = ((double)right->v - ra->v) * dy_left - ((double)left->v - la->v) * dy_right;
        divisor = rounded_d;
    }
    else
    {
        d = (double)ra->sx - la->sx;
        divisor = bottom ? d : (double)stored(d);
        z = ((double)software_raster->near_z / ra->point.z - (double)software_raster->near_z / la->point.z) * 16777216.0 / divisor;
        nu = (double)ra->u - la->u;
        nv = (double)ra->v - la->v;
    }
    out->z = integer(software_raster, z);
    out->u = integer(software_raster, (nu * scale) * 65536.0 / divisor);
    out->v = integer(software_raster, (nv * scale) * 65536.0 / divisor);
    return 1;
}
static BHClipVertex *walk(BHClipVertex *vertex, long y, int reverse, int below)
{
    unsigned long i = 0;
    do
    {
        vertex = reverse ? vertex->previous : vertex->next;
        if (++i > 1024)
            return NULL;
    } while (below ? (long)vertex->sy <= y : (long)vertex->sy >= y);
    return vertex;
}
static void left_edge(BHSoftwareRaster *software_raster, RasterEdge *edge, long y)
{
    BHClipVertex *below = edge->below, *above = below->previous;
    double inv = 1.0 / ((double)below->sy - above->sy);
    float inverse = stored(inv), delta = stored((double)(y + 1) - above->sy);
    double scale = (double)(software_raster->texture_width - 1), bu = ((double)above->u * scale) * 65536.0;
    double bv = ((double)above->v * scale) * 65536.0, bz = ((double)software_raster->near_z / above->point.z) * 16777216.0;
    double dx = ((double)below->sx - above->sx) * inverse;
    double du = ((((double)below->u * scale) * 65536.0) - bu) * inv;
    double dv = ((((double)below->v * scale) * 65536.0) - bv) * inverse;
    double dz = (((double)software_raster->near_z / below->point.z) * 16777216.0 - bz) * inverse;
    edge->dx = stored(dx);
    edge->du = stored(du);
    edge->dv = stored(dv);
    edge->dz = stored(dz);
    edge->x = stored(dx * delta + above->sx);
    edge->u = stored(du * delta + stored(bu));
    edge->v = stored(dv * delta + stored(bv));
    edge->z = dz * delta + stored(bz);
}
static void right_edge(RasterEdge *edge, long y)
{
    BHClipVertex *below = edge->below, *above = below->next;
    edge->dx = stored(((double)below->sx - above->sx) * (1.0 / ((double)below->sy - above->sy)));
    edge->x = stored(((double)(y + 1) - above->sy) * edge->dx + above->sx);
}
static int run(BHSoftwareRaster *software_raster, BHClipVertex *ring, int packed, int masked, int shaded, int blend, int policy)
{
    BHClipVertex *vertex, *minimum, *maximum, *left_vertex, *right_vertex;
    RasterEdge left, right;
    RasterGradient top, end, step;
    const unsigned char *shade_i;
    const unsigned short *shade_p;
    unsigned long n = 0, i, mask, log = 0, parity, texel, mapped;
    long first, last, y, x, count, u, v, z, rows;
    unsigned short depth, *depth_row, *destination;
    unsigned char *row;
    double sum = 0, scalar;
    float value;
    if (!software_raster || !ring || !software_raster->pixels || !software_raster->depth || !software_raster->texels || !software_raster->image || !software_raster->tables || !software_raster->image->dither ||
        (!software_raster->image->screen && packed && !shaded && !blend) ||
        software_raster->tables->bytes_per_pixel != (unsigned long)(packed ? 2 : 1) || !software_raster->width || !software_raster->height || software_raster->width > 4096 ||
        software_raster->height > 4096 || !_finite(software_raster->near_z) || software_raster->near_z <= 0 || software_raster->texture_width < 2 || software_raster->texture_width > 2048 ||
        (software_raster->texture_width & (software_raster->texture_width - 1)) ||
        (software_raster->pitch >= 0 ? software_raster->pitch : -(double)software_raster->pitch) < software_raster->width * (packed ? 2 : 1) ||
        (software_raster->depth_pitch >= 0 ? software_raster->depth_pitch : -(double)software_raster->depth_pitch) < software_raster->width * 2 || (software_raster->depth_pitch & 1) ||
        !bh_math_environment_supported())
        return fail(software_raster, BH_RASTER_ARGUMENT);
    software_raster->error = BH_RASTER_OK;
    software_raster->spans = software_raster->visited = software_raster->written = 0;
    if (software_raster->shade_offset >= 131072 - 255 || (blend && software_raster->blend_plane >= 8))
        return fail(software_raster, BH_RASTER_ARGUMENT);
    vertex = minimum = maximum = ring;
    do
    {
        if (!vertex || !vertex->next || !vertex->previous || vertex->next->previous != vertex || vertex->previous->next != vertex || ++n > 1024)
            return fail(software_raster, BH_RASTER_TOPOLOGY);
        if (!_finite(vertex->sx) || !_finite(vertex->sy) || !_finite(vertex->u) || !_finite(vertex->v) || !_finite(vertex->point.z) ||
            vertex->point.z <= 0)
            return fail(software_raster, BH_RASTER_NUMERICAL);
        if (vertex->sx < 0 || vertex->sx > software_raster->width || vertex->sy < 0 || vertex->sy > software_raster->height)
            return fail(software_raster, BH_RASTER_BOUNDS);
        if (vertex->sy < minimum->sy)
            minimum = vertex;
        if (vertex->sy > maximum->sy)
            maximum = vertex;
        if (software_raster->attribute)
        {
            memcpy(&value, &vertex->unknown28, 4);
            if (!_finite(value))
                return fail(software_raster, BH_RASTER_NUMERICAL);
            sum += value;
        }
        vertex = vertex->next;
    } while (vertex != ring);
    if (n < 3)
        return fail(software_raster, BH_RASTER_TOPOLOGY);
    first = (long)minimum->sy;
    last = (long)maximum->sy;
    if (first >= last)
        return 1;
    shade_i = software_raster->tables->shade_indexed ? software_raster->tables->shade_indexed + software_raster->shade_offset : NULL;
    shade_p = software_raster->tables->shade_native ? software_raster->tables->shade_native + software_raster->shade_offset : NULL;
    if (software_raster->attribute)
    {
        scalar = 255.0 - (sum / n) * 255.0;
        x = integer(software_raster, scalar);
        if (x < 0)
            x = 0;
        else if (x > 255)
            x = 255;
        software_raster->attribute_row = (unsigned long)x;
        if (!software_raster->gradient)
            return fail(software_raster, BH_RASTER_ARGUMENT);
        if (packed)
        {
            if (!software_raster->gradient->packed)
                return fail(software_raster, BH_RASTER_ARGUMENT);
            shade_p = software_raster->gradient->packed + software_raster->attribute_row * 256;
        }
        else
        {
            if (!software_raster->gradient->indexed)
                return fail(software_raster, BH_RASTER_ARGUMENT);
            shade_i = software_raster->gradient->indexed + software_raster->attribute_row * 256;
        }
    }
    if (shaded && ((packed && !blend) ? !shade_p : !shade_i))
        return fail(software_raster, BH_RASTER_ARGUMENT);
    if (blend && (packed ? !software_raster->tables->blend_native : !software_raster->tables->blend_indexed))
        return fail(software_raster, BH_RASTER_ARGUMENT);
    left_vertex = walk(minimum, first, 0, 1);
    right_vertex = walk(minimum, first, 1, 1);
    if (!left_vertex || !right_vertex)
        return fail(software_raster, BH_RASTER_TOPOLOGY);
    if (!gradient(software_raster, left_vertex, right_vertex, left_vertex->previous, right_vertex->next, 0, &top))
        return 0;
    left_vertex = walk(maximum, last, 1, 0);
    right_vertex = walk(maximum, last, 0, 0);
    if (!left_vertex || !right_vertex)
        return fail(software_raster, BH_RASTER_TOPOLOGY);
    /* Bottom gradients use the upper vertices as their edge tips. */
    if (!gradient(software_raster, left_vertex, right_vertex, left_vertex->next, right_vertex->previous, 1, &end))
        return 0;
    rows = last - first;
    step.z = sub32(end.z, top.z) / rows;
    step.u = sub32(end.u, top.u) / rows;
    step.v = sub32(end.v, top.v) / rows;
    memset(&left, 0, sizeof(left));
    memset(&right, 0, sizeof(right));
    left.below = right.below = minimum;
    mask = software_raster->texture_width - 1;
    for (i = software_raster->texture_width; i > 1; i >>= 1)
        ++log;
    for (y = first; y < last; y++)
    {
        if ((long)left.below->sy == y)
        {
            left.below = walk(left.below, y, 0, 1);
            if (!left.below)
                return fail(software_raster, BH_RASTER_TOPOLOGY);
            left_edge(software_raster, &left, y);
        }
        else
        {
            left.x = stored((double)left.x + left.dx);
            left.u = stored((double)left.u + left.du);
            left.v = stored((double)left.v + left.dv);
            left.z += left.dz;
        }
        if ((long)right.below->sy == y)
        {
            right.below = walk(right.below, y, 1, 1);
            if (!right.below)
                return fail(software_raster, BH_RASTER_TOPOLOGY);
            right_edge(&right, y);
        }
        else
            right.x = stored((double)right.x + right.dx);
        if (!_finite(left.x) || !_finite(right.x))
            return fail(software_raster, BH_RASTER_NUMERICAL);
        if (right.x < left.x)
            return 1;
        x = integer(software_raster, left.x);
        count = integer(software_raster, (double)right.x - x);
        u = integer(software_raster, left.u);
        v = integer(software_raster, left.v);
        z = integer(software_raster, left.z);
        if (x < 0 || count < 0 || (unsigned long)x > software_raster->width || (unsigned long)count > software_raster->width - (unsigned long)x)
            return fail(software_raster, BH_RASTER_BOUNDS);
        row = software_raster->pixels + y * software_raster->pitch;
        depth_row = (unsigned short *)((unsigned char *)software_raster->depth + y * software_raster->depth_pitch);
        parity = ((unsigned long)y & 1) * 512 + ((unsigned long)x & 1) * 256;
        ++software_raster->spans;
        for (i = 0; i < (unsigned long)count; i++)
        {
            ++software_raster->visited;
            depth = (unsigned short)((unsigned long)z >> 8);
            if (policy == 2 || depth_row[x] <= depth)
            {
                texel =
                    software_raster->texels[((unsigned long)(u >> 16) & mask) + (((unsigned long)(v >> (16 - log)) & (mask << log)))];
                if (!masked || texel)
                {
                    if (packed && !shaded && !blend)
                    {
                        destination = (unsigned short *)row;
                        destination[x] = software_raster->image->screen[parity + texel];
                        ++software_raster->written;
                        if (policy != 1)
                            depth_row[x] = depth;
                    }
                    else
                    {
                        mapped = software_raster->image->dither[parity + texel];
                        if (shaded && (!packed || blend))
                            mapped = shade_i[mapped];
                        if (!blend)
                        {
                            if (packed)
                                ((unsigned short *)row)[x] = shade_p[mapped];
                            else
                                row[x] = (unsigned char)mapped;
                            ++software_raster->written;
                            if (policy != 1)
                                depth_row[x] = depth;
                        }
                        else if (mapped)
                        {
                            if (packed)
                            {
                                unsigned long color = ((unsigned short *)row)[x], outer = 0, j;
                                for (j = 0; j < 3; j++)
                                    outer += (color & software_raster->tables->rgb332_mask[j])
                                             << ((unsigned long)software_raster->tables->rgb332_shift[j] & 31);
                                ((unsigned short *)row)[x] =
                                    software_raster->tables->blend_native[software_raster->blend_plane * 65536 + (outer & 65535) + mapped];
                            }
                            else
                                row[x] =
                                    software_raster->tables
                                        ->blend_indexed[software_raster->blend_plane * 65536 + (unsigned long)row[x] * 256 + mapped];
                            ++software_raster->written;
                        }
                    }
                }
            }
            z = add32(z, top.z);
            u = add32(u, top.u);
            v = add32(v, top.v);
            ++x;
            parity ^= 256;
        }
        top.z = add32(top.z, step.z);
        top.u = add32(top.u, step.u);
        top.v = add32(top.v, step.v);
    }
    return 1;
}
#define KERNEL(P, N, B, M, S, L, Z)                                                                                    \
    int bh_raster_##P##_##N(BHSoftwareRaster *c, BHClipVertex *v)                                                      \
    {                                                                                                                  \
        return run(c, v, B, M, S, L, Z);                                                                               \
    }
#define FAMILY(P, B)                                                                                                   \
    KERNEL(P, plain_test_write, B, 0, 0, 0, 0)                                                                         \
    KERNEL(P, plain_test, B, 0, 0, 0, 1)                                                                               \
    KERNEL(P, plain_write, B, 0, 0, 0, 2)                                                                              \
    KERNEL(P, masked_test_write, B, 1, 0, 0, 0)                                                                        \
    KERNEL(P, masked_test, B, 1, 0, 0, 1)                                                                              \
    KERNEL(P, masked_write, B, 1, 0, 0, 2)                                                                             \
    KERNEL(P, shade_test_write, B, 0, 1, 0, 0)                                                                         \
    KERNEL(P, shade_test, B, 0, 1, 0, 1)                                                                               \
    KERNEL(P, shade_write, B, 0, 1, 0, 2)                                                                              \
    KERNEL(P, masked_shade_test_write, B, 1, 1, 0, 0)                                                                  \
    KERNEL(P, masked_shade_test, B, 1, 1, 0, 1)                                                                        \
    KERNEL(P, masked_shade_write, B, 1, 1, 0, 2)                                                                       \
    KERNEL(P, blend, B, 1, 0, 1, 1)                                                                                    \
    KERNEL(P, shade_blend, B, 1, 1, 1, 1)
FAMILY(indexed, 0)
FAMILY(packed, 1)
#undef FAMILY
#undef KERNEL
