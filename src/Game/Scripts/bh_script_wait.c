#include "bh_script_wait.h"
#include <stddef.h>
#include <string.h>

static_assert(sizeof(BHScriptWaitState) == 48, "BHScriptWaitState size");
static_assert(offsetof(BHEntity, payload) + 4 == 0xe8, "wait state start");
static_assert(offsetof(BHEntity, payload) + 52 == 0x118, "wait context start");
static_assert(offsetof(BHScriptWaitPacket, expected) == 8, "BHScriptWaitPacket.expected offset");
typedef struct FiredPacket
{
    long event;
    BHEntity *actor;
    unsigned char unspecified[32];
} FiredPacket;
static_assert(sizeof(FiredPacket) == 40, "FiredPacket size");
static BHScriptWaitState *state(BHEntity *entity)
{
    return bh_script_wait_fields(entity);
}
static BHScriptTimerContext *owner(BHEntity *entity)
{
    BHScriptTimerContext *ctx;
    memcpy(&ctx, &entity->bindings.script_timer, 4);
    return ctx;
}
static unsigned long bound_generation(BHEntity *entity)
{
    unsigned long n;
    memcpy(&n, &entity->bindings.wait_generation, 4);
    return n;
}
static void bind(BHEntity *entity, BHScriptTimerContext *ctx)
{
    memcpy(&entity->bindings.script_timer, &ctx, 4);
    memcpy(&entity->bindings.wait_generation, &entity->generation, 4);
}
static int live(BHScriptTimerContext *ctx, BHEntity *entity, unsigned long generation)
{
    return ctx && bh_entity_is_active(ctx->pool, entity) && entity->generation == generation;
}
static BHScriptTimerContext *owned(BHEntity *entity, BHEntityCallback query)
{
    BHScriptTimerContext *ctx;
    if (!entity || entity->query != query)
        return NULL;
    ctx = owner(entity);
    return ctx && live(ctx, entity, bound_generation(entity)) ? ctx : NULL;
}
BHScriptWaitState *bh_script_wait_state(BHScriptTimerContext *ctx, BHEntity *entity)
{
    if (!ctx || !entity || (entity->query != bh_script_wait_template && entity->query != bh_script_wait_child) ||
        owner(entity) != ctx || !live(ctx, entity, bound_generation(entity)))
        return NULL;
    return state(entity);
}
int bh_script_wait_bind(BHScriptTimerContext *ctx, BHEntity *entity, const BHScriptWaitState *initial)
{
    BHScriptWaitState copy;
    if (!ctx || !entity || !initial || !live(ctx, entity, entity->generation) || initial->variable_remaining > 21 ||
        initial->variable_expected > 21)
        return 0;
    memcpy(&copy, initial, 48);
    memcpy(state(entity), &copy, 48);
    bind(entity, ctx);
    entity->step = NULL;
    if (!bh_entity_set_query(ctx->pool, entity, bh_script_wait_template))
    {
        ctx->error = BH_TIMER_POOL;
        return 0;
    }
    ctx->error = BH_TIMER_OK;
    return 1;
}
void bh_script_wait_template(BHEntity *entity, void *packet)
{
    BHScriptTimerContext *ctx = owned(entity, bh_script_wait_template);
    BHScriptWaitState *s, *created_state;
    BHScriptWaitPacket *event = (BHScriptWaitPacket *)packet;
    const BHScriptTimerActor *variables = NULL;
    BHEntity *created, *next;
    unsigned long i;
    if (!ctx || !event)
        return;
    s = state(entity);
    ctx->error = BH_TIMER_OK;
    if (event->event == 20 && !s->script_loaded)
    {
        if (!bh_entity_release(ctx->pool, entity))
            ctx->error = BH_TIMER_POOL;
        return;
    }
    if (event->event == 4)
    {
        entity->step = bh_script_timer_delete_step();
        return;
    }
    if (event->event != 8)
        return;
    if (!event->actor)
    {
        ctx->error = BH_TIMER_ACTOR;
        return;
    }
    if (s->variable_remaining >= 0 || s->variable_expected >= 0)
    {
        for (i = 0; i < ctx->actor_count; ++i)
            if (ctx->actors[i].entity == event->actor)
            {
                variables = ctx->actors + i;
                break;
            }
        if (!variables || s->variable_remaining > 21 || s->variable_expected > 21)
        {
            ctx->error = BH_TIMER_ACTOR;
            return;
        }
        if (s->variable_remaining >= 0)
            s->remaining = variables->variables[s->variable_remaining];
        if (s->variable_expected >= 0)
            s->expected = variables->variables[s->variable_expected];
    }
    created = bh_entity_acquire(ctx->pool);
    if (!created)
    {
        ctx->error = BH_TIMER_POOL;
        return;
    }
    created->position = event->actor->position;
    memcpy(state(created), s, 48);
    bind(created, ctx);
    created_state = state(created);
    created_state->script_loaded = ctx->script_loading;
    created_state->actor = event->actor;
    if (!bh_entity_set_query(ctx->pool, created, bh_script_wait_child))
    {
        bh_entity_release(ctx->pool, created);
        ctx->error = BH_TIMER_POOL;
        return;
    }
    next = s->next;
    if (next)
    {
        if (!next->query)
        {
            ctx->error = BH_TIMER_ARGUMENT;
            return;
        }
        next->query(next, packet);
    }
}
void bh_script_wait_child(BHEntity *entity, void *packet)
{
    BHScriptTimerContext *ctx = owned(entity, bh_script_wait_child);
    BHScriptWaitState *s;
    BHScriptWaitPacket *event = (BHScriptWaitPacket *)packet;
    FiredPacket activation;
    unsigned long generation, word;
    long result;
    if (!ctx || !event)
        return;
    s = state(entity);
    ctx->error = BH_TIMER_OK;
    if (event->event == 20 && !s->script_loaded)
    {
        if (!bh_entity_release(ctx->pool, entity))
            ctx->error = BH_TIMER_POOL;
        return;
    }
    if (event->event == 14)
    {
        if (event->expected != s->expected)
            return;
        memset(&activation, 0, sizeof(activation));
        activation.event = 8;
        activation.actor = s->actor;
        generation = entity->generation;
        if (s->fired && s->fired->query)
            s->fired->query(s->fired, &activation);
        if (!live(ctx, entity, generation) || owner(entity) != ctx || bound_generation(entity) != generation)
        {
            ctx->error = BH_TIMER_MUTATION;
            return;
        }
        s = state(entity);
        if (s->remaining)
        {
            word = (unsigned long)s->remaining - 1UL;
            memcpy(&result, &word, 4);
            s->remaining = result;
            if (!result)
            {
                if (!bh_entity_set_query(ctx->pool, entity, NULL))
                {
                    ctx->error = BH_TIMER_POOL;
                    return;
                }
                entity->step = bh_script_timer_delete_step();
            }
        }
        return;
    }
    if (event->event == 4 || (event->event == 10 && event->actor == s->actor))
        entity->step = bh_script_timer_delete_step();
}
