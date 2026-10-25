# World segment queries and actual camera correction

Four complete original owners are reconstructed and linked:

| Address | Source symbol | Role |
|---|---|---|
| 0040b020–0040b2f8 | `bh_world_segment_actor_trace` | engine candidate |
| 0040ba40–0040c480 | `bh_world_segment_room_trace` | engine candidate |
| 0040c4a0–0040c65f | `bh_world_segment_run` | engine candidate |
| 0041e6b0–0041e84d | `bh_camera_correct` | game |

The first two symbols remain static helpers in the real world-segment
translation unit; the named link map includes both. These are distinct owners,
not shared epilogues, compiler helpers, imports or verified libraries. Engine
provenance remains unresolved. CRT `__ftol`, allocation and math are reused and
excluded from recovery counts. Every instruction in all four complete exported
bodies was verified against the pinned immutable original EXE.

The explicit source context borrows the live scene ring, a complete table of
world/query/room-actor bindings and an ordered actor/query array. This replaces
the original model registry and global actor pointer array without inventing
models, callbacks or vtables. A query may be absent when its collision model is
absent. Every supplied query is a real `BHSegmentQuery` over owned decoded BSP
geometry; world and actor transforms use the recovered matrix owners. Actors
have original common-kind zero as initialized by `bh_entity_acquire`. Nonzero
common kinds designate world objects and must use world bindings.

Room traversal chooses the first strict portal crossing in local slot order,
using original aperture extents. It tests room actors, room geometry and
successive owned children before tail-traversing that portal neighbor. It
retains the original room-cache fallback and cache publication before callbacks.
The global actor pass uses the room hit to shorten its segment, retains array
order for equal contacts and preserves the room's material word when an actor
replaces the geometric hit. Self identity is explicit and typed.

Actor event 3 is set once before each room actor loop. The actual installed
callback fills its position, and the actor +f4 successor is read after the
callback. No query callback is supplied or installed by production source.
Source guards bound cycles and reject missing dependencies. Errors restore
caller cache/output values but cannot undo side effects of callbacks that have
already run. Guard behavior is separate from the recovered valid-input domain.

`BHCameraCorrectionOwner` owns the former cached-room and selected-blocker
globals. Its real correction traces target to eye with radius -1, resolves a
world hit's parent when selecting its blocker, then traces between the two
normal offsets +0.1 and +0.6. A second hit uses the original
`0.1 + 0.25*fraction` normal clearance; a miss publishes the +0.6 endpoint.
The contextual callback bridge supplies this actual owner to camera state.

The numerical review retains the portal plane's signed integer to binary32
conversion; axis-specific ratio and aperture stores; mixed stored/retained
products in sphere and actor broad phases; unrounded sphere points when
computing normals; projection grouping; and signed face-flag conversion through
binary32 and CRT `__ftol`. The latter returns 64 bits while the original stores
only the low 32 bits, including the rounded positive 2^31 case. Detailed
instruction anchors are in `review.json`. No instruction match, original
runtime differential or PC24 equivalence is claimed.

Run `python META/m4/world-segment/build_evidence.py` to reproduce
`build/world-segment/world-segment.exe`. It compiles every real dependency
translation unit, links without undefined definitions and executes 52 checks.
The retained named map, command/output record and source hashes accompany the
review. Coverage includes real BSP contacts, child/self/order and bounds,
all six strict portal directions and equality borders, large integer plane
rounding, room ownership fallback, actual actor model broad/narrow phases,
material float bit loss, callback mutation/publication and bounded topology.

The composed scenario binds `bh_camera_correct_callback` to the real camera
state, runs mode 1 through the real BSP dependencies and checks wall clipping
while the actual actor pointer excludes its own collision model. It then runs
the recovered player-camera reset's 50 synchronous source state updates through
the same real correction. This scenario has no callback doubles. Separate room
event tests use explicitly isolated fixtures, never shipping callback bindings.

All execution is of source-built checks. No original code or game is executed;
no menu, flight, combat or automatic gameplay work is involved. Main integration
and per-implementation ledger admission belong to the owning restoration block.
