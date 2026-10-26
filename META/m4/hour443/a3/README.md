# Complete target/debris step0043cd00

New `src/bh_target_step.c` implements the complete1,404-byte /431-instruction
entity step0043cd00..0043d27b. Its15-entry/60-byte switch table at43d27c..43d2b7
is owned data, explicitly fingerprinted and never counted as another function.
The raw full instruction-byte listing, decompile, table and boundary hashes are
retained here. This is ordinary application lifecycle/effect behavior, not CRT,
import/EH/stack support or an independently counted shared tail. Historical
game-versus-engine authorship remains `engine_candidate`.

## Real typed closure

The source binds the SAME actual `BHDebrisBody` sleep timer, owner pointer and
`BHPhysicsState` used by the admitted response query and new complete physics
owner. It does not duplicate those fields in a shadow entity. Target fields
ec/fc..120 retain countdown, box limits, smoke chance, token, loot and counter.
The state writer synchronizes original114 emission with the physics roster's
same collision flags. Original mode004df250 and smoke image004f0028 become
borrowed explicit bindings. Source-private context lives at payload+616,
separate from the real debris response bridge at payload+632.

Actual source particle004025c0, explosion00402e50, pickup00441a00 and complete
physics004124d0 owners perform all corresponding effects. No fake constructor,
vtable, renderer, query handler or successful return is supplied. Pickup spawn
uses its real renderer bindings and owning resource/world/room registrations.
The target/debris registry is the SAME null-terminated outside-actor array used
by pickups. Retirement delegates the reviewed identical registry shift/release
behavior to actual `bh_debris_remove`, which additionally clears its source
body ownership. The existing fixed physics roster must then be disposed or
rebound before another actor steps; dynamic full-world roster reconstruction
is outside this family, not silently synthesized.

An actual live owner query receives event12 with token at+4 before destruction
effects. This interface may remain borrowed; the positive semantics of an
unrecovered event12 recipient are NOT claimed. The fixture normally installs
actual admitted `bh_actor_target_removed`, whose genuine ignored-event12 path
runs. A separately labelled test observer records the packet/order and delegates
that real receiver. It is not production code or a substitute game handler.

## Behavior and numerical contracts

Mode1 returns before state/random changes. A positive counter increments with
original32-bit wrapping. Nonzero smoke chance consumes a draw even if negative;
successful chance consumes five parameter draws plus the actual particle
constructor draw. All13 particle arguments and their original rates/decays are
forwarded. Smoke precedes countdown and sleeping branches.

Positive destruction countdown subtracts exact double1/30 and compares the
retained result before its float store. Expiry calls owner event12, then either
six randomized transformed explosions (negative loot), or the complete pickup
switch followed by one explosion. Case13 and out-of-table cases skip pickup;
case14 creates type4/variant1. Six-point interpolation spills X/Y to float while
retaining Z through the world-basis products. Each component's original sum
order is explicit, including X+Z+Y in the first row. Each real constructor
finishes before the following random point is selected.

Nonexpired targets skip physics only when the shared sleep timer is ordered>2.
Its pre-store decrement clamps negative or unordered values to0. Actual whole
physics uses original binary32 dt0x3d088889. Afterward squared linear/angular
norms are tested against .25/.01; quiet/unordered tests add exact double2/30 to
the same shared sleep word. PC53 nearest/masked and the real physics finite
domain remain required; PC24/extreme-state behavior remains a fidelity limit.
Borrowed callbacks may not invalidate or rebind the target body storage during
its step. Released/recycled current targets stop later accesses explicitly.

## Validation and integration

`python META/m4/hour443/a3/build_check.py` freshly compiles target/test, complete
physics/effects and the corrected explosion TU, then links snapshotted admitted
dependency objects. `build/target-step/bhunter-target-step-check.exe` passes179
checks, and removing the target module fails the link. Source/header contexts
and exact linked retained object hashes are in `standalone-build.json`.

Tests cover all16 nonnegative loot inputs0..15 through actual constructors,
real shared registry removal, one correct15-strength explosion for every such
case, all six rotated box-sampled12-strength explosions and their complete
48+1+9*N constructor random schedules, real smoke state, paused mode, shared
flags synchronization, actual physics motion and shared quiet timer. The
corrected explosion constant is PE004892c8=-1/32768; typical magnitudes12/15
create actual bounded debris batches. Previous wrong explosion variants remain
historical and must not inherit these linked checks.

Parent adds `src/bh_target_step.c` and `tests/target_step.c`, then calls
`int __cdecl bh_target_step_checkpoint(unsigned long *checks);`.
The standalone macro is `BH_TARGET_STEP_STANDALONE`. Full current and clean
parent builds remain admission gates. No existing TU/ledger/checkpoint file was
edited here. No original code, game launch, gameplay automation, physical
input/UI capture, audio playback or native networking occurred.
