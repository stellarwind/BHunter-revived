#ifndef BH_SCRIPT_WAIT_H
#define BH_SCRIPT_WAIT_H
#include "bh_script_timer.h"
typedef struct BHScriptWaitState
{
    BHEntity *fired, *next, *actor;
    long script_loaded, remaining;
    unsigned long retained14, retained18;
    long expected, variable_remaining, variable_expected;
    unsigned long retained28, retained2c;
} BHScriptWaitState;
typedef struct BHScriptWaitPacket
{
    long event;
    BHEntity *actor;
    long expected;
} BHScriptWaitPacket;
/* Source binding borrows the actual timer context/pool/actor variables. All
   owners, referenced fired/next entities and actor storage outlive the wait.
   All48 original bytes are copied; private context+generation follows them.
   Nonnegative selectors resolve from the actual22-short actor table. */
int __cdecl bh_script_wait_bind(BHScriptTimerContext *, BHEntity *, const BHScriptWaitState *);
BHScriptWaitState *__cdecl bh_script_wait_state(BHScriptTimerContext *, BHEntity *);
/* Whole00443270 /00442f40. Event14 reads expected at packet+8 independently
   of actor+4. Fired packet has40 bytes, but only event8/actor are contractual;
   source initializes the unspecified remainder without defining a new wire
   protocol. Remaining decrements modulo32 bits, starting0 means unlimited.
   Deferred deletion uses the actual admitted owner returned by its getter.
   Current wait release/recycle by a fired query stops with MUTATION. */
void __cdecl bh_script_wait_template(BHEntity *, void *packet);
void __cdecl bh_script_wait_child(BHEntity *, void *packet);
#endif
