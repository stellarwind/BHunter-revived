#include "bh_script_sensor.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4, "sensor layout");
typedef struct SensorPacket
{
    long event;
    BHEntity *actor;
    long expected;
    unsigned char unused[28];
} SensorPacket;
static_assert(sizeof(SensorPacket) == 40 && offsetof(SensorPacket, expected) == 8, "packet layout");
static BHScriptSensorContext *context(BHEntity *entity)
{
    BHScriptSensorContext *ctx;
    memcpy(&ctx, &entity->bindings.script_sensor, 4);
    return ctx;
}
static unsigned long generation(BHEntity *entity)
{
    unsigned long n;
    memcpy(&n, &entity->bindings.sensor_generation, 4);
    return n;
}
static int live(BHEntityPool *pool, const BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(pool, entity) && entity->generation == generation;
}
static int present(BHEntityPool *pool, const BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
static const BHScriptTimerActor *record(BHScriptSensorContext *ctx, BHEntity *entity)
{
    return bh_script_timer_actor(&ctx->lifetime->timers, entity);
}
/* VC5 may reverse a comparison to a bare C0 test and thereby accept unordered.
   Explicit NaN exclusions preserve the reviewed ordered x87 branches. */
static int greater(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a > b;
}
static int greater_equal(double a, double b)
{
    return !_isnan(a) && !_isnan(b) && a >= b;
}
int bh_script_sensor_read(BHEntity *entity, BHScriptSensorState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->actor, &entity->sensor.actor, 4);
    memcpy(&state->script_loaded, &entity->sensor.script_loaded, 4);
    memcpy(&state->remaining, &entity->sensor.remaining, 4);
    memcpy(&state->threshold, &entity->sensor.threshold, 4);
    memcpy(&state->broadcast_radius, &entity->sensor.broadcast_radius, 4);
    memcpy(&state->expected, &entity->sensor.expected, 4);
    memcpy(&state->countdown, &entity->sensor.countdown, 4);
    memcpy(&state->latch, &entity->sensor.latch, 4);
    return 1;
}
float bh_script_sensor_damage(const BHEntity *entity)
{
    float value = 0;
    if (entity)
        memcpy(&value, &entity->placement.damage, 4);
    return value;
}
void bh_script_sensor_set_damage(BHEntity *entity, float value)
{
    if (entity)
        memcpy(&entity->placement.damage, &value, 4);
}
static int valid(BHScriptSensorContext *ctx, BHEntity *entity, const BHScriptSensorState *state, int bound)
{
    unsigned long g;
    BHEntityPool *pool;
    if (!ctx || !ctx->lifetime || !ctx->lifetime->timers.pool || !entity || !state ||
        (ctx->radar_count && !ctx->radar) || ctx->radar_count > 65535)
    {
        if (ctx)
            ctx->error = BH_SENSOR_ARGUMENT;
        return 0;
    }
    pool = ctx->lifetime->timers.pool;
    g = bound ? generation(entity) : entity->generation;
    if (!live(pool, entity, g) || !record(ctx, state->actor) || !present(pool, state->actor))
    {
        ctx->error = BH_SENSOR_ACTOR;
        return 0;
    }
    if (!bh_math_environment_supported())
    {
        ctx->error = BH_SENSOR_NUMERICAL;
        return 0;
    }
    ctx->error = BH_SENSOR_OK;
    return 1;
}
int bh_script_sensor_bind(BHScriptSensorContext *ctx, BHEntity *entity, const BHScriptSensorState *state, int damage)
{
    BHScriptLifetimeState base;
    if (!valid(ctx, entity, state, 0))
        return 0;
    bh_script_lifetime_read(entity, &base);
    base.actor = state->actor;
    base.script_loaded = state->script_loaded;
    memcpy(&base.sound, &state->remaining, 4);
    base.countdown = state->countdown;
    if (!bh_script_lifetime_bind(ctx->lifetime, entity, &base, bh_script_sensor_query))
    {
        ctx->error = BH_SENSOR_ARGUMENT;
        return 0;
    }
    memcpy(&entity->sensor.threshold, &state->threshold, 4);
    memcpy(&entity->sensor.broadcast_radius, &state->broadcast_radius, 4);
    memcpy(&entity->sensor.expected, &state->expected, 4);
    memcpy(&entity->sensor.latch, &state->latch, 4);
    memcpy(&entity->bindings.script_sensor, &ctx, 4);
    memcpy(&entity->bindings.sensor_generation, &entity->generation, 4);
    entity->step = damage ? bh_script_damage_tick : bh_script_proximity_tick;
    return 1;
}
/* FST of the subtraction precedes the branch but the add-period uses the
   retained, unrounded x87 result. Unordered follows the due branch. */
static int due(BHEntity *entity, float old, double period)
{
    double value = (double)old - (1.0 / 30.0);
    float next;
    bh_round_float(&next, value);
    memcpy(&entity->sensor.countdown, &next, 4);
    if (greater(value, 0))
        return 0;
    bh_round_float(&next, value + period);
    memcpy(&entity->sensor.countdown, &next, 4);
    return 1;
}
static int broadcast(BHScriptSensorContext *ctx, BHEntity *entity, BHEntity *actor, long radius, long expected,
                     long event)
{
    SensorPacket packet;
    float r;
    unsigned long g = generation(entity);
    int result;
    BHEntity *after;
    memset(&packet, 0, sizeof(packet));
    packet.event = event;
    packet.actor = actor;
    packet.expected = expected;
    bh_round_float(&r, (double)radius);
    result = bh_entities_query(ctx->lifetime->timers.pool, 0, 0, event == 17 ? NULL : &actor->position,
                               event == 17 ? 0 : r, &packet);
    if (!live(ctx->lifetime->timers.pool, entity, g))
    {
        ctx->error = BH_SENSOR_MUTATION;
        return 0;
    }
    memcpy(&after, &entity->sensor.actor, 4);
    if (after != actor)
    {
        ctx->error = BH_SENSOR_MUTATION;
        return 0;
    }
    if (!result)
    {
        ctx->error = BH_SENSOR_QUERY;
        return 0;
    }
    return 1;
}
static int decrease(BHScriptSensorContext *ctx, BHEntity *entity, int final_event)
{
    long remaining;
    unsigned long bits;
    memcpy(&remaining, &entity->sensor.remaining, 4);
    if (!remaining)
        return 0;
    bits = (unsigned long)remaining - 1UL;
    memcpy(&remaining, &bits, 4);
    memcpy(&entity->sensor.remaining, &remaining, 4);
    if (remaining)
        return 0;
    if (final_event)
    {
        BHEntity *actor;
        memcpy(&actor, &entity->sensor.actor, 4);
        if (!broadcast(ctx, entity, actor, 0, 0, 17))
            return 1;
    }
    bh_entity_release(ctx->lifetime->timers.pool, entity);
    return 1;
}
static int in_box(const BHEntity *entity, const BHEntity *p)
{
    BHVec3 low, high;
    memcpy(&low, (char *)&entity->placement.minimum, 12);
    memcpy(&high, (char *)&entity->placement.maximum, 12);
    /* Retain the original unequal C0 versus C0|C3 tests, including asymmetric
    unordered bound behavior. Ordinary six ordered <= comparisons differ. */
    return greater_equal(p->position.x, low.x) && !greater(p->position.x, high.x) && !greater(low.y, p->position.y) &&
           greater_equal(high.y, p->position.y) && !greater(low.z, p->position.z) &&
           greater_equal(high.z, p->position.z);
}
static int nearby(BHEntity *entity, BHEntity *b, long threshold)
{
    double x = (double)entity->position.x - b->position.x, y = (double)entity->position.y - b->position.y,
           z = (double)entity->position.z - b->position.z;
    float rx, rz;
    bh_round_float(&rx, x);
    bh_round_float(&rz, z);
    return greater((double)threshold, (x * rx + y * y) + z * rz);
}
void bh_script_proximity_tick(BHEntity *entity)
{
    BHScriptSensorContext *ctx = context(entity);
    BHScriptSensorState state;
    unsigned long i;
    const BHScriptTimerActor *owner;
    BHEntity *bounds;
    float latch = 0;
    bh_script_sensor_read(entity, &state);
    if (!valid(ctx, entity, &state, 1))
        return;
    if (!due(entity, state.countdown, .25))
        return;
    for (i = 0; i < ctx->radar_count; ++i)
    {
        BHEntity *p = ctx->radar[i];
        int inside;
        if (p != ctx->player)
            continue;
        if (!p || !present(ctx->lifetime->timers.pool, p))
        {
            ctx->error = BH_SENSOR_ACTOR;
            return;
        }
        memcpy(&state.threshold, &entity->sensor.threshold, 4);
        memcpy(&state.broadcast_radius, &entity->sensor.broadcast_radius, 4);
        memcpy(&state.expected, &entity->sensor.expected, 4);
        if (state.threshold > 0)
            inside = nearby(p, state.actor, state.threshold);
        else
        {
            owner = record(ctx, state.actor);
            if (owner->owner)
                owner = owner->owner;
            bounds = owner->entity;
            if (!bounds || !present(ctx->lifetime->timers.pool, bounds))
            {
                ctx->error = BH_SENSOR_ACTOR;
                return;
            }
            inside = in_box(bounds, p);
        }
        if (inside)
        {
            memcpy(&latch, &entity->sensor.latch, 4);
            if (greater(latch, 0))
                return;
            latch = 1;
            memcpy(&entity->sensor.latch, &latch, 4);
            if (!broadcast(ctx, entity, state.actor, state.broadcast_radius, state.expected, 14))
                return;
            if (decrease(ctx, entity, 0))
                return;
        }
    }
    latch = 0;
    memcpy(&entity->sensor.latch, &latch, 4);
}
void bh_script_damage_tick(BHEntity *entity)
{
    BHScriptSensorContext *ctx = context(entity);
    BHScriptSensorState state;
    double scaled;
    float damage, countdown, latch = 0;
    bh_script_sensor_read(entity, &state);
    if (!valid(ctx, entity, &state, 1))
        return;
    if (greater(state.latch, 0))
        bh_script_sensor_set_damage(state.actor, 0);
    if (!due(entity, state.countdown, 1))
        return;
    memcpy(&entity->sensor.latch, &latch, 4);
    scaled = (double)bh_script_sensor_damage(state.actor) * .98;
    bh_round_float(&damage, scaled);
    bh_script_sensor_set_damage(state.actor, damage);
    if (!greater(scaled, (double)state.threshold))
        return;
    if (!broadcast(ctx, entity, state.actor, state.broadcast_radius, state.expected, 14))
        return;
    memcpy(&countdown, &entity->sensor.countdown, 4);
    bh_round_float(&countdown, (double)countdown + 4);
    memcpy(&entity->sensor.countdown, &countdown, 4);
    latch = 1;
    memcpy(&entity->sensor.latch, &latch, 4);
    bh_script_sensor_set_damage(state.actor, 0);
    decrease(ctx, entity, 1);
}
