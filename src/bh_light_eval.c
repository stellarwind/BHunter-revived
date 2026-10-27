#include "bh_light_eval.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>

static int finite_vector(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int valid(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale, int prepared,
                 unsigned long *result)
{
    const BHLight *p, *slow, *fast;
    unsigned long type;
    if (!c || !result || !_finite(scale))
        return 0;
    slow = fast = c->lights;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 0;
    }
    for (p = c->lights; p; p = p->next)
    {
        type = prepared ? p->prepared_type : p->type;
        if (type == 1 || type == 2)
            if (!_finite(p->effective_intensity))
                return 0;
        if (type >= 2 && type <= 4 && !finite_vector(normal))
            return 0;
        if (type == 2 || type == 4)
            if (!finite_vector(prepared ? &p->local_direction : &p->direction))
                return 0;
        if (type == 3 || type == 4)
        {
            if (!finite_vector(point) || !finite_vector(prepared ? &p->local_position : &p->position) ||
                !_finite(p->intensity) || !_finite(p->range))
                return 0;
            if (type == 4 && (!_finite(p->inner_cos) || !_finite(p->outer_cos)))
                return 0;
        }
    }
    return 1;
}
static unsigned long channel(double value)
{
    int n = (int)value; /* verified CRT __ftol supplies truncation toward zero */
    return n < 0 ? 0 : (n > 255 ? 255 : (unsigned long)n);
}
static int evaluate(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                    unsigned long *result, int prepared)
{
    const BHLight *p;
    const BHVec3 *position, *direction;
    double red = 0, green = 0, strength, x, y, z, distance, attenuation, alignment, rr, gg, bb;
    float blue = 0, stored_x, stored_y, stored_z, stored_distance, stored_strength, stored_alignment, stored_green;
    unsigned long type, packed;
    if (c && c->reference_override)
        point = prepared ? &c->local_reference : c->reference_override;
    if (!valid(c, point, normal, scale, prepared, result))
        return 0;
    for (p = c->lights; p; p = p->next)
    {
        type = prepared ? p->prepared_type : p->type;
        if (type < 1 || type > 4)
            continue;
        position = prepared ? &p->local_position : &p->position;
        direction = prepared ? &p->local_direction : &p->direction;
        if (type == 1)
            strength = p->effective_intensity;
        else if (type == 2)
        {
            strength = -(((double)normal->y * direction->y + (double)normal->z * direction->z) +
                         (double)normal->x * direction->x);
            if (!(strength > 0))
                continue;
            strength *= p->effective_intensity;
        }
        else
        {
            x = (double)point->x - position->x;
            y = (double)point->y - position->y;
            z = (double)point->z - position->z;
            bh_round_float(&stored_z, z);
            if (type == 4)
            {
                bh_round_float(&stored_x, x);
                bh_round_float(&stored_y, y);
                distance = sqrt((x * stored_x + y * stored_y) + z * stored_z);
            }
            else
                distance = sqrt((x * x + y * y) + z * stored_z);
            bh_round_float(&stored_distance, distance);
            /* Coincident point/spot produces unordered diffuse/cone values in
               the masked original and makes no positive contribution. */
            if (stored_distance == 0)
                continue;
            attenuation = p->range > 0 ? 1.0 - (double)stored_distance / p->range : 1.0;
            if (attenuation < 0)
                attenuation = 0;
            if (type == 4)
            {
                bh_round_float(&stored_strength, attenuation);
                attenuation = stored_strength;
                /* Prepared and world owners use different addition orders. */
                alignment = prepared ? (((double)direction->z * stored_z + (double)direction->y * stored_y) +
                                        (double)direction->x * stored_x) /
                                           stored_distance
                                     : (((double)direction->x * stored_x + (double)direction->y * stored_y) +
                                        (double)direction->z * stored_z) /
                                           stored_distance;
                bh_round_float(&stored_alignment, alignment);
                strength =
                    -(((double)normal->y * stored_y + (double)normal->z * stored_z) + (double)normal->x * stored_x) /
                    stored_distance;
            }
            else
                strength =
                    -(((double)normal->y * y + (double)normal->z * stored_z) + (double)normal->x * x) / stored_distance;
            bh_round_float(&stored_strength, (strength * p->intensity) * attenuation);
            strength = stored_strength;
            if (type == 4)
            {
                if (alignment < p->outer_cos)
                    strength = 0;
                else if (stored_alignment < p->inner_cos)
                    strength =
                        (((double)stored_alignment - p->outer_cos) / ((double)p->inner_cos - p->outer_cos)) * strength;
            }
            if (!(strength > 0))
                continue;
        }
        red += ((p->color >> 16) & 255) * strength;
        green += ((p->color >> 8) & 255) * strength;
        /* Only blue is rounded after each addition in the original. */
        bh_round_float(&blue, (p->color & 255) * strength + blue);
    }
    bh_round_float(&stored_green, green);
    rr = red * scale;
    gg = (double)stored_green * scale;
    bb = (double)blue * scale;
    if (!_finite(rr) || !_finite(gg) || !_finite(bb) || rr < -2147483648.0 || rr > 2147483647.0 || gg < -2147483648.0 ||
        gg > 2147483647.0 || bb < -2147483648.0 || bb > 2147483647.0)
        return 0;
    packed = (channel(rr) << 16) | (channel(gg) << 8) | channel(bb);
    *result = packed;
    return 1;
}
int __cdecl bh_light_evaluate_prepared(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                                       unsigned long *result)
{
    return evaluate(c, point, normal, scale, result, 1);
}
int __cdecl bh_light_evaluate_world(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                                    unsigned long *result)
{
    return evaluate(c, point, normal, scale, result, 0);
}
