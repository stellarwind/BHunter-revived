# Scene caller boundary and dependency review

2026-10-25. Static read-only Ghidra review of the immutable `GAME_DATA/BHUNTER.EXE`.
The code exports are inferred pseudocode; the complete instruction JSON bodies
are checked against the original PE by `extract_review.py`. No original code
was loaded as a callable subsystem, and no game was launched.

## Classification before implementation

| Entry | Complete original body | Role | Evidence |
| --- | --- | --- | --- |
| 0041c860 | 0041c860–0041d7bd, 3934 bytes / 1162 instructions | game, high confidence | Entity callback assigned by player constructor 00421d20 at 004220ea; player pose, weapons, HUD, scene traversal and effects |
| 00428f50 | 00428f50–00429455, 1286 / 360 | game, high confidence | Camera entity callback assigned by 0042a9e0, 0042f400 and 0042a9b0; scene render and game mode handoff |
| 0041c020 | 0041c020–0041c858, 2105 / 604 | game, high confidence | Eight persistent 48-byte background actor slots; actual game model/flare handles, two scene callers |
| 00477fe0 | 00477fe0–00478059, 122 / 43 | engine_candidate, attribution unknown | Render command owner, header-zero insertion and six-word pose payload; also used by level editor render path |
| 00477830 | 00477830–00477877, 72 / 25 | engine_candidate, attribution unknown | Real queued callback; six float-to-double promotions to projection camera owner 00452070 then next-link load |
| 004784c0 | 004784c0–00478533, 116 / 37 | engine_candidate, attribution unknown | Header-zero command for sky/horizon owner 00477aa0; not implemented here |
| 00477aa0 | 00477aa0–00477ad3, 52 / 22 | engine_candidate, attribution unknown | Render-mode barrier 0045b500 followed by sky/horizon drawing 0045db80; not implemented here |

All seven entries have contiguous complete RET-ended bodies. None is an import,
verified CRT/library routine, compiler exception/unwind helper, or shared tail.
No reusable engine library has been identified from this local review. The
existing verified CRT `rand` and `__ftol` calls are dependencies, not assigned
gameplay recovery work. No instruction matching claim is made.

## Caller contracts and remaining blockers

`0041c860` is a player render callback, **not** a constructor. Its inferred
`float param_1` is incorrect: instructions treat the argument as an entity
pointer. The large branch runs only when entity byte `+2bb` is one and global
`004dfb08` is nonzero. It submits listener zero using eye `+160`, angles `+16c`
and velocity `+178`, queues the projection-camera pose and horizon command,
then calls `0041c020` for each slot 0 through 7. It also produces HUD/fade,
player attachments, weapon callbacks and nearby entity indicators before
calling recovered scene owner `00409130` with FOV 60, near .01, far 25000 and
the actual draw dimensions. Remaining player body submissions and early-level
effects occur outside that gate. Completing the function still requires real
definitions/contracts for `00478060`, `00478930`, `00478730`, `0041b2d0`,
`0041bbf0`, `0041b650`, `00418f00`, `0041a3d0`, `00427710`, both dynamically
selected weapon callbacks and required render producers/consumers. Those
missing calls cannot be replaced with success callbacks or omitted branches.

`00428f50` is the alternate camera scene callback. It submits the same listener,
projection pose and horizon commands, conditionally substitutes a full-screen
image near selected position-table records, otherwise advances/renders all
eight background slots, and calls `00409130` with the same projection settings.
It then draws corner overlays when applicable. If global `004dfb08` is zero it
returns. Otherwise it rebuilds player input `00421740`, resets/reads input
through `004756b0`/`00475870`, selects player configuration via `004221e0`,
publishes actual global function choices and sends two real event packets
through `004770f0`. Finally it installs `0042a9b0` and `0047d680` into the
camera's callbacks. Restoring only its rendering prefix would not complete this
owner. The handoff's real recipient bindings, resource bindings, and horizon consumer remain
explicit blockers.

Player initialization is `00421d20`: entity allocation `00476f20`, input record
`00475640`, input bindings, hunter model loading and radius, basis/transform,
physics parameters, state fields, sound handle, weapon construction and callback
installation. The nonzero-first-argument construction branch has a separate
callback path. Full constructor admission requires these real lifetime owners.

Existing admitted definitions already cover entity allocation `00476f20`
(`bh_entity_acquire`), event/query dispatch `004770f0` (`bh_entities_query`),
and route planning `00429480` (`bh_route_plan`). These should be reused with
real owning bindings, not redundantly decompiled or listed as missing code.

Camera initialization is `0042a9e0` (with another path in `0042f400`): actual
entity allocation, level position-table copy, identity basis, pose initialization,
callback registration, room lookup `0040a8c0`, fifty calls to actual update
callback `00429fb0`, then configuration owner `004221e0(0x17)`. Replacing those
fifty updates with a guessed final pose is not supported. `0042a9b0` reinstalls
the normal camera callbacks when global `004dfb08` returns to zero.

Later in this same block, complete update `00429fb0` was restored and linked
with its real route, curve, camera-mode, font-width and text-ring dependencies;
see `../flyby-camera/REVIEW.md`. The constructor is nevertheless still blocked
by its installed refresh callback `00428f50`, whose complete rendering/handoff
body remains unrecovered. A source enum or fake callback cannot stand in for
that missing definition to admit the constructor. Color preparation
`0045d7a0` is also now restored, but full horizon drawing `0045db80` remains
separate unfinished behavior.

## Recovered camera command family

`src/bh_camera_queue.c` contains complete source owners for `00477fe0` and
`00477830`. Both reuse real queue storage and admitted `bh_main_camera_pose`
(`00452070`). Header zero is distinct from numbered bucket zero: camera commands
precede every numbered draw bucket. The six payload words occupy original
`+04..+18`, so the semantic `resource` field carries position X. The producer
copies bits, including nonfinite bits; only the actual pose owner enforces its
existing numerical guard. Queue exhaustion silently drops production in the
original; the source additionally reports zero to its explicit caller.

The source API's invalid-binding and next/tail bounds checks are defensive
binding guards. They do not claim original malformed-pointer behavior. The
consumer invokes the real source camera owner, then publishes `next` as the
original callback does. There are no original-code callbacks or stubs.

## Recovered background actor owner

`src/bh_scene_background.c` contains complete `0041c020`. It preserves eight
48-byte records, first-use and delayed recycling, binary32 endpoint/angle/state
stores, fixed 1/30-second updates, eye parallax, a triangular height envelope,
separate flame fade, model submission and two/six flare billboards. Original
startup `004474c0` sets eight negative elapsed sentinels at `00447947`; the
explicit source initialization adapter only binds storage/resources and those
sentinels. The much larger startup function is not admitted by this adapter.

The actual first/second model bindings are `m_spship.bsp` and `m_spsh3.bsp`,
loaded by startup `004474c0` into `004d1a98` and `004df238`. The actual flare
bindings are `flare_wh.bmp` and `flare_bl.bmp`, loaded by `00479200` at
`00479680`/`00479692` into `004f00ac`/`004f0078`. The recovered texture loader's
existing BMP-to-PCX alias resolves the real archive white/blue PCX payloads.
Archive entries include both actual BSPs. No arbitrary replacement pickup
model is required to exercise this background owner with real resources.

Raw `background-constants.json` records each floating constant's address, width,
bytes and exact decoded value. Several apparently positive decompiler formulas
in fact subtract negative constants: the step increases elapsed, parallax adds
0.9 times eye X/Z, and sizes/opacity add positive random terms. Real shared VC5
CRT random ordering is retained: initial construction uses seven or eight draws,
delayed recycling five, first-model flames twelve, second-model flames four.
Queue exhaustion still updates state and consumes every relevant random draw.
The source uses real admitted resource/billboard queue producers, not substitutes.

Moderate finite behavior is exercised under the established PC53/nearest kernel
profile. General PC24 equivalence, malformed/nonfinite original slot state,
large-angle transcendental edge behavior and human gameplay fidelity remain
unvalidated. Full resource initialization and the sky/horizon family are still
missing; this work does not claim that either scene caller is complete.

## Focused source checks

`check_standalone.py` compiles each real new translation unit and its focused
test translation unit with pinned VC5 SP3 flags, links named source-only
`build/scene-callers-review/camera_queue.exe` and `scene_background.exe` against
real source module objects and normal system/CRT dependencies, and executes them.
The saved `standalone.json` records exact commands and output. Current result:
23 camera checks and 32 background checks, all passed. The additional
`scene_background_native.exe` target compiles the real native test translation
unit and links all required real definitions. Its 154 checks pass with actual
M_SPSHIP/M_SPSH3 model and white/blue flare assets: one source camera command,
one source model command and two/six flare commands reach recovered consumers
and actual offscreen native rasterization. Both invalid-resource/image controls
remain blank. This is a static explicit slot-state fixture, not recovered game
startup or automated game execution. `resource-strings.json` verifies the four
actual loader string bytes directly against the immutable PE.

These focused links are
supplementary; parent integration build and checkpoint evidence are required
for admission. No byte-match evidence is inferred from functional checks.
