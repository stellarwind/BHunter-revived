#include "bh_sprite_quad.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>

static int fail(BHSpriteQuadContext *c, BHSpriteQuadError e)
{
    if (c)
        c->error = e;
    return 0;
}
static int finite_vec(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int integer(double n, int *out)
{
    if (!_finite(n) || n < -2147483648.0 || n > 2147483647.0)
        return 0;
    *out = (int)n;
    return 1;
}
static int channel(double n, float modulation, unsigned long *out)
{
    int intermediate, result;
    /* Two separate __ftol calls: do not combine the products. */
    if (!integer(n, &intermediate) || !integer((double)intermediate * modulation, &result))
        return 0;
    *out = result < 0 ? 0 : result > 255 ? 255 : (unsigned long)result;
    return 1;
}
int __cdecl bh_sprite_quad_draw(BHSpriteQuadContext *c, BHTextureImage *image, const BHVec3 *center,
                                const BHVec3 *normal, const BHVec3 *up, float width, float height, float angle,
                                unsigned char flags, float modulation, float u, float u_extent, float v, float v_extent)
{
    BHVec3 origin, n, t, r, old_t, old_r, light_normal;
    BHClipVertex quad[4], *ring;
    double side, dot, sine, cosine, alignment, u_end, v_end, tz, rz;
    float sn, cs, su[3], u1, v1;
    unsigned long i, dynamic = 0, red, green, blue;
    int back = 0, shade;
    if (!c || !image || !c->camera || !c->clip || !c->directional || !c->emit ||
        (c->point_enabled && !c->world_lights) || !finite_vec(center) || !finite_vec(normal) || !finite_vec(up) ||
        !finite_vec(&c->camera->position) || !_finite(width) || !_finite(height) || !_finite(angle) ||
        !_finite(modulation) || !_finite(u) || !_finite(u_extent) || !_finite(v) || !_finite(v_extent))
        return fail(c, BH_SPRITE_QUAD_ARGUMENT);
    origin = *center;
    n.x = -normal->x;
    n.y = -normal->y;
    n.z = -normal->z;
    t = *up;
    c->error = BH_SPRITE_QUAD_OK;
    c->emitted = 0;
    side = (((double)c->camera->position.x - origin.x) * n.x + ((double)c->camera->position.y - origin.y) * n.y) +
           ((double)c->camera->position.z - origin.z) * n.z;
    if (side > 0)
    {
        if (!(flags & 8))
            return 1;
        back = 1;
    }
    bh_vec3_normalize(&n);
    bh_vec3_normalize(&t);
    dot = ((double)t.x * n.x + (double)t.y * n.y) + (double)t.z * n.z;
    bh_round_float(&t.x, (double)t.x - dot * n.x);
    bh_round_float(&t.y, (double)t.y - dot * n.y);
    bh_round_float(&t.z, (double)t.z - dot * n.z);
    bh_vec3_normalize(&t);
    bh_vec3_cross(&n, &t, &r);
    tz = t.z;
    rz = r.z;
    if (angle != 0)
    {
        old_t = t;
        old_r = r;
        sine = sin((double)angle);
        cosine = cos((double)angle);
        bh_round_float(&sn, sine);
        bh_round_float(&cs, cosine);
        /* Original retains sin for up.x's temporary, cos for right.x. */
        bh_round_float(su, sine * old_t.x);
        bh_round_float(su + 1, (double)sn * old_t.y);
        bh_round_float(su + 2, (double)sn * old_t.z);
        bh_round_float(&t.x, (double)sn * old_r.x + (double)cs * old_t.x);
        bh_round_float(&t.y, (double)sn * old_r.y + (double)cs * old_t.y);
        tz = (double)sn * old_r.z + (double)cs * old_t.z;
        bh_round_float(&r.x, cosine * old_r.x - su[0]);
        bh_round_float(&r.y, (double)cs * old_r.y - su[1]);
        rz = (double)cs * old_r.z - su[2];
    }
    bh_round_float(&t.x, ((double)t.x * height) * .5);
    bh_round_float(&t.y, ((double)t.y * height) * .5);
    bh_round_float(&t.z, (tz * height) * .5);
    bh_round_float(&r.x, ((double)r.x * width) * .5);
    bh_round_float(&r.y, ((double)r.y * width) * .5);
    bh_round_float(&r.z, (rz * width) * .5);
    if (!finite_vec(&n) || !finite_vec(&t) || !finite_vec(&r))
        return fail(c, BH_SPRITE_QUAD_NUMERICAL);
    memset(quad, 0, sizeof(quad));
    /* A(-r,+t), B(-r,-t), C(+r,-t), D(+r,+t). */
    for (i = 0; i < 3; ++i)
    {
        double o = ((const float *)&origin)[i];
        float a = ((float *)&r)[i], b = ((float *)&t)[i], stored;
        /* A.x/y and D.xyz reload spilled sums; B/C and A.z retain them. */
        if (i < 2)
        {
            bh_round_float(&stored, o - a);
            bh_round_float(((float *)&quad[0].local) + i, (double)stored + b);
        }
        else
            bh_round_float(((float *)&quad[0].local) + i, (o - a) + b);
        bh_round_float(((float *)&quad[1].local) + i, (o - a) - b);
        bh_round_float(((float *)&quad[2].local) + i, (o + a) - b);
        bh_round_float(&stored, (double)a + o);
        bh_round_float(((float *)&quad[3].local) + i, (double)stored + b);
    }
    u_end = (double)u_extent + u;
    v_end = (double)v_extent + v;
    if (flags & 1)
    {
        bh_round_float(&u, 1.0 - u);
        u_end = 1.0 - u_end;
    }
    if (flags & 2)
    {
        bh_round_float(&v, 1.0 - v);
        v_end = 1.0 - v_end;
    }
    bh_round_float(&u1, u_end);
    bh_round_float(&v1, v_end);
    /* Original stack vertices: A=local_b4, B=local_78, C=local_3c,
       D=local_f0. U follows the right basis; V follows the up basis.
       The opposite pairing rotates/stretches rectangular menu labels. */
    quad[0].u = u;
    quad[0].v = v;
    quad[1].u = u;
    quad[1].v = v1;
    quad[2].u = u1;
    quad[2].v = v1;
    quad[3].u = u1;
    quad[3].v = v;
    if (flags & 4)
        for (i = 0; i < 4; ++i)
        {
            float swap = quad[i].u;
            quad[i].u = quad[i].v;
            quad[i].v = swap;
        }
    for (i = 0; i < 4; ++i)
    {
        quad[i].next = quad + (i + (back ? 3 : 1)) % 4;
        quad[i].previous = quad + (i + (back ? 1 : 3)) % 4;
        bh_mat34_transform_point(&c->camera->projected, &quad[i].local, &quad[i].point);
    }
    ring = bh_clip_polygon(c->clip, quad, image);
    if (!ring)
    {
        if (c->clip->error != BH_CLIP_OK)
            return fail(c, BH_SPRITE_QUAD_CLIP);
        return 1;
    }
    if (!c->directional->enabled && !c->point_enabled)
    {
        if (!integer((double)modulation * -255.0, &shade))
            return fail(c, BH_SPRITE_QUAD_NUMERICAL);
        c->color = c->base_color - (unsigned long)shade * 0x10101UL;
    }
    else
    {
        if (c->point_enabled)
        {
            light_normal = n;
            if (!back)
            {
                light_normal.x = -n.x;
                light_normal.y = -n.y;
                light_normal.z = -n.z;
            }
            if (!bh_light_evaluate_world(c->world_lights, &origin, &light_normal, modulation, &dynamic))
                return fail(c, BH_SPRITE_QUAD_LIGHT);
        }
        if (c->directional->enabled)
        {
            if (!finite_vec(&c->directional->direction))
                return fail(c, BH_SPRITE_QUAD_ARGUMENT);
            alignment = ((double)n.x * c->directional->direction.x + (double)n.y * c->directional->direction.y) +
                        (double)n.z * c->directional->direction.z;
            if (!back)
                alignment = -alignment;
            if (alignment < 0)
                alignment = 0;
            if (!channel((alignment * c->directional->directional[0] + ((dynamic >> 16) & 255)) +
                             c->directional->ambient[0],
                         modulation, &red) ||
                !channel((alignment * c->directional->directional[1] + ((dynamic >> 8) & 255)) +
                             c->directional->ambient[1],
                         modulation, &green) ||
                !channel((alignment * c->directional->directional[2] + (dynamic & 255)) + c->directional->ambient[2],
                         modulation, &blue))
                return fail(c, BH_SPRITE_QUAD_NUMERICAL);
            dynamic = (red * 256 + green) * 256 + blue;
        }
        c->color = dynamic + c->base_color;
    }
    if (!c->emit(ring, image, c->color, c->emit_user))
        return fail(c, BH_SPRITE_QUAD_EMIT);
    c->emitted = 1;
    return 1;
}
