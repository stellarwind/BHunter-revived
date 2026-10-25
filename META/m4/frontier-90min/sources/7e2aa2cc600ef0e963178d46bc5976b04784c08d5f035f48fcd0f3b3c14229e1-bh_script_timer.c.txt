#include "bh_script_timer.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char state_size[sizeof(BHScriptTimerState) == 48 ? 1 : -1];
typedef char state_offset[offsetof(BHEntity, payload) + 4 == 0xe8 ? 1 : -1];
typedef char countdown_offset[offsetof(BHScriptTimerState, countdown) == 44 ? 1 : -1];

static void __cdecl template_timer(BHEntity *entity, void *packet);
static void __cdecl template_timer2(BHEntity *entity, void *packet);
static void __cdecl running_query(BHEntity *entity, void *packet);
static void __cdecl active_tick(BHEntity *entity);
static void __cdecl always_tick(BHEntity *entity);
static void __cdecl delete_tick(BHEntity *entity);

static BHScriptTimerState *state(BHEntity *entity) {
    return (BHScriptTimerState *)(entity->payload + 4);
}
static BHScriptTimerContext *context(BHEntity *entity) {
    BHScriptTimerContext *result;
    memcpy(&result, entity->payload + 52, sizeof(result));
    return result;
}
static void bind_context(BHEntity *entity, BHScriptTimerContext *ctx) {
    memcpy(entity->payload + 52, &ctx, sizeof(ctx));
}
static int live(const BHScriptTimerContext *ctx, const BHEntity *entity, unsigned long generation) {
    BHEntity *p;
    unsigned long count = 0;
    if (!ctx || !ctx->pool)
        return 0;
    for (p = ctx->pool->active; p && count++ < ctx->pool->capacity; p = p->next)
        if (p == entity)
            return p->generation == generation;
    return 0;
}
static const BHScriptTimerActor *actor(const BHScriptTimerContext *ctx, BHEntity *entity) {
    unsigned long i;
    for (i = 0; i < ctx->actor_count; ++i)
        if (ctx->actors[i].entity == entity)
            return ctx->actors + i;
    return NULL;
}
static long signed_word(unsigned long value) {
    long result;
    memcpy(&result, &value, 4);
    return result;
}
static double random_part(long low, long high) {
    unsigned long product = (unsigned long)rand() * ((unsigned long)high - (unsigned long)low);
    return (double)signed_word(product) * (-1.0 / 32768.0);
}
int __cdecl bh_script_timers_init(BHScriptTimerContext *ctx, BHEntityPool *pool,
                                  const BHScriptTimerActor *actors, unsigned long count) {
    BHScriptTimerContext next;
    unsigned long i, j;
    if (!ctx || !pool || (count && !actors) || count > 65535 || !bh_entities_valid(pool))
        return 0;
    for (i = 0; i < count; ++i) {
        if (!actors[i].entity)
            return 0;
        for (j = 0; j < i; ++j)
            if (actors[i].entity == actors[j].entity)
                return 0;
    }
    memset(&next, 0, sizeof(next));
    next.pool = pool;
    next.actors = actors;
    next.actor_count = count;
    *ctx = next;
    return 1;
}
BHScriptTimerState *__cdecl bh_script_timer_state(BHScriptTimerContext *ctx, BHEntity *entity) {
    if (!ctx || !entity || !live(ctx, entity, entity->generation) ||
        (entity->query != template_timer && entity->query != template_timer2 &&
         entity->query != running_query) ||
        context(entity) != ctx)
        return NULL;
    return state(entity);
}
int __cdecl bh_script_timer_bind(BHScriptTimerContext *ctx, BHEntity *entity,
                                 const BHScriptTimerState *initial, int unconditional) {
    if (!ctx || !entity || !initial || !live(ctx, entity, entity->generation) ||
        !bh_math_environment_supported() || initial->variable_remaining > 21 ||
        initial->variable_low > 21 || initial->variable_high > 21)
        return 0;
    *state(entity) = *initial;
    bind_context(entity, ctx);
    entity->step = NULL;
    if (!bh_entity_set_query(ctx->pool, entity, unconditional ? template_timer2 : template_timer)) {
        ctx->error = BH_TIMER_POOL;
        return 0;
    }
    ctx->error = BH_TIMER_OK;
    return 1;
}
static void __cdecl delete_tick(BHEntity *entity) {
    BHScriptTimerContext *ctx = context(entity);
    if (!bh_entity_release(ctx->pool, entity))
        ctx->error = BH_TIMER_POOL;
}
static void __cdecl running_query(BHEntity *entity, void *packet) {
    BHScriptTimerPacket *event = (BHScriptTimerPacket *)packet;
    BHScriptTimerState *timer = state(entity);
    if (!event)
        return;
    if (event->event == 4 || (event->event == 20 && !timer->script_loaded))
        delete_tick(entity);
    else if (event->event == 10 && event->actor == timer->actor)
        entity->step = delete_tick;
}
static void tick(BHEntity *entity, int unconditional) {
    BHScriptTimerContext *ctx = context(entity);
    BHScriptTimerState *timer = state(entity);
    const BHScriptTimerActor *owner;
    BHScriptTimerPacket packet;
    double decremented, part;
    unsigned long generation = entity->generation;
    if (!bh_math_environment_supported()) {
        ctx->error = BH_TIMER_ARGUMENT;
        return;
    }
    if (!unconditional) {
        owner = actor(ctx, timer->actor);
        if (!owner) {
            ctx->error = BH_TIMER_ACTOR;
            return;
        }
        if (owner->owner)
            owner = owner->owner;
        if (!owner->active)
            return;
    }
    decremented = (double)timer->countdown - (1.0 / 30.0);
    bh_round_float(&timer->countdown, decremented);
    if (decremented >= 0.0)
        return;
    packet.event = 8;
    packet.actor = timer->actor;
    if (timer->fired && timer->fired->query)
        timer->fired->query(timer->fired, &packet);
    if (!live(ctx, entity, generation)) {
        ctx->error = BH_TIMER_MUTATION;
        return;
    }
    part = random_part(timer->low, timer->high);
    bh_round_float(&timer->countdown, ((double)timer->countdown - part) + timer->low);
    if (timer->remaining) {
        timer->remaining = signed_word((unsigned long)timer->remaining - 1);
        if (!timer->remaining)
            delete_tick(entity);
    }
}
static void __cdecl active_tick(BHEntity *entity) { tick(entity, 0); }
static void __cdecl always_tick(BHEntity *entity) { tick(entity, 1); }
static void activate(BHEntity *entity, void *packet, int unconditional) {
    BHScriptTimerContext *ctx = context(entity);
    BHScriptTimerState *timer = state(entity), copy;
    BHScriptTimerPacket *event = (BHScriptTimerPacket *)packet;
    const BHScriptTimerActor *variables;
    BHEntity *created, *next;
    if (!event)
        return;
    if (event->event == 20 && !timer->script_loaded) {
        delete_tick(entity);
        return;
    }
    if (event->event == 4) {
        entity->step = delete_tick;
        return;
    }
    if (event->event != 8)
        return;
    if (!bh_math_environment_supported()) {
        ctx->error = BH_TIMER_ARGUMENT;
        return;
    }
    variables = actor(ctx, event->actor);
    if (timer->variable_remaining >= 0 || timer->variable_low >= 0 || timer->variable_high >= 0) {
        if (!variables || timer->variable_remaining > 21 || timer->variable_low > 21 ||
            timer->variable_high > 21) {
            ctx->error = BH_TIMER_ACTOR;
            return;
        }
        if (timer->variable_remaining >= 0)
            timer->remaining = variables->variables[timer->variable_remaining];
        if (timer->variable_low >= 0)
            timer->low = variables->variables[timer->variable_low];
        if (timer->variable_high >= 0)
            timer->high = variables->variables[timer->variable_high];
    }
    copy = *timer;
    created = bh_entity_acquire(ctx->pool);
    if (!created) {
        ctx->error = BH_TIMER_POOL;
        return;
    }
    *state(created) = copy;
    bind_context(created, ctx);
    state(created)->script_loaded = ctx->script_loading;
    state(created)->actor = event->actor;
    bh_round_float(&state(created)->countdown, (double)copy.low - random_part(copy.low, copy.high));
    created->step = unconditional ? always_tick : active_tick;
    if (!bh_entity_set_query(ctx->pool, created, running_query)) {
        ctx->error = BH_TIMER_POOL;
        return;
    }
    next = timer->next;
    if (next) {
        if (!next->query) {
            ctx->error = BH_TIMER_ARGUMENT;
            return;
        }
        next->query(next, packet);
    }
}
static void __cdecl template_timer(BHEntity *entity, void *packet) { activate(entity, packet, 0); }
static void __cdecl template_timer2(BHEntity *entity, void *packet) { activate(entity, packet, 1); }
