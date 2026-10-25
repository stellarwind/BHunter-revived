# Complete camera route advance 004296a0

2026-10-25. Boundary/role reviewed before implementation: ordinary complete
2307-byte, 701-instruction cdecl owner, `004296a0..00429fa2`, RET ended. It is
used by game cinematic camera update `00429fb0` to advance curve segments across
world rooms. Classification is `engine_candidate`, attribution unknown. No
verified CRT/library signature, compiler EH/unwind helper, import or independent
shared-tail role was established. Verified CRT `__ftol` remains a dependency
and is not counted as gameplay restoration.

`review.py` checks every original instruction byte directly against the
immutable PE, verifies all seven numerical constants, confirms the four actual
callee addresses, and records source hashes. `src/bh_camera_route.c` contains
the complete source behavior over explicit state/scene bindings. Its real
dependencies are existing `bh_world_locate` (`0040a8c0`), `bh_portal_center`
(`00433f70`), `bh_portal_normal` (`00434040`), and the real CRT float-to-integer
conversion. Route choice reads the actual source `BHNavigation.routes` matrix;
there is no replacement pathfinding callback or fabricated original pointer.

Source state explicitly represents original actor current room `+c4`, next room
`+1f4`, curve parameter `+1dc`, parameter rate `+1e0`, control vectors
`+254/+260/+26c/+278`, and selected position-table index `+284`. Context borrows
the real graph and scene ring and the caller's selected waypoint position
array. The original `004df6c0` table uses 32-byte rows; this API receives only
their position projection and does not claim that a native BHVec3 array is the
original table layout. Full startup/table selection remains a separate owner.

All three branches are recovered:

1. Corridor: promote next to current, find the previous-room incoming portal,
   continue through the other real neighbor, and project portal-to-portal curve
   controls along both original axis normals. Multiple candidates retain the
   original last-candidate loop order; absent candidates reject instead of
   reading an uninitialized local.
2. Junction: locate the selected goal using the real world locator, then read
   the destination-major graph exit cell. If incoming and outgoing are the same
   portal, the two interior control points detour around the real room center
   by -10/+10 on X/Z; Y remains at the center.
3. Destination: publish the selected waypoint index, construct the final
   incoming-portal-to-waypoint approach, and retain the final phase offset 2.

The old fractional parameter is converted to elapsed time before changing rate.
Corridor/junction time quotients are explicitly binary32; destination retains
its quotient. Corridor uses a binary32 distance sum but retains the new rate
for remapping phase. Junction retains the distance sum but stores/reloads the
new rate before remapping. Destination stores the sum and uses the stored new
rate. The two traversal branches add one to phase; destination adds two. These
signs follow raw -1/-2 constants, not the misleading initial pseudocode.

Source guards are explicit extensions for malformed foreign topology,
uninitialized incoming/outgoing slots, invalid waypoint/route indices and
nonfinite/out-of-range phase or old rate. Guard failure preserves route state
and reports a context error. Portal loops are bounded by six slots, navigation
lookup by its actual node count, and world lookup by the validated source ring.
Original zero control length retains masked-x87 positive infinity/NaN behavior;
it is tested and not silently replaced with success or a fixed speed. Moderate
finite PC53 behavior is exercised; PC24 and broad extreme numerical fidelity
remain open. No instruction matching or original-runtime differential execution
is claimed.

Focused source-only target `build/camera-route/camera_route.exe` compiles this
actual TU and its real dependencies and passes 15 checks. They construct real
source navigation graphs for corridor, final approach, junction matrix routing,
same-portal detour, invalid-input preservation, and zero-length behavior.

Separate `tests/camera_route_real.c` compiles and links as
`build/camera-route/camera_route_real.exe`. It loads actual BLOCKS.TXT,
constructs 454 real model geometries and 5815 actual TEST.LEV placements through
the recovered owners, matches portals, and builds 1454 navigation nodes/283
junctions with the actual TEST.PTH cache. Four captured real route goal vectors
and actual graph nodes exercise matrix-selected exits. Real corridor and
destination-room branches run too, and their controls feed the recovered
`004341a0` interpolation owner. All 31 real-data checks pass. Captured goal
vectors are actual earlier route fixtures; this test does not invent or admit
the original full camera-table initializer. Exact compile/link/run commands and
output are saved in `standalone.json`. Parent current integration/affected
checkpoint and ledger evidence are required before admission.

No game, automatic play controller, original-code mapper or original subsystem
callback ran. This work restores source dependencies while preserving the
remaining scene/camera constructor and credits/font blockers.
