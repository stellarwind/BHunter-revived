#include "bh_light.h"
#include "../Math/bh_round.h"
#include <math.h>
#include <float.h>
static int finite_vector(const BHVec3 *vector)
{
    return vector && _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static int valid(const BHLightContext *context, const BHVec3 *reference, float radius)
{
    const BHLight *light, *slow, *fast;
    unsigned long i;
    if (!context || !_finite(radius) || !finite_vector(context->reference_override ? context->reference_override : reference))
        return 0;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)&context->world_to_model)[i]))
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
        if (light->type == 2 && !finite_vector(&light->direction))
            return 0;
        if (light->type == 3 || light->type == 4)
        {
            if (!finite_vector(&light->position) || !_finite(light->range) || !_finite(light->intensity))
                return 0;
            if (light->type == 4 && (!finite_vector(&light->direction) || !_finite(light->inner_cos) || !_finite(light->outer_cos)))
                return 0;
        }
    }
    return 1;
}
int bh_light_prepare(BHLightContext *context, const BHVec3 *reference, float radius)
{
    BHLight *light;
    BHVec3 delta;
    float distance, attenuation, alignment;
    double z, reciprocal, retained;
    if (!valid(context, reference, radius))
        return 0;
    if (context->reference_override)
    {
        reference = context->reference_override;
        bh_mat34_transform_point(&context->world_to_model, reference, &context->local_reference);
    }
    for (light = context->lights; light; light = light->next)
    {
        if (light->type == 2)
        {
            bh_mat34_transform_direction(&context->world_to_model, &light->direction, &light->local_direction);
        }
        else if (light->type == 3 || light->type == 4)
        {
            bh_round_float(&delta.x, (double)reference->x - light->position.x);
            bh_round_float(&delta.y, (double)reference->y - light->position.y);
            z = (double)reference->z - light->position.z;
            bh_round_float(&delta.z, z);
            /* Original retains the Z subtraction across its float store, then
               multiplies by the stored Z. X/Y are both reloaded float values. */
            bh_round_float(&distance, sqrt(((double)delta.x * delta.x + (double)delta.y * delta.y) + z * delta.z));
            if (light->range > 0 && (double)radius + light->range < distance)
            {
                light->prepared_type = 0;
                continue;
            }
            if ((double)distance * .125 <= radius)
            {
                bh_mat34_transform_point(&context->world_to_model, &light->position, &light->local_position);
                if (light->type == 4)
                    bh_mat34_transform_direction(&context->world_to_model, &light->direction, &light->local_direction);
                light->prepared_type = light->type;
                continue;
            }
            retained = light->range > 0 ? 1.0 - (double)distance / light->range : 1.0;
            bh_round_float(&attenuation, retained);
            if (light->type == 4)
            {
                retained = (((double)light->direction.x * delta.x + (double)light->direction.y * delta.y) +
                            (double)light->direction.z * delta.z) /
                           distance;
                bh_round_float(&alignment, retained);
                /* Outer comparison uses the retained value; inner comparison
                   and ramp reload the float, as the original instructions do. */
                if (retained < light->outer_cos)
                    attenuation = 0;
                else if (alignment < light->inner_cos)
                    bh_round_float(&attenuation,
                                   (((double)alignment - light->outer_cos) / ((double)light->inner_cos - light->outer_cos)) *
                                       attenuation);
                retained = attenuation;
            }
            if (retained <= 0)
            {
                light->prepared_type = 0;
                continue;
            }
            reciprocal = 1.0 / distance;
            bh_round_float(&delta.x, (double)delta.x * reciprocal);
            bh_round_float(&delta.y, (double)delta.y * reciprocal);
            bh_round_float(&delta.z, (double)delta.z * reciprocal);
            light->prepared_type = 2;
            bh_mat34_transform_direction(&context->world_to_model, &delta, &light->local_direction);
            bh_round_float(&light->effective_intensity, (double)light->intensity * attenuation);
        }
    }
    return 1;
}
