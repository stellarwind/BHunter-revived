# Primary weapon HUD arc00432ff0 and shared curved-path producer00432050

Classified before recovery:00432ff0 is a game HUD callback installed in weapon
table0048fa60 index4's second word (data xref0048fac4);375bytes/104instructions
through00433166.00432050 is a complete794-byte/233-instruction shared curved
effect-path producer, also called outside the weapon HUD; engine-candidate
authorship remains unresolved. Neither body is CRT/import/compiler scaffolding
or an independently counted shared tail. CRT rand and verified RET sentinels
are excluded from gameplay function counts.

All original bytes are pinned to immutable reference SHA256 by
`export_review.py`, with whole-body hashes/bounds in `owners.json`. Both full
raw listings and decompiler output are retained. The generic function-create
CLI failed for the initially undeclared callback; `DefineArcBoundary.java`
added the reviewed AddressSet00432ff0..00433166 only to the Ghidra analysis DB.
No EXE or other immutable reference was changed and no original code executed.

## Dependencies, ownership and queue representation

The real `src/bh_weapon_hud_arc.c` TU links the already recovered
`bh_queue_reserve`00477530 and actual VC5 CRT `rand` (RAND_MAX32767). It owns no
weapon/target allocation and invents no callbacks/vtables. `BHWeaponHudState`
is an explicit borrowed binding of weapon+f8 target and signed+fc charge;
`BHWeaponHudArcPlayer` binds observed player+0c position,+30 forward,+280 aim.
The target's real installed query at+d0 must remain alive through synchronous
event3 dispatch. Its returned XYZ packet and charge mutation are consumed in
the actual original order. A compact full weapon/player ABI is not asserted.

The existing draw enum is extended at its end only: BH_DRAW_PATH_HEADER14 and
BH_DRAW_PATH_POINT15 are source semantic selectors, not original code pointers.
Header resource is borrowed original-global0049632c image identity; words0 is
the number of successfully allocated points. Point payload retains original
record offsets: resource=floatXbits, words0=Y,1=Z,2=width,3=first texture
parameter,4=second texture parameter,5=modulation,6=rand&3 tile. Unused command
payload is not cleared. Root's separate `bh_path_queue` family owns actual
queued path consumption/setup and native drawing; this producer does not
substitute a software/native consumer or claim its admission evidence.

## Complete callback and curve behavior

NULL target returns without player reads or RNG/queue effects. Otherwise
capture start=stored(player.position+forward) and
control=stored(start-aim*(-100float)) BEFORE calling target event3. After the
query, preserve target XYZ as endpoint and construct a second control point
using three actual random draws: target-(rand-16384)*(-1/2048double).
Read signed weapon charge after those query/random effects; pass
stored(charge*(1/48double)) to real curved-path producer. Query may replace
target or mutate charge/player; captured player control remains unchanged.

Producer clamps strength>1 to1. It reserves bucket3 header first; failure stops
all further work and random consumption. A successful initial point copies
start, width.5, texture parameters0/0, modulation100*strength and one random
tile. Progress starts0 and adds strength/16, rounding float on every one of16
steps (loop indices7..-8). At each successfully reserved point, blend the
start/control line and target-control/target line with smoothstep of1-progress.
Width1.25, sqrt(strength) attribute, float successful-point index and
100*(strength-progress) modulation are retained. All but last step add three
random jitters with envelope stored((64-index²)*strength*2^-21), then one random
tile. Last step has no jitter. Failed point reservations still advance progress
but consume no random numbers; final header publishes actual success count,
including0 when only header fit. Full queues drop without error; malformed
source queue reports failure while preserving earlier queue effects.

All17 points imply62 actual CRT draws; HUD callback adds3 for65. Capacity9
(headers only) consumes0 producer draws; capacity10 publishes zero-point header
and consumes0; capacities11/12/26/27 consume1/5/61/62. Global CRT stream ordering
is retained, not replaced with a private RNG or always-success callback.

## Numerical stores and validation

Caller owns moderate finite PC53/nearest/masked profile. Negative strength is
an explicit source guard before sqrt/publication, not an invented original
lower clamp; callback guard occurs after its query and3 random draws. PC24,
nonfinite/huge inputs and original software rasterization remain unvalidated.

Raw1-progress is retained for one polynomial square factor while its stored
float is used for the other. Blend is stored float for line weights, but the
complement comes from retained raw blend. X's two interpolated line sums stay
double through blending; Y/Z's line sums reload their stored float values.
Every final coordinate and jitter envelope is stored float. These distinctions
are anchored in raw00432157/1a7/1ef and004321c2/1dc/2208/2215/222b.

`tests/weapon_hud_arc.c` has114checks,0failures in named source-linked
`bhunter-weapon-hud-arc-check.exe`; exact commands/source and real integration
object hashes are in `standalone-build.json`. Checks include actual CRT RNG
recurrence positive control, analytic initial/first/last/partial/zero-strength
geometry, fields and untouched payload, clamping, all queue exhaustion cuts,
global RNG advancement, real synchronous event dispatch and mutation ordering,
and three independent static PC53 fixtures with12 exact probes detecting
retained-X/stored-YZ/raw-blend-complement simplifications. The callback fixture
is isolated test behavior, absent from source/shipping targets and not counted
as a recovered game query implementation. Static fixtures are analytical
instruction-derived evidence, not original-runtime differential claims.
