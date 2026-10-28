#include "bh_trail.h"
#include "bh_fp.h"
#include "bh_round.h"
#include "bh_callback_helpers.h"
#include <string.h>
#include <math.h>
#include <float.h>
static void get(const BHEntity *e, unsigned long o, void *p, unsigned long n)
{
    memcpy(p, (const unsigned char *)e + o, n);
}
static void put(BHEntity *e, unsigned long o, const void *p, unsigned long n)
{
    memcpy((unsigned char *)e + o, p, n);
}
static float scalar(const BHEntity *e, unsigned long o)
{
    float f;
    get(e, o, &f, 4);
    return f;
}
static void number(BHEntity *e, unsigned long o, double v)
{
    float f;
    bh_round_float(&f, v);
    put(e, o, &f, 4);
}
static BHEntity *link(const BHEntity *e, unsigned long o)
{
    BHEntity *p;
    get(e, o, &p, 4);
    return p;
}
static BHTrailContext *context(BHEntity *e)
{
    BHTrailContext *p;
    memcpy(&p, e->payload + 600, 4);
    return p;
}
static void bind_context(BHEntity *e, BHTrailContext *c)
{
    memcpy(e->payload + 600, &c, 4);
}
static int live(BHEntityPool *p, BHEntity *e)
{
    return bh_entity_is_active(p, e);
}
int __cdecl bh_trail_bind(BHTrailContext *c, BHEntity *e, const BHTrailParameters *p)
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
    put(e, 0xe8, &generation, 4);
    put(e, 0xec, &p->bucket, 4);
    put(e, 0xf0, &p->point_word, 4);
    put(e, 0xf4, &p->attached, 4);
    put(e, 0xf8, &zero, 4);
    put(e, 0xfc, &p->lifetime, 4);
    put(e, 0x100, &p->decay, 4);
    put(e, 0x104, &p->interval, 4);
    put(e, 0x108, &p->node_lifetime, 4);
    put(e, 0x10c, &p->width, 4);
    number(e, 0x110, 0);
    put(e, 0x114, &p->width_step, 4);
    put(e, 0x118, &p->texture_scale, 4);
    put(e, 0x11c, &p->image, 4);
    put(e, 0x120, &p->drift, 12);
    bind_context(e, c);
    e->step = bh_trail_generate;
    e->refresh = NULL;
    return 1;
}
BHEntity *__cdecl bh_trail_create(BHTrailContext *c, unsigned long image, long bucket, unsigned long point_word,
                                  const BHVec3 *position, BHEntity *attached, float lifetime, float fade_seconds,
                                  float frequency, float node_value, float width, float width_rate,
                                  const BHVec3 *velocity, float texture_scale)
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
unsigned long __cdecl bh_trail_create_pair(BHTrailContext *c, unsigned long image, long bucket,
                                           unsigned long point_word, const BHVec3 *position, BHEntity *attached,
                                           float lifetime, float fade_seconds, float frequency, float node_value,
                                           float width, float width_rate, const BHVec3 *velocity, float texture_scale,
                                           BHEntity **first, BHEntity **second)
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
void __cdecl bh_trail_node_tick(BHEntity *e)
{
    BHTrailContext *c = context(e);
    BHEntity *older, *newer;
    BHVec3 drift;
    double next;
    if (!c || !bh_math_environment_supported())
        return;
    next = (double)scalar(e, 0xf0) - scalar(e, 0x100);
    number(e, 0xf0, next);
    if (next < 0)
    {
        older = link(e, 0xec);
        newer = link(e, 0xe8);
        if (older)
            put(older, 0xe8, &newer, 4);
        if (newer)
            put(newer, 0xec, &older, 4);
        bh_entity_release(c->pool, e);
        return;
    }
    get(e, 0x114, &drift, 12);
    number(e, 0xc, (double)drift.x + e->position.x);
    number(e, 0x10, (double)drift.y + e->position.y);
    number(e, 0x14, (double)drift.z + e->position.z);
    number(e, 0xf4, (double)scalar(e, 0x104) + scalar(e, 0xf4));
}
void __cdecl bh_trail_refresh(BHEntity *e, void *unused)
{
    BHTrailContext *c = context(e);
    BHEntity *node;
    BHDrawCommand *header, *point;
    unsigned long bucket, image, count = 0, n = 0;
    float zero = 0;
    (void)unused;
    if (!c || !link(e, 0xec))
        return;
    get(e, 0x108, &bucket, 4);
    header = bh_queue_reserve(c->queue, (long)bucket);
    if (!header)
        return;
    get(e, 0x110, &image, 4);
    header->resource = image;
    for (node = e; node && n++ < c->pool->capacity; node = link(node, 0xec))
    {
        point = bh_queue_reserve(c->queue, (long)bucket);
        if (!point)
            continue;
        point->kind = BH_DRAW_PATH_POINT;
        memcpy(&point->resource, &node->position, 12);
        get(node, 0xf4, point->words + 2, 4);
        get(node, 0xf8, point->words + 4, 4);
        get(node, 0xfc, point->words + 5, 4);
        get(node, 0x10c, point->words + 6, 4);
        if (count && link(node, 0xec))
            get(node, 0xf0, point->words + 3, 4);
        else
            memcpy(point->words + 3, &zero, 4);
        ++count;
    }
    header->words[0] = count;
    header->kind = BH_DRAW_PATH_HEADER;
}
void __cdecl bh_trail_generate(BHEntity *e)
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
    lifetime = scalar(e, 0xfc);
    next = lifetime - (1.0 / 30.0);
    number(e, 0xfc, next);
    if (lifetime > 0 && !(next > 0))
    {
        bh_entity_release(c->pool, e);
        return;
    }
    next = (double)scalar(e, 0x110) - (1.0 / 30.0);
    number(e, 0x110, next);
    if (next > 0)
        return;
    number(e, 0x110, (double)scalar(e, 0x104) + next);
    attached = link(e, 0xf4);
    get(e, 0xe8, &generation, 4);
    if (attached && attached->generation != generation)
    {
        attached = NULL;
        put(e, 0xf4, &attached, 4);
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
    older = link(e, 0xf8);
    put(node, 0xec, &older, 4);
    attached = NULL;
    put(node, 0xe8, &attached, 4);
    if (older)
    {
        put(older, 0xe8, &node, 4);
        older->refresh = NULL;
    }
    put(e, 0xf8, &node, 4);
    node->position = e->position;
    if (older)
    {
        dx = (double)older->position.x - node->position.x;
        dy = (double)older->position.y - node->position.y;
        dz = (double)older->position.z - node->position.z;
        bh_round_float(&stored_x, dx);
        bh_round_float(&stored_z, dz);
        length = sqrt((dx * stored_x + dy * dy) + dz * stored_z);
        number(node, 0xfc, (double)scalar(older, 0xfc) + length);
    }
    else
        number(node, 0xfc, 0);
    number(node, 0xf0, scalar(e, 0x108));
    number(node, 0x100, scalar(e, 0x100));
    lifetime = scalar(e, 0xfc);
    if (lifetime > 0 && lifetime < 4)
    {
        number(node, 0xf0, (double)scalar(node, 0xf0) * lifetime * .25f);
        number(node, 0x100, (double)scalar(node, 0x100) * lifetime * .25f);
    }
    number(node, 0xf4, scalar(e, 0x10c));
    number(node, 0x104, scalar(e, 0x114));
    get(e, 0xf0, &generation, 4);
    put(node, 0x10c, &generation, 4);
    get(e, 0xec, &generation, 4);
    put(node, 0x108, &generation, 4);
    get(e, 0x11c, &generation, 4);
    put(node, 0x110, &generation, 4);
    get(e, 0x120, &drift, 12);
    put(node, 0x114, &drift, 12);
    number(node, 0xf8, (double)scalar(e, 0x118) * scalar(e, 0xfc));
    bind_context(node, c);
    node->refresh = bh_trail_refresh;
    node->step = bh_trail_node_tick;
    ++c->emitted;
}
