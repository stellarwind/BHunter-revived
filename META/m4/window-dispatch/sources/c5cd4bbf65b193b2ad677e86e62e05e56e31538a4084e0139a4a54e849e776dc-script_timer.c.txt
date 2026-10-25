#include "../src/bh_script_timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Case {
    unsigned long seed;
    long low, high;
    unsigned long initial, rearmed;
} Case;
static const Case cases[] = {
#include "script_timer_cases.h"
};
static unsigned long checks, fired_count, forwarded_count, used_at_forward;
static int failures, kill_on_fire, override_on_fire;
static BHEntity *last_actor, *watched;
static BHEntityPool *observed_pool;
static BHScriptTimerContext *observed_context;
#define CHECK(x, label) check((x) != 0, label, __LINE__)
static void check(int ok, const char *label, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        if (failures < 10)
            printf("FAIL timer %d: %s\n", line, label);
    }
}
static unsigned long bits(float value) {
    unsigned long result;
    memcpy(&result, &value, 4);
    return result;
}
static void __cdecl fired(BHEntity *entity, void *packet) {
    BHScriptTimerPacket *event = (BHScriptTimerPacket *)packet;
    CHECK(event->event == 8, "real fired callback receives activation packet");
    ++fired_count;
    last_actor = event->actor;
    if (override_on_fire) {
        BHScriptTimerState *timer = bh_script_timer_state(observed_context, watched);
        CHECK(timer != NULL, "callback can mutate the current live timer state");
        if (timer) {
            timer->countdown = .25f;
            timer->low = timer->high = 1;
            timer->remaining = 2;
        }
    }
    if (kill_on_fire)
        CHECK(bh_entity_release(observed_pool, watched),
              "callback releases current real pool timer");
}
static void __cdecl forwarded(BHEntity *entity, void *packet) {
    BHScriptTimerPacket *event = (BHScriptTimerPacket *)packet;
    CHECK(event->event == 8, "real next callback receives original activation");
    ++forwarded_count;
    used_at_forward = observed_pool->used;
}
static BHScriptTimerState initial(BHEntity *sink, BHEntity *next) {
    BHScriptTimerState state;
    memset(&state, 0, sizeof(state));
    state.fired = sink;
    state.next = next;
    state.script_loaded = 1;
    state.variable_remaining = state.variable_low = state.variable_high = -1;
    return state;
}
static void tests(void) {
    BHEntityPool pool;
    BHEntity *actor, *sink, *next, *template, *child;
    BHScriptTimerActor actor_state, owner_state;
    BHScriptTimerContext ctx;
    BHScriptTimerState config, *timer;
    BHScriptTimerPacket packet;
    unsigned long i, generation, before;
    float unchanged;
    CHECK(bh_entities_init(&pool, 8, NULL), "real source entity pool for timers");
    observed_pool = &pool;
    actor = bh_entity_acquire(&pool);
    sink = bh_entity_acquire(&pool);
    next = bh_entity_acquire(&pool);
    template = bh_entity_acquire(&pool);
    CHECK(actor && sink && next && template, "real actor, callbacks and script template slots");
    if (!actor || !sink || !next || !template) {
        bh_entities_dispose(&pool);
        return;
    }
    CHECK(bh_entity_set_query(&pool, sink, fired) && bh_entity_set_query(&pool, next, forwarded),
          "real callback registration dependencies");
    memset(&actor_state, 0, sizeof(actor_state));
    memset(&owner_state, 0, sizeof(owner_state));
    actor_state.entity = actor;
    actor_state.active = 1;
    CHECK(bh_script_timers_init(&ctx, &pool, &actor_state, 1),
          "explicit stable timer/actor ownership");
    observed_context = &ctx;
    packet.event = 8;
    packet.actor = actor;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const Case *c = cases + i;
        config = initial(sink, next);
        config.low = c->low;
        config.high = c->high;
        CHECK(bh_script_timer_bind(&ctx, template, &config, 1), "bind actual timer2 query handler");
        before = pool.used;
        srand(c->seed);
        template->query(template, &packet);
        child = pool.active;
        timer = bh_script_timer_state(&ctx, child);
        CHECK(timer && child != template && pool.used == before + 1 && used_at_forward == pool.used,
              "activation clones real entity and registers it before forwarding");
        if (!timer)
            break;
        CHECK(timer->actor == actor && bits(timer->countdown) == c->initial,
              "initial delay matches independent CRT rand/signed IMUL/binary32 bits");
        timer->countdown = -.25f;
        before = fired_count;
        CHECK(bh_entities_focus(&pool, child) && bh_entities_step(&pool),
              "real source focused step dispatch");
        CHECK(fired_count == before + 1 && last_actor == actor &&
                  bits(timer->countdown) == c->rearmed,
              "one event and rearm preserve negative remainder and second random value");
        packet.event = 4;
        child->query(child, &packet);
        CHECK(pool.used == 4 && !pool.focused, "running event 4 releases slot immediately");
        packet.event = 8;
    }
    config = initial(sink, next);
    config.remaining = 99;
    config.low = 3;
    config.high = 4;
    config.variable_remaining = 0;
    config.variable_low = 1;
    config.variable_high = 2;
    actor_state.variables[0] = 2;
    actor_state.variables[1] = actor_state.variables[2] = 0;
    actor_state.owner = &owner_state;
    owner_state.active = 0;
    CHECK(bh_script_timer_bind(&ctx, template, &config, 0),
          "bind actual activity-gated timer handler");
    template->query(template, &packet);
    child = pool.active;
    timer = bh_script_timer_state(&ctx, child);
    CHECK(timer && timer->remaining == 2 && !timer->low && !timer->high &&
              bh_script_timer_state(&ctx, template)->remaining == 2,
          "signed actor variables resolve into template before cloning");
    if (timer) {
        unchanged = timer->countdown;
        before = fired_count;
        CHECK(bh_entities_focus(&pool, child) && bh_entities_step(&pool) && fired_count == before &&
                  timer->countdown == unchanged,
              "inactive owner pauses timer without subtracting time");
        owner_state.active = 1;
        actor_state.active = 0;
        CHECK(bh_entities_step(&pool) && fired_count == before + 1 && timer->remaining == 1,
              "one-level owner's activity overrides actor flag");
        generation = child->generation;
        CHECK(bh_entities_step(&pool) && fired_count == before + 2 &&
                  child->generation != generation && pool.used == 4,
              "finite repeat count decrements and releases exactly at zero");
    }
    actor_state.owner = NULL;
    actor_state.active = 1;
    actor_state.variables[0] = -2;
    actor_state.variables[1] = actor_state.variables[2] = -1;
    CHECK(bh_script_timer_bind(&ctx, template, &config, 0), "bind signed-variable fixture");
    template->query(template, &packet);
    child = pool.active;
    timer = bh_script_timer_state(&ctx, child);
    CHECK(timer && timer->remaining == -2 && timer->low == -1 && timer->high == -1 &&
              bh_entities_focus(&pool, child) && bh_entities_step(&pool) && timer->remaining == -3,
          "negative actor shorts sign-extend and negative repeat count keeps decrementing");
    packet.event = 4;
    child->query(child, &packet);
    packet.event = 8;
    config = initial(sink, NULL);
    ctx.script_loading = 1;
    CHECK(bh_script_timer_bind(&ctx, template, &config, 1), "bind unconditional cleanup fixture");
    template->query(template, &packet);
    child = pool.active;
    packet.event = 20;
    child->query(child, &packet);
    CHECK(bh_script_timer_state(&ctx, child) != NULL, "script-loaded runtime survives event 20");
    packet.event = 10;
    packet.actor = sink;
    child->query(child, &packet);
    generation = child->generation;
    CHECK(bh_entities_focus(&pool, child) && bh_entities_step(&pool) &&
              child->generation == generation,
          "nonmatching actor removal does not delete timer");
    packet.actor = actor;
    child->query(child, &packet);
    CHECK(child->generation == generation && bh_entities_step(&pool) &&
              child->generation != generation,
          "matching actor removal defers deletion until step");
    ctx.script_loading = 0;
    packet.event = 8;
    template->query(template, &packet);
    child = pool.active;
    packet.event = 20;
    child->query(child, &packet);
    CHECK(pool.used == 4, "non-script-loaded runtime removed by event 20");
    packet.event = 8;
    actor_state.active = 0;
    template->query(template, &packet);
    child = pool.active;
    watched = child;
    override_on_fire = 1;
    before = fired_count;
    CHECK(bh_entities_focus(&pool, child) && bh_entities_step(&pool) && fired_count == before + 1 &&
              bh_script_timer_state(&ctx, child)->countdown == 1.25f &&
              bh_script_timer_state(&ctx, child)->remaining == 1,
          "timer2 ignores actor inactivity and reloads callback-mutated delay and count");
    override_on_fire = 0;
    packet.event = 4;
    child->query(child, &packet);
    packet.event = 8;
    template->query(template, &packet);
    child = pool.active;
    watched = child;
    kill_on_fire = 1;
    before = pool.used;
    CHECK(bh_entities_focus(&pool, child) && bh_entities_step(&pool) && pool.used == before - 1 &&
              ctx.error == BH_TIMER_MUTATION,
          "callback release stops source rearm through recycled slot");
    kill_on_fire = 0;
    packet.event = 4;
    generation = template->generation;
    template->query(template, &packet);
    CHECK(template->generation == generation && bh_entities_focus(&pool, template) &&
              bh_entities_step(&pool) && template->generation != generation,
          "template event 4 installs real deferred deletion callback");
    CHECK(bh_entities_valid(&pool) && bh_entities_dispose(&pool),
          "all callback/query/pool lifetime checks close");
}
typedef struct Memory {
    unsigned long live;
    int reject;
} Memory;
static void *__cdecl allocate(unsigned long bytes, void *user) {
    Memory *memory = (Memory *)user;
    void *result;
    if (memory->reject)
        return NULL;
    result = malloc(bytes);
    if (result)
        ++memory->live;
    return result;
}
static void __cdecl release(void *pointer, void *user) {
    Memory *memory = (Memory *)user;
    CHECK(memory->live > 0, "balanced real timer pool ownership");
    --memory->live;
    free(pointer);
}
static void allocation_failure(void) {
    Memory memory = {0, 0};
    BHSceneAllocator allocator = {allocate, release, &memory};
    BHEntityPool pool;
    BHEntity *source, *actor, *sink, *next;
    BHScriptTimerActor actor_state;
    BHScriptTimerContext ctx;
    BHScriptTimerState config;
    BHScriptTimerPacket packet;
    unsigned long before;
    CHECK(bh_entities_init(&pool, 4, &allocator), "real allocator for exhausted timer pool");
    actor = bh_entity_acquire(&pool);
    sink = bh_entity_acquire(&pool);
    next = bh_entity_acquire(&pool);
    source = bh_entity_acquire(&pool);
    CHECK(actor && sink && next && source && !pool.free_slots,
          "exhaust all real slots before growth failure");
    memset(&actor_state, 0, sizeof(actor_state));
    actor_state.entity = actor;
    actor_state.variables[0] = -7;
    CHECK(bh_script_timers_init(&ctx, &pool, &actor_state, 1), "failure-path timer context");
    config = initial(sink, next);
    config.variable_remaining = 0;
    CHECK(bh_script_timer_bind(&ctx, source, &config, 1), "failure-path real callback binding");
    packet.event = 8;
    packet.actor = actor;
    before = forwarded_count;
    memory.reject = 1;
    source->query(source, &packet);
    CHECK(ctx.error == BH_TIMER_POOL && pool.used == 4 && forwarded_count == before &&
              bh_script_timer_state(&ctx, source)->remaining == -7 && memory.live == 1,
          "failed real growth keeps resolved template values and never forwards or invents child");
    CHECK(bh_entities_valid(&pool) && bh_entities_dispose(&pool) && !memory.live,
          "allocation failure retains valid pool and complete teardown");
}
int bh_script_timer_checkpoint(unsigned long *count) {
    checks = fired_count = forwarded_count = 0;
    failures = kill_on_fire = override_on_fire = 0;
    tests();
    allocation_failure();
    *count = checks;
    printf("{\"scenario\":\"script-timers\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
