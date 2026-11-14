#include "../Entities/bh_field_access.h"
#include "bh_trail.h"
#ifdef BH_SDL_BACKEND
#include "../../Graphics/bh_render_interpolation.h"
#endif
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include "../../Core/bh_callback_helpers.h"
#include <string.h>
#include <math.h>
#include <float.h>
static void trail_number(float *field, double value)
{
    float stored;
    bh_round_float(&stored, value);
    memcpy(field, &stored, sizeof(stored));
}
static BHTrailContext *context(BHEntity *entity)
{
    BHTrailContext *trail_context;
    memcpy(&trail_context, &entity->bindings.trail, 4);
    return trail_context;
}
static void bind_context(BHEntity *entity, BHTrailContext *trail_context)
{
    memcpy(&entity->bindings.trail, &trail_context, 4);
}
static int live(BHEntityPool *pool, BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
int bh_trail_bind(BHTrailContext *trail_context, BHEntity *emitter, const BHTrailParameters *parameters)
{
    BHEntity *no_node = NULL;
    unsigned long i, generation = 0;
    const float *parameter_values;
    if (!trail_context || !trail_context->pool || !trail_context->queue || !emitter || !parameters ||
        !live(trail_context->pool, emitter) || !trail_context->queue->commands || !bh_math_environment_supported())
        return 0;
    parameter_values = &parameters->lifetime;
    for (i = 0; i < 10; ++i)
        if (!_finite(parameter_values[i]))
            return 0;
    if (parameters->attached)
    {
        if (!live(trail_context->pool, parameters->attached))
            return 0;
        generation = parameters->attached->generation;
    }
    memcpy(&emitter->trail.attachment_generation, &generation, 4);
    memcpy(&emitter->trail.bucket, &parameters->bucket, 4);
    memcpy(&emitter->trail.point_word, &parameters->point_word, 4);
    memcpy(&emitter->trail.attached, &parameters->attached, 4);
    memcpy(&emitter->trail.newest, &no_node, 4);
    memcpy(&emitter->trail.lifetime, &parameters->lifetime, 4);
    memcpy(&emitter->trail.decay, &parameters->decay, 4);
    memcpy(&emitter->trail.interval, &parameters->interval, 4);
    memcpy(&emitter->trail.node_lifetime, &parameters->node_lifetime, 4);
    memcpy(&emitter->trail.width, &parameters->width, 4);
    trail_number(&emitter->trail.countdown, 0);
    memcpy(&emitter->trail.width_step, &parameters->width_step, 4);
    memcpy(&emitter->trail.texture_scale, &parameters->texture_scale, 4);
    memcpy(&emitter->trail.image, &parameters->image, 4);
    memcpy(&emitter->trail.drift, &parameters->drift, 12);
    bind_context(emitter, trail_context);
    emitter->step = bh_trail_generate;
    emitter->refresh = NULL;
    return 1;
}
BHEntity *bh_trail_create(BHTrailContext *trail_context, unsigned long image, long bucket, unsigned long point_word,
                          const BHVec3 *position, BHEntity *attached, float lifetime, float fade_seconds,
                          float frequency, float node_value, float width, float width_rate, const BHVec3 *velocity,
                          float texture_scale)
{
    BHEntity *emitter;
    BHTrailParameters parameters;
    if (!trail_context || !position || !velocity || !trail_context->pool || !trail_context->queue ||
        !(fade_seconds > 0) || !(frequency > 0) || !_finite(fade_seconds) || !_finite(frequency) ||
        !_finite(position->x) || !_finite(position->y) || !_finite(position->z) || !bh_math_environment_supported())
        return NULL;
    memset(&parameters, 0, sizeof(parameters));
    parameters.bucket = (unsigned long)bucket;
    parameters.image = image;
    parameters.point_word = point_word;
    parameters.attached = attached;
    parameters.lifetime = lifetime;
    parameters.node_lifetime = node_value;
    parameters.width = width;
    parameters.texture_scale = texture_scale;
    bh_round_float(&parameters.interval, 1.0 / (double)frequency);
    bh_round_float(&parameters.decay, ((double)node_value / fade_seconds) * (1.0 / 30.0));
    bh_round_float(&parameters.width_step, (double)width_rate * (1.0 / 30.0));
    bh_round_float(&parameters.drift.x, (double)velocity->x * (1.0 / 30.0));
    bh_round_float(&parameters.drift.y, (double)velocity->y * (1.0 / 30.0));
    bh_round_float(&parameters.drift.z, (double)velocity->z * (1.0 / 30.0));
    emitter = bh_entity_acquire(trail_context->pool);
    if (!emitter)
        return NULL;
    if (!bh_trail_bind(trail_context, emitter, &parameters))
    {
        bh_entity_release(trail_context->pool, emitter);
        return NULL;
    }
    emitter->position = *position;
    if (!bh_entity_set_query(trail_context->pool, emitter, bh_effect_target_removed))
    {
        bh_entity_release(trail_context->pool, emitter);
        return NULL;
    }
    return emitter;
}
unsigned long bh_trail_create_pair(BHTrailContext *trail_context, unsigned long image, long bucket,
                                   unsigned long point_word, const BHVec3 *position, BHEntity *attached, float lifetime,
                                   float fade_seconds, float frequency, float node_value, float width, float width_rate,
                                   const BHVec3 *velocity, float texture_scale, BHEntity **first, BHEntity **second)
{
    BHEntity *first_emitter, *second_emitter;
    BHVec3 scaled_velocity;
    float paired_node_value, second_fade_seconds, second_width_rate;
    if (!velocity || !bh_math_environment_supported())
        return 0;
    bh_round_float(&paired_node_value, (double)node_value * 0.7092198581560284);
    first_emitter =
        bh_trail_create(trail_context, image, bucket, point_word ^ 2, position, attached, lifetime, fade_seconds,
                        frequency, paired_node_value, width, width_rate, velocity, texture_scale);
    bh_round_float(&scaled_velocity.x, (double)velocity->x * 1.7);
    bh_round_float(&scaled_velocity.y, (double)velocity->y * 1.7);
    bh_round_float(&scaled_velocity.z, (double)velocity->z * 1.7);
    bh_round_float(&second_fade_seconds, (double)fade_seconds * 0.5882352941176471);
    bh_round_float(&second_width_rate, (double)width_rate * 1.7);
    second_emitter =
        bh_trail_create(trail_context, image, bucket, point_word, position, attached, lifetime, second_fade_seconds,
                        frequency, paired_node_value, width, second_width_rate, &scaled_velocity, texture_scale);
    if (first)
        *first = first_emitter;
    if (second)
        *second = second_emitter;
    return (first_emitter ? 1 : 0) + (second_emitter ? 1 : 0);
}
void bh_trail_node_tick(BHEntity *node)
{
    BHTrailContext *trail_context = context(node);
    BHEntity *older, *newer;
    BHVec3 drift;
    double next_alpha;
    if (!trail_context || !bh_math_environment_supported())
        return;
    next_alpha = (double)bh_field_f32(&node->trail_node.alpha) - bh_field_f32(&node->trail_node.decay);
    trail_number(&node->trail_node.alpha, next_alpha);
    if (next_alpha < 0)
    {
        older = node->trail_node.older;
        newer = node->trail_node.newer;
        if (older)
            memcpy(&older->trail_node.newer, &newer, 4);
        if (newer)
            memcpy(&newer->trail_node.older, &older, 4);
        bh_entity_release(trail_context->pool, node);
        return;
    }
    memcpy(&drift, &node->trail_node.drift, 12);
    trail_number(&node->position.x, (double)drift.x + node->position.x);
    trail_number(&node->position.y, (double)drift.y + node->position.y);
    trail_number(&node->position.z, (double)drift.z + node->position.z);
    trail_number(&node->trail_node.width,
                 (double)bh_field_f32(&node->trail_node.width_step) + bh_field_f32(&node->trail_node.width));
}
void bh_trail_refresh(BHEntity *newest, void *unused)
{
    BHTrailContext *trail_context = context(newest);
    BHEntity *node;
    BHDrawCommand *header, *point;
    unsigned long bucket, image, count = 0, nodes_visited = 0;
    float zero = 0;
    (void)unused;
    if (!trail_context || !newest->trail_node.older)
        return;
    memcpy(&bucket, &newest->trail_node.bucket, 4);
    header = bh_queue_reserve(trail_context->queue, (long)bucket);
    if (!header)
        return;
    memcpy(&image, &newest->trail_node.image, 4);
    header->resource = image;
    for (node = newest; node && nodes_visited++ < trail_context->pool->capacity; node = node->trail_node.older)
    {
        point = bh_queue_reserve(trail_context->queue, (long)bucket);
        if (!point)
            continue;
        point->kind = BH_DRAW_PATH_POINT;
        memcpy(&point->resource, &node->position, 12);
        memcpy(point->words + 2, &node->trail_node.width, 4);
        memcpy(point->words + 4, &node->trail_node.texture_phase, 4);
        memcpy(point->words + 5, &node->trail_node.distance, 4);
        memcpy(point->words + 6, &node->trail_node.point_word, 4);
        if (count && node->trail_node.older)
            memcpy(point->words + 3, &node->trail_node.alpha, 4);
        else
            memcpy(point->words + 3, &zero, 4);
#ifdef BH_SDL_BACKEND
        {
            BHEntity *older = node->trail_node.older;
            bh_interpolation_trail_point((unsigned long)(point - trail_context->queue->commands), (unsigned long)node,
                                         node->generation, (unsigned long)older, older ? older->generation : 0, image);
        }
#endif
        ++count;
    }
    header->words[0] = count;
    header->kind = BH_DRAW_PATH_HEADER;
}
void bh_trail_generate(BHEntity *emitter)
{
    BHTrailContext *trail_context = context(emitter);
    BHEntity *attached, *older, *node;
    BHEntityPositionPacket packet;
    double remaining_time, lifetime, dx, dy, dz, length;
    float stored_x, stored_z;
    unsigned long generation;
    BHVec3 drift;
    if (!trail_context || !bh_math_environment_supported())
        return;
    lifetime = bh_field_f32(&emitter->trail.lifetime);
    remaining_time = lifetime - (1.0 / 30.0);
    trail_number(&emitter->trail.lifetime, remaining_time);
    if (lifetime > 0 && !(remaining_time > 0))
    {
        bh_entity_release(trail_context->pool, emitter);
        return;
    }
    remaining_time = (double)bh_field_f32(&emitter->trail.countdown) - (1.0 / 30.0);
    trail_number(&emitter->trail.countdown, remaining_time);
    if (remaining_time > 0)
        return;
    trail_number(&emitter->trail.countdown, (double)bh_field_f32(&emitter->trail.interval) + remaining_time);
    attached = emitter->trail.attached;
    memcpy(&generation, &emitter->trail.attachment_generation, 4);
    if (attached && attached->generation != generation)
    {
        attached = NULL;
        memcpy(&emitter->trail.attached, &attached, 4);
    }
    if (attached)
    {
        packet.event = 3;
        packet.position = emitter->position;
        if (attached->query)
            attached->query(attached, &packet);
        emitter->position = packet.position;
    }
    node = bh_entity_acquire(trail_context->pool);
    if (!node)
    {
        ++trail_context->failures;
        return;
    }
    older = emitter->trail.newest;
    memcpy(&node->trail_node.older, &older, 4);
    attached = NULL;
    memcpy(&node->trail_node.newer, &attached, 4);
    if (older)
    {
        memcpy(&older->trail_node.newer, &node, 4);
        older->refresh = NULL;
    }
    memcpy(&emitter->trail.newest, &node, 4);
    node->position = emitter->position;
    if (older)
    {
        dx = (double)older->position.x - node->position.x;
        dy = (double)older->position.y - node->position.y;
        dz = (double)older->position.z - node->position.z;
        bh_round_float(&stored_x, dx);
        bh_round_float(&stored_z, dz);
        length = sqrt((dx * stored_x + dy * dy) + dz * stored_z);
        trail_number(&node->trail_node.distance, (double)bh_field_f32(&older->trail_node.distance) + length);
    }
    else
        trail_number(&node->trail_node.distance, 0);
    trail_number(&node->trail_node.alpha, bh_field_f32(&emitter->trail.node_lifetime));
    trail_number(&node->trail_node.decay, bh_field_f32(&emitter->trail.decay));
    lifetime = bh_field_f32(&emitter->trail.lifetime);
    if (lifetime > 0 && lifetime < 4)
    {
        trail_number(&node->trail_node.alpha, (double)bh_field_f32(&node->trail_node.alpha) * lifetime * .25f);
        trail_number(&node->trail_node.decay, (double)bh_field_f32(&node->trail_node.decay) * lifetime * .25f);
    }
    trail_number(&node->trail_node.width, bh_field_f32(&emitter->trail.width));
    trail_number(&node->trail_node.width_step, bh_field_f32(&emitter->trail.width_step));
    memcpy(&generation, &emitter->trail.point_word, 4);
    memcpy(&node->trail_node.point_word, &generation, 4);
    memcpy(&generation, &emitter->trail.bucket, 4);
    memcpy(&node->trail_node.bucket, &generation, 4);
    memcpy(&generation, &emitter->trail.image, 4);
    memcpy(&node->trail_node.image, &generation, 4);
    memcpy(&drift, &emitter->trail.drift, 12);
    memcpy(&node->trail_node.drift, &drift, 12);
    trail_number(&node->trail_node.texture_phase,
                 (double)bh_field_f32(&emitter->trail.texture_scale) * bh_field_f32(&emitter->trail.lifetime));
    bind_context(node, trail_context);
    node->refresh = bh_trail_refresh;
    node->step = bh_trail_node_tick;
    ++trail_context->emitted;
}
