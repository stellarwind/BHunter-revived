#include "bh_light_eval.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <math.h>

static int finite_vector(const BHVec3 *vector)
{
    return vector && _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static int valid(const BHLightContext *context, const BHVec3 *point, const BHVec3 *normal, float scale, int prepared,
                 unsigned long *result)
{
    const BHLight *light, *slow, *fast;
    unsigned long type;
    if (!context || !result || !_finite(scale))
        return 0;
    slow = fast = context->lights;
    while (fast && fast->next)
    {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast)
            return 0;
    }
    for (light = context->lights; light; light = light->next)
    {
        type = prepared ? light->prepared_type : light->type;
        if (type == 1 || type == 2)
            if (!_finite(light->effective_intensity))
                return 0;
        if (type >= 2 && type <= 4 && !finite_vector(normal))
            return 0;
        if (type == 2 || type == 4)
            if (!finite_vector(prepared ? &light->local_direction : &light->direction))
                return 0;
        if (type == 3 || type == 4)
        {
            if (!finite_vector(point) || !finite_vector(prepared ? &light->local_position : &light->position) ||
                !_finite(light->intensity) || !_finite(light->range))
                return 0;
            if (type == 4 && (!_finite(light->inner_cos) || !_finite(light->outer_cos)))
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
static int evaluate(const BHLightContext *context, const BHVec3 *point, const BHVec3 *normal, float scale,
                    unsigned long *result, int prepared)
{
    const BHLight *light;
    const BHVec3 *position, *direction;
    double red = 0, green = 0, strength, x, y, z, distance, attenuation, alignment, rr, gg, bb;
    float blue = 0, stored_x, stored_y, stored_z, stored_distance, stored_strength, stored_alignment, stored_green;
    unsigned long type, packed;
    if (context && context->reference_override)
        point = prepared ? &context->local_reference : context->reference_override;
    if (!valid(context, point, normal, scale, prepared, result))
        return 0;
    for (light = context->lights; light; light = light->next)
    {
        type = prepared ? light->prepared_type : light->type;
        if (type < 1 || type > 4)
            continue;
        position = prepared ? &light->local_position : &light->position;
        direction = prepared ? &light->local_direction : &light->direction;
        if (type == 1)
            strength = light->effective_intensity;
        else if (type == 2)
        {
            strength = -(((double)normal->y * direction->y + (double)normal->z * direction->z) +
                         (double)normal->x * direction->x);
            if (!(strength > 0))
                continue;
            strength *= light->effective_intensity;
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
            attenuation = light->range > 0 ? 1.0 - (double)stored_distance / light->range : 1.0;
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
            bh_round_float(&stored_strength, (strength * light->intensity) * attenuation);
            strength = stored_strength;
            if (type == 4)
            {
                if (alignment < light->outer_cos)
                    strength = 0;
                else if (stored_alignment < light->inner_cos)
                    strength =
                        (((double)stored_alignment - light->outer_cos) / ((double)light->inner_cos - light->outer_cos)) * strength;
            }
            if (!(strength > 0))
                continue;
        }
        red += ((light->color >> 16) & 255) * strength;
        green += ((light->color >> 8) & 255) * strength;
        /* Only blue is rounded after each addition in the original. */
        bh_round_float(&blue, (light->color & 255) * strength + blue);
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
int bh_light_evaluate_prepared(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                               unsigned long *result)
{
    return evaluate(c, point, normal, scale, result, 1);
}
int bh_light_evaluate_world(const BHLightContext *c, const BHVec3 *point, const BHVec3 *normal, float scale,
                            unsigned long *result)
{
    return evaluate(c, point, normal, scale, result, 0);
}
