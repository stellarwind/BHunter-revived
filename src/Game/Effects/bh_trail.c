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
static BHTrailContext *context(BHEntity *e)
{
    BHTrailContext *p;
    memcpy(&p, &e->bindings.trail, 4);
    return p;
}
static void bind_context(BHEntity *e, BHTrailContext *c)
{
    memcpy(&e->bindings.trail, &c, 4);
}
static int live(BHEntityPool *p, BHEntity *e)
{
    return bh_entity_is_active(p, e);
}
int bh_trail_bind(BHTrailContext *c, BHEntity *e, const BHTrailParameters *p)
{
    BHEntity *zero = NULL;
    unsigned long i, generation = 0;
    const float *values;
    if (!c || !c->pool || !c->queue || !e || !p || !live(c->pool, e) || !c->queue->commands ||
        !bh_math_environment_supported())
        return 0;
    values = &p->lifetime;
    for (i = 0; i < 10; ++i)
        if (!_finite(values[i]))
            return 0;
    if (p->attached)
    {
        if (!live(c->pool, p->attached))
            return 0;
        generation = p->attached->generation;
    }
    memcpy(&e->trail.attachment_generation, &generation, 4);
    memcpy(&e->trail.bucket, &p->bucket, 4);
    memcpy(&e->trail.point_word, &p->point_word, 4);
    memcpy(&e->trail.attached, &p->attached, 4);
    memcpy(&e->trail.newest, &zero, 4);
    memcpy(&e->trail.lifetime, &p->lifetime, 4);
    memcpy(&e->trail.decay, &p->decay, 4);
    memcpy(&e->trail.interval, &p->interval, 4);
    memcpy(&e->trail.node_lifetime, &p->node_lifetime, 4);
    memcpy(&e->trail.width, &p->width, 4);
    trail_number(&e->trail.countdown, 0);
    memcpy(&e->trail.width_step, &p->width_step, 4);
    memcpy(&e->trail.texture_scale, &p->texture_scale, 4);
    memcpy(&e->trail.image, &p->image, 4);
    memcpy(&e->trail.drift, &p->drift, 12);
    bind_context(e, c);
    e->step = bh_trail_generate;
    e->refresh = NULL;
    return 1;
}
BHEntity *bh_trail_create(BHTrailContext *c, unsigned long image, long bucket, unsigned long point_word,
                          const BHVec3 *position, BHEntity *attached, float lifetime, float fade_seconds,
                          float frequency, float node_value, float width, float width_rate, const BHVec3 *velocity,
                          float texture_scale)
{
    BHEntity *e;
    BHTrailParameters p;
    if (!c || !position || !velocity || !c->pool || !c->queue || !(fade_seconds > 0) || !(frequency > 0) ||
        !_finite(fade_seconds) || !_finite(frequency) || !_finite(position->x) || !_finite(position->y) ||
        !_finite(position->z) || !bh_math_environment_supported())
        return NULL;
    memset(&p, 0, sizeof(p));
    p.bucket = (unsigned long)bucket;
    p.image = image;
    p.point_word = point_word;
    p.attached = attached;
    p.lifetime = lifetime;
    p.node_lifetime = node_value;
    p.width = width;
    p.texture_scale = texture_scale;
    bh_round_float(&p.interval, 1.0 / (double)frequency);
    bh_round_float(&p.decay, ((double)node_value / fade_seconds) * (1.0 / 30.0));
    bh_round_float(&p.width_step, (double)width_rate * (1.0 / 30.0));
    bh_round_float(&p.drift.x, (double)velocity->x * (1.0 / 30.0));
    bh_round_float(&p.drift.y, (double)velocity->y * (1.0 / 30.0));
    bh_round_float(&p.drift.z, (double)velocity->z * (1.0 / 30.0));
    e = bh_entity_acquire(c->pool);
    if (!e)
        return NULL;
    if (!bh_trail_bind(c, e, &p))
    {
        bh_entity_release(c->pool, e);
        return NULL;
    }
    e->position = *position;
    if (!bh_entity_set_query(c->pool, e, bh_effect_target_removed))
    {
        bh_entity_release(c->pool, e);
        return NULL;
    }
    return e;
}
unsigned long bh_trail_create_pair(BHTrailContext *c, unsigned long image, long bucket, unsigned long point_word,
                                   const BHVec3 *position, BHEntity *attached, float lifetime, float fade_seconds,
                                   float frequency, float node_value, float width, float width_rate,
                                   const BHVec3 *velocity, float texture_scale, BHEntity **first, BHEntity **second)
{
    BHEntity *a, *b;
    BHVec3 scaled;
    float value, fade, rate;
    if (!velocity || !bh_math_environment_supported())
        return 0;
    bh_round_float(&value, (double)node_value * 0.7092198581560284);
    a = bh_trail_create(c, image, bucket, point_word ^ 2, position, attached, lifetime, fade_seconds, frequency, value,
                        width, width_rate, velocity, texture_scale);
    bh_round_float(&scaled.x, (double)velocity->x * 1.7);
    bh_round_float(&scaled.y, (double)velocity->y * 1.7);
    bh_round_float(&scaled.z, (double)velocity->z * 1.7);
    bh_round_float(&fade, (double)fade_seconds * 0.5882352941176471);
    bh_round_float(&rate, (double)width_rate * 1.7);
    b = bh_trail_create(c, image, bucket, point_word, position, attached, lifetime, fade, frequency, value, width, rate,
                        &scaled, texture_scale);
    if (first)
        *first = a;
    if (second)
        *second = b;
    return (a ? 1 : 0) + (b ? 1 : 0);
}
void bh_trail_node_tick(BHEntity *e)
{
    BHTrailContext *c = context(e);
    BHEntity *older, *newer;
    BHVec3 drift;
    double next;
    if (!c || !bh_math_environment_supported())
        return;
    next = (double)bh_field_f32(&e->trail_node.alpha) - bh_field_f32(&e->trail_node.decay);
    trail_number(&e->trail_node.alpha, next);
    if (next < 0)
    {
        older = e->trail_node.older;
        newer = e->trail_node.newer;
        if (older)
            memcpy(&older->trail_node.newer, &newer, 4);
        if (newer)
            memcpy(&newer->trail_node.older, &older, 4);
        bh_entity_release(c->pool, e);
        return;
    }
    memcpy(&drift, &e->trail_node.drift, 12);
    trail_number(&e->position.x, (double)drift.x + e->position.x);
    trail_number(&e->position.y, (double)drift.y + e->position.y);
    trail_number(&e->position.z, (double)drift.z + e->position.z);
    trail_number(&e->trail_node.width,
                 (double)bh_field_f32(&e->trail_node.width_step) + bh_field_f32(&e->trail_node.width));
}
void bh_trail_refresh(BHEntity *e, void *unused)
{
    BHTrailContext *c = context(e);
    BHEntity *node;
    BHDrawCommand *header, *point;
    unsigned long bucket, image, count = 0, n = 0;
    float zero = 0;
    (void)unused;
    if (!c || !e->trail_node.older)
        return;
    memcpy(&bucket, &e->trail_node.bucket, 4);
    header = bh_queue_reserve(c->queue, (long)bucket);
    if (!header)
        return;
    memcpy(&image, &e->trail_node.image, 4);
    header->resource = image;
    for (node = e; node && n++ < c->pool->capacity; node = node->trail_node.older)
    {
        point = bh_queue_reserve(c->queue, (long)bucket);
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
            bh_interpolation_trail_point((unsigned long)(point - c->queue->commands), (unsigned long)node,
                                         node->generation, (unsigned long)older, older ? older->generation : 0, image);
        }
#endif
        ++count;
    }
    header->words[0] = count;
    header->kind = BH_DRAW_PATH_HEADER;
}
void bh_trail_generate(BHEntity *e)
{
    BHTrailContext *c = context(e);
    BHEntity *attached, *older, *node;
    BHEntityPositionPacket packet;
    double next, lifetime, dx, dy, dz, length;
    float stored_x, stored_z;
    unsigned long generation;
    BHVec3 drift;
    if (!c || !bh_math_environment_supported())
        return;
    lifetime = bh_field_f32(&e->trail.lifetime);
    next = lifetime - (1.0 / 30.0);
    trail_number(&e->trail.lifetime, next);
    if (lifetime > 0 && !(next > 0))
    {
        bh_entity_release(c->pool, e);
        return;
    }
    next = (double)bh_field_f32(&e->trail.countdown) - (1.0 / 30.0);
    trail_number(&e->trail.countdown, next);
    if (next > 0)
        return;
    trail_number(&e->trail.countdown, (double)bh_field_f32(&e->trail.interval) + next);
    attached = e->trail.attached;
    memcpy(&generation, &e->trail.attachment_generation, 4);
    if (attached && attached->generation != generation)
    {
        attached = NULL;
        memcpy(&e->trail.attached, &attached, 4);
    }
    if (attached)
    {
        packet.event = 3;
        packet.position = e->position;
        if (attached->query)
            attached->query(attached, &packet);
        e->position = packet.position;
    }
    node = bh_entity_acquire(c->pool);
    if (!node)
    {
        ++c->failures;
        return;
    }
    older = e->trail.newest;
    memcpy(&node->trail_node.older, &older, 4);
    attached = NULL;
    memcpy(&node->trail_node.newer, &attached, 4);
    if (older)
    {
        memcpy(&older->trail_node.newer, &node, 4);
        older->refresh = NULL;
    }
    memcpy(&e->trail.newest, &node, 4);
    node->position = e->position;
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
    trail_number(&node->trail_node.alpha, bh_field_f32(&e->trail.node_lifetime));
    trail_number(&node->trail_node.decay, bh_field_f32(&e->trail.decay));
    lifetime = bh_field_f32(&e->trail.lifetime);
    if (lifetime > 0 && lifetime < 4)
    {
        trail_number(&node->trail_node.alpha, (double)bh_field_f32(&node->trail_node.alpha) * lifetime * .25f);
        trail_number(&node->trail_node.decay, (double)bh_field_f32(&node->trail_node.decay) * lifetime * .25f);
    }
    trail_number(&node->trail_node.width, bh_field_f32(&e->trail.width));
    trail_number(&node->trail_node.width_step, bh_field_f32(&e->trail.width_step));
    memcpy(&generation, &e->trail.point_word, 4);
    memcpy(&node->trail_node.point_word, &generation, 4);
    memcpy(&generation, &e->trail.bucket, 4);
    memcpy(&node->trail_node.bucket, &generation, 4);
    memcpy(&generation, &e->trail.image, 4);
    memcpy(&node->trail_node.image, &generation, 4);
    memcpy(&drift, &e->trail.drift, 12);
    memcpy(&node->trail_node.drift, &drift, 12);
    trail_number(&node->trail_node.texture_phase,
                 (double)bh_field_f32(&e->trail.texture_scale) * bh_field_f32(&e->trail.lifetime));
    bind_context(node, c);
    node->refresh = bh_trail_refresh;
    node->step = bh_trail_node_tick;
    ++c->emitted;
}
