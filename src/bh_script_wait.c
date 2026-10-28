#include "bh_script_wait.h"
#include <stddef.h>
#include <string.h>

typedef char wait_state_size[sizeof(BHScriptWaitState) == 48 ? 1 : -1];
typedef char wait_state_start[offsetof(BHEntity, payload) + 4 == 0xe8 ? 1 : -1];
typedef char wait_context_start[offsetof(BHEntity, payload) + 52 == 0x118 ? 1 : -1];
typedef char wait_packet_expected[offsetof(BHScriptWaitPacket, expected) == 8 ? 1 : -1];
typedef struct FiredPacket
{
    long event;
    BHEntity *actor;
    unsigned char unspecified[32];
} FiredPacket;
typedef char fired_packet_size[sizeof(FiredPacket) == 40 ? 1 : -1];
static BHScriptWaitState *state(BHEntity *e)
{
    return (BHScriptWaitState *)(e->payload + 4);
}
static BHScriptTimerContext *owner(BHEntity *e)
{
    BHScriptTimerContext *c;
    memcpy(&c, e->payload + 52, 4);
    return c;
}
static unsigned long bound_generation(BHEntity *e)
{
    unsigned long n;
    memcpy(&n, e->payload + 56, 4);
    return n;
}
static void bind(BHEntity *e, BHScriptTimerContext *c)
{
    memcpy(e->payload + 52, &c, 4);
    memcpy(e->payload + 56, &e->generation, 4);
}
static int live(BHScriptTimerContext *c, BHEntity *e, unsigned long generation)
{
    return c && bh_entity_is_active(c->pool, e) && e->generation == generation;
}
static BHScriptTimerContext *owned(BHEntity *e, BHEntityCallback query)
{
    BHScriptTimerContext *c;
    if (!e || e->query != query)
        return NULL;
    c = owner(e);
    return c && live(c, e, bound_generation(e)) ? c : NULL;
}
BHScriptWaitState *__cdecl bh_script_wait_state(BHScriptTimerContext *c, BHEntity *e)
{
    if (!c || !e || (e->query != bh_script_wait_template && e->query != bh_script_wait_child) || owner(e) != c ||
        !live(c, e, bound_generation(e)))
        return NULL;
    return state(e);
}
int __cdecl bh_script_wait_bind(BHScriptTimerContext *c, BHEntity *e, const BHScriptWaitState *initial)
{
    BHScriptWaitState copy;
    if (!c || !e || !initial || !live(c, e, e->generation) || initial->variable_remaining > 21 ||
        initial->variable_expected > 21)
        return 0;
    memcpy(&copy, initial, 48);
    memcpy(state(e), &copy, 48);
    bind(e, c);
    e->step = NULL;
    if (!bh_entity_set_query(c->pool, e, bh_script_wait_template))
    {
        c->error = BH_TIMER_POOL;
        return 0;
    }
    c->error = BH_TIMER_OK;
    return 1;
}
void __cdecl bh_script_wait_template(BHEntity *e, void *packet)
{
    BHScriptTimerContext *c = owned(e, bh_script_wait_template);
    BHScriptWaitState *s, *created_state;
    BHScriptWaitPacket *event = (BHScriptWaitPacket *)packet;
    const BHScriptTimerActor *variables = NULL;
    BHEntity *created, *next;
    unsigned long i;
    if (!c || !event)
        return;
    s = state(e);
    c->error = BH_TIMER_OK;
    if (event->event == 20 && !s->script_loaded)
    {
        if (!bh_entity_release(c->pool, e))
            c->error = BH_TIMER_POOL;
        return;
    }
    if (event->event == 4)
    {
        e->step = bh_script_timer_delete_step();
        return;
    }
    if (event->event != 8)
        return;
    if (!event->actor)
    {
        c->error = BH_TIMER_ACTOR;
        return;
    }
    if (s->variable_remaining >= 0 || s->variable_expected >= 0)
    {
        for (i = 0; i < c->actor_count; ++i)
            if (c->actors[i].entity == event->actor)
            {
                variables = c->actors + i;
                break;
            }
        if (!variables || s->variable_remaining > 21 || s->variable_expected > 21)
        {
            c->error = BH_TIMER_ACTOR;
            return;
        }
        if (s->variable_remaining >= 0)
            s->remaining = variables->variables[s->variable_remaining];
        if (s->variable_expected >= 0)
            s->expected = variables->variables[s->variable_expected];
    }
    created = bh_entity_acquire(c->pool);
    if (!created)
    {
        c->error = BH_TIMER_POOL;
        return;
    }
    created->position = event->actor->position;
    memcpy(state(created), s, 48);
    bind(created, c);
    created_state = state(created);
    created_state->script_loaded = c->script_loading;
    created_state->actor = event->actor;
    if (!bh_entity_set_query(c->pool, created, bh_script_wait_child))
    {
        bh_entity_release(c->pool, created);
        c->error = BH_TIMER_POOL;
        return;
    }
    next = s->next;
    if (next)
    {
        if (!next->query)
        {
            c->error = BH_TIMER_ARGUMENT;
            return;
        }
        next->query(next, packet);
    }
}
void __cdecl bh_script_wait_child(BHEntity *e, void *packet)
{
    BHScriptTimerContext *c = owned(e, bh_script_wait_child);
    BHScriptWaitState *s;
    BHScriptWaitPacket *event = (BHScriptWaitPacket *)packet;
    FiredPacket activation;
    unsigned long generation, word;
    long result;
    if (!c || !event)
        return;
    s = state(e);
    c->error = BH_TIMER_OK;
    if (event->event == 20 && !s->script_loaded)
    {
        if (!bh_entity_release(c->pool, e))
            c->error = BH_TIMER_POOL;
        return;
    }
    if (event->event == 14)
    {
        if (event->expected != s->expected)
            return;
        memset(&activation, 0, sizeof(activation));
        activation.event = 8;
        activation.actor = s->actor;
        generation = e->generation;
        if (s->fired && s->fired->query)
            s->fired->query(s->fired, &activation);
        if (!live(c, e, generation) || owner(e) != c || bound_generation(e) != generation)
        {
            c->error = BH_TIMER_MUTATION;
            return;
        }
        s = state(e);
        if (s->remaining)
        {
            word = (unsigned long)s->remaining - 1UL;
            memcpy(&result, &word, 4);
            s->remaining = result;
            if (!result)
            {
                if (!bh_entity_set_query(c->pool, e, NULL))
                {
                    c->error = BH_TIMER_POOL;
                    return;
                }
                e->step = bh_script_timer_delete_step();
            }
        }
        return;
    }
    if (event->event == 4 || (event->event == 10 && event->actor == s->actor))
        e->step = bh_script_timer_delete_step();
}
