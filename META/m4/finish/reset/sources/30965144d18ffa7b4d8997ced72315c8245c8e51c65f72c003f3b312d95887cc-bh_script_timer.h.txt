#ifndef BH_SCRIPT_TIMER_H
#define BH_SCRIPT_TIMER_H
#include "bh_entities.h"

/* The complete 48-byte state copied by original timer/timer2 activation,
   verified at entity +e8 through +114. Private source binding follows it;
   it is not an inferred original global or original field. */
typedef struct BHScriptTimerState {
    BHEntity *fired, *next, *actor;
    long script_loaded, remaining, low, high, reserved;
    long variable_remaining, variable_low, variable_high;
    float countdown;
} BHScriptTimerState;

typedef struct BHScriptTimerActor {
    BHEntity *entity;
    const struct BHScriptTimerActor *owner;
    long active;
    short variables[22];
} BHScriptTimerActor;

typedef struct BHScriptTimerPacket {
    long event;
    BHEntity *actor;
} BHScriptTimerPacket;
typedef enum BHScriptTimerError {
    BH_TIMER_OK,
    BH_TIMER_ARGUMENT,
    BH_TIMER_ACTOR,
    BH_TIMER_POOL,
    BH_TIMER_MUTATION
} BHScriptTimerError;

typedef struct BHScriptTimerContext {
    BHEntityPool *pool;
    const BHScriptTimerActor *actors;
    unsigned long actor_count;
    long script_loading;
    BHScriptTimerError error;
} BHScriptTimerContext;

/* Context, pool and actor-state table must outlive all bound timers. The table
   supplies actual caller-owned actor variables and one-level owner/activity
   state; it is not an invented entity/world offset. Changes are visible at
   each activation/tick. One tick is the original fixed double 1/30 second.
   Uses the linked VC5 CRT rand() state, with the original signed 32-bit product
   wrapping and -1/32768 factor; no replacement random generator. */
int __cdecl bh_script_timers_init(BHScriptTimerContext *context, BHEntityPool *pool,
                                  const BHScriptTimerActor *actors, unsigned long count);
/* Bind a live real pool entity as timer (always_tick==0) or timer2 (!=0).
   Initial state is explicit; use -1 for each literal variable selector. Timer
   activation event 8 resolves signed actor variables, clones all 48 state bytes
   into a real acquired entity, installs its real step/query callbacks, then
   forwards activation to next. Event 4 defers template deletion; running timer
   event 4 deletes immediately. Event 20 deletes only non-script-loaded timers.
   Actor event 10 defers matching running-timer deletion until the next step.
   Template binding is a source constructor adapter, not the full script parser.
   Caller must retain fired/next entities and contexts until their timers end. */
int __cdecl bh_script_timer_bind(BHScriptTimerContext *context, BHEntity *entity,
                                 const BHScriptTimerState *state, int always_tick);
/* Borrowed mutable original timer state; expires on release/recycle. A callback
   may change it. If a fired callback releases/recycles the current timer, source
   stepping stops with MUTATION instead of writing through a recycled slot. */
BHScriptTimerState *__cdecl bh_script_timer_state(BHScriptTimerContext *context, BHEntity *entity);
/* Count-zero exposure of actual00442eb0. Caller must supply its existing
   persistent timer context binding at entity payload+52. */
BHEntityStep __cdecl bh_script_timer_delete_step(void);
/* Count-zero exposure of actual00442de0 for placed-world startup/reset.
   Bind the real timer/lifetime owner before installing this callback. */
BHEntityStep __cdecl bh_script_timer_unconditional_step(void);
#endif
