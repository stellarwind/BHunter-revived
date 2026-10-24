# BHunter restoration progress

Updated 2026-10-24. Plan: `../RESTORATION_PLAN.md`.

## Current checkpoint

Human playtesting offer, 2026-10-24: ask the user for a focused manual playtest when a concrete source-restoration question needs gameplay evidence. Describe the action and observation needed; no automated gameplay is authorized. Source restoration continues independently wherever possible.

**M2 foundations are complete; M3 is SKIPPED by user direction; M4 decompilation and source restoration is active. No playable source-built game exists yet.** Math/FP, archive/VFILE, paths/config, allocation/entity containers and bounded LEV/PTH/BSP/TXT/WAV/PCX data compose in one source-only integration target. Existing M3 evidence is retained, with missing scenarios explicitly unvalidated; M3 has not passed and does not block source restoration. Automated gameplay and further pilot development are abandoned. Any game execution is limited to a bounded startup test. User resumed implementation on 2026-10-24. Work is command-line/programmatic only.

| Milestone | State | Evidence / next gate |
|---|---|---|
| M0 Reference/build baseline | Complete | `build.cmd`; pinned manifest; clean source checkout without originals passes 14 checks and missing-definition link control; see `m0/clean-checkout.json` and `../BUILDING.md` |
| M1 Provenance/source map | Initial assigned frontier complete | 887-record role catalog; 57 manual frontier decisions, corrected input-data/import boundary, reviewed callback/tail ownership, and nine-function VFILE assignment; `m1/REVIEW.md`. Broader dynamic closure remains provisional |
| M2 Foundations integrated | Complete for reviewed first-scene foundations | 376,611 real-data checks; 36,131 clean source-only checks; 23 TUs; eleven missing-module controls; `m2/EXIT_AUDIT.md`, `m2/final/exit-validation.json` |
| M3 Runnable original reference/scenarios | SKIPPED by user direction, 2026-10-24; not passed | Retain historical evidence in `m3/REFERENCE.md` and `m3/scenario-matrix.json`; missing observations remain unvalidated. No further automated gameplay or pilot development. Does not block M4 |
| M4 First source-driven playable slice | In progress; first textured source-rendered shell view | Real model face flags/bounds, 5,815 placement apertures/bounds and captured portal links reproduce in the linked harness. Full entity/script/render/audio owners remain; no source game target yet |
| M5 Complete single-player | Not started | Requires slice and actual original feature inventory |
| M6 Modern platform support | Not started | Backend choice follows faithful slice; numerical contracts remain open |
| M7 Verified networking | Not started | Active DirectPlay3A references; modes/protocol still need tracing |
| M8 Source-only audit | Not started | Not measurable from current instruction coverage |
| M9 Release/preservation | Not started | No release package |

## Working evidence

M4 recovered camera/frustum/viewport family, 2026-10-24: complete
`004515b0`, `00451660` and `00451b80` compile/link in `src/bh_view_camera.c`.
The 89-TU integration passes 663,020 real-data / 170,786 clean checks and
45 missing-module controls. The 1,623 focused checks cover original default
60-degree FOV/near1/far100000, viewport reset/asymmetric rectangle clamps,
64 rotated translated camera configurations, independent pinhole projection,
world-space planes and original rigid-inverse semantics. Plane boundary checks
account for the half-ULP precision of the original stored float offset.

The scene now uses recovered projection/frustum construction and opts into the
pinned original executable's default FOV/depth range. The saved view emits
5,881 triangles with146 textures and264,231 nonblack pixels; it was inspected.
Native controls retain their explicit90-degree/.25/400 analytical profile and
all28 renderer/46 texture checks pass. Actual original camera choice, visibility,
lighting, scripts and TL-depth/primitive/software dispatch are still unfinished.
The software rasterizer dependency was inspected and retained explicitly, not
stubbed or treated as completed. No original code or game execution.
Evidence: `m4/view-camera/`; audit: `python tools/check_view_camera.py`.
Current iteration checks are complete; user requested commit/push. Next work
remains original producer/dispatch/visibility closure toward scene/entity/control;
human prologue recording remains independent.

M4 original ring clip/project owner, 2026-10-24: complete `00452140`
now compiles/links as `bh_clip_polygon` in `src/bh_clipper.c`. The 87-TU
integration passes 661,397 real-data / 169,163 clean checks and 44 missing-module
controls. The 1,139 focused checks cover both depth and all four screen planes,
paired ring insertion/head/winding, exact boundaries and endpoint duplicates,
projection caches, inverse-camera texture coordinates and 1,024-node guards.
An independent homogeneous Sutherland-Hodgman oracle covers 256 triangles;
a separate six-plane fixture fills all 12 original scratch slots and verifies
the resulting seven-node polygon. No original game code executes.

Both colored and textured scene polygons now use the recovered clipper, replacing
the diagnostic clipping algorithm. Native 28 depth/clipping/pixel controls and
46 texture controls pass, and the real scene renders 397 triangles/146 textures.
The native saved view has 267,844 nonblack pixels. Compared with the prior
adapter, 2,695 colors and three coverage locations change (net +1 nonblack
pixel); this is recorded in `m4/clipper/scene-comparison.json`, not described as
pixel identity or original-game visual acceptance. The output was inspected.
Viewport/camera binding and TL-depth conversion remain explicit adapters.

Image metadata preserves original flag54/coefficient58..6c semantics without
changing its layout. Near/far clips precede division; side clips retain the
reviewed perspective UV/depth formulas and cache behavior. Source guards reject
malformed rings/nonfinite input/invalid inverse and zero unused scratch fields.
PC24, extreme x87 exponent equivalence and byte matching remain unclaimed.
Evidence: `m4/clipper/`; audit: `python tools/check_clipper.py`.
Next: original primitive producer/dispatch closure (including the explicit
unfinished software-rasterizer dependency), then visibility/traversal and
scene/entity/control. Human prologue recording stays independent.

M4 general affine inverse prerequisite, 2026-10-24: complete `00401b30`,
`00401c10` and `00401e50` compile/link in `src/bh_matrix_inverse.c` as LU
solve, scaled decomposition and general affine inverse. The 85-TU integration
passes 660,258 real-data / 168,024 clean checks and 43 missing-module controls.
The focused 3,380 checks cover exact pivot/parity/store vectors, analytical
scale/shear/translation, actual aliases,128 independent Gauss-Jordan oracle
matrices, source transform composition and projected-texture coordinate use.
Zero input rows reject; dependent nonzero rows retain the original success
return and possible nonfinite solution, with no invented epsilon/tiny pivot.
Explicit store boundaries preserve the reviewed PC53 contract; PC24 and byte
matching remain unclaimed. Algorithm resembles common numerical-library LU,
but verified library provenance remains unresolved; no gameplay authorship claim.

Original `00452140` calls this general inverse for projected textures; the
existing rigid inverse fails the nonrigid positive control. This is a recovered
required dependency, not full clipper admission or a changed scene render.
Evidence: `m4/matrix-inverse/`; audit: `python tools/check_matrix_inverse.py`.
Next remains full original clip/project owner `00452140`, then producer and
traversal dependencies. No original code or game execution. Human prologue
recording remains independent.

M4 native primitive draw-buffer ownership, 2026-10-24: complete `004507d0`
and `004508a0` now compile/link as `bh_texture_draw_vertex` and
`bh_texture_draw_flush`. The 83-TU integration passes 656,878 real-data /
164,644 clean checks and 42 missing-module controls. Sixty-one focused checks
exercise original paired growth/copy/count rules, both allocation failures,
mid-polygon rollback/retry, software/empty gates, topology guards and native
mixed triangle/quad fan pixels with real image residency. Native HRESULT/reset
behavior follows the inspected body; failure injection is not claimed.

The scene now submits textures through recovered run queues and the original
Direct3D2 triangle-fan call with flags0x0c. It retains 397 triangles, 146 textures
and 267,843 nonblack pixels. Fan submission changes 1,421 of 307,200 pixel
colors versus the prior repeated-triangle-list adapter. A controlled source
variant changing only that native call back to repeated lists reproduces the
prior bitmap exactly; evidence is `m4/primitive-queue/fan-control.json`. This
isolates native dispatch behavior, not original-game visual equivalence.

The already projected polygon composition helper remains an explicit adapter;
full `0046e390` ring/software production, `00452140` clipping, visibility/lighting,
world/entity construction and control remain unfinished. No original code or
game execution. Evidence: `m4/primitive-queue/`; audit:
`python tools/check_primitive_queue.py`. Next: complete original clip/project
owner `00452140` and its texture-coordinate/vertex-ring contracts, then producer
and traversal dependencies. Human prologue recording proceeds independently.
Earlier exact-bitmap claims below apply to their own historical variants.

M4 original rendering-model ownership, 2026-10-24: complete `00457e20` and
`00458140` now compile/link as `bh_model_read` and `bh_model_load` in the
81-TU integration target. 656,817 real-data / 164,583 clean checks pass with
41 missing-module controls. The focused family passes 8,039 real-data checks,
including 63 synthetic path/lifetime/failure controls and all 505 archived BSPs.
All six owned record arrays compare to independent archive reads; original
0x80 layout, identity transform, zero model runtime fields, pinned root radius,
used-material rules and real shared image-cache bindings are checked. Missing
used images remain NULL without failing the model; opened malformed files do
not trigger search fallback. Source validation safely rejects corrupt input and
cleans failure leaks; no byte match or original-destructor recovery is claimed.

The scene draws directly from the recovered model-owned arrays and material
bindings. The saved image remains pixel-identical (397 textured triangles,
146 textures) to the preceding view. Shell selection, camera/projection and
polygon submission are still diagnostic adapters. No original code or game
execution. Evidence: `m4/model-owner/`; audit: `python tools/check_model_owner.py`.
Next: original primitive producer/consumer dependency family, then traversal,
scene/entity construction and player control. Human prologue recording remains
pending and independent. Earlier image/model-data variants retain their separate
historical source and evidence below.

M4 image/cache/software-palette ownership, 2026-10-24: four complete original
owners (`0044e880`, `0044e9b0`, `0044eb60`, `0044f550`) now compile and link
in the 79-TU integration target. 648,778 real-data / 164,520 clean checks pass,
with 40 missing-module controls. The image family contributes 131 synthetic
checks and loads 454 native-backed images from all 459 PCXs; five are explicit
original size-catalog rejections. Native pixels/palettes match pinned asset CRCs.
Tests cover cache identity, path fallback, original streaming-PCX dialect,
0x70 image layout, draw arenas, software quantization tables, allocation failure
cleanup and native 16/24/32-bit staging pixels. No byte-match claim is made.

The scene now uses the recovered image loader; its saved bitmap is pixel-identical
to the preceding textured diagnostic view. Original BSP/model ownership, primitive
production, visibility/lighting and scene/entity/control behavior remain unfinished.
No original code or game execution. Evidence: `m4/image-owner/`; audit:
`python tools/check_image_owner.py`. Six texture owners have separately recorded
recompiled variants; earlier variants retain their own historical evidence.
Next: original BSP/model owners `00457e20` and `00458140`, then original primitive
production/traversal dependencies toward source scene/entity/control behavior.
Human prologue/target-objective recording remains pending and independent.

The following texture milestone describes its historical source variant; its
full-image-loader WIP status was superseded by the image-owner checkpoint above.

M4 native texture ownership and textured shell view, 2026-10-24: six complete
original format/pool/residency owners (`0044e490`, `0044e510`, `0044e580`,
`0044e5f0`, `0044e780`, `004505f0`) now compile/link in the 76-TU integration
with real SDK surfaces/palettes/Texture2 handles. 646,369 real-data / 164,389
clean checks pass, including 46 texture checks and 38 missing-module controls.
Tests cover below-request format fallback, original mandatory/best-effort
allocation, signed oldest-slot reuse, same-frame EndScene/BeginScene flush,
actual indexed pixels/palette refresh, source/native lifetimes and PC53 retention.

The separate source scene target now loads 146 real textures and raw BSP edge
UVs, emitting 397 clipped textured triangles in the saved corridor view. Native
pixel controls distinguish perspective-correct UVs from affine interpolation;
28 prior renderer controls and all texture controls also pass in a clean checkout
without assets. `m4/texture-owner/scene.png` is the current rendered checkpoint.
The earlier static-color variant and its source/evidence remain separately pinned.
Native RGB uses an explicit system-memory slot-capability binding; original
HAL/video-memory behavior is unvalidated. Original renderer primitive traversal,
lighting and full image/BSP ownership are NOT claimed from the diagnostic view.

Call review corrected the next dependency: BSP textures use `0044f550`, while
`0046f490` is a separate 2D image/rectangle owner. Full `0044f550` remains WIP;
current PCX staging/cache/material binding are explicit source adapters. Original
`00458310` confirms that NULL image materials do not emit a polygon; absent
`ROOF.GIF` is retained as NULL, not replaced with a fabricated image. 202 sidecars
stay unapplied and gameplay/entity/render owners remain unfinished. No original
code or game execution. Evidence: `m4/texture-owner/`; audit
`python tools/check_texture_owner.py`. Next: complete actual texture-image and
BSP model ownership/primitive producer dependencies, then source scene/entity
construction and control. Human prologue recording remains independent.

M4 static scene rendering checkpoint, 2026-10-24: source-only
`build/scene-snapshot/bhunter-scene-snapshot.exe` now loads 454 real block models
and 5,815 TEST.LEV placements, renders the 1,455 portal-bearing shell placements
through native Direct3D2 RGB/DrawPrimitive and saves a 640x480 view. The bounded
view emits 403 clipped triangles and 267,907 nonblack pixels. Twenty-eight native
pixel/depth/clipping/lifecycle checks pass, including in a clean source-only
checkout without original assets. This is an explicit diagnostic adapter, not
recovery of original renderer owners or a playable M4 exit. Colors/FOV/camera
are diagnostic, 202 scripts stay unapplied, and props/entity proxy meshes are
outside this shell view. No wrapper or original code executes. Evidence:
`m4/scene-view/`; reproduce with `python tools/scene_snapshot.py --archive
GAME_DATA/Bhunter.bin`; audit `python tools/check_scene_view.py`.
Next source family: original BSP material/image ownership (`00457e20` and
`0046f490` dependency closure), followed by real textured model submission.
User's manual prologue recording remains pending; no automated play authorized.

M4 deferred sound update, 2026-10-24: user resumed source work and selected
source-built scene rendering as the next observable checkpoint. Complete
`0047c200` now compiles/links in its real TU with real queue/voice owners and
DirectSound interfaces. The 72-TU target passes 646,323 real-data / 164,343
clean checks, including 57 new checks and 12 native Play calls using silent PCM.
Nearest loop ranking/duplication, one-shot retirement, generation reuse,
strict mute boundary, float-spilled Doppler, cached setters and real native
failure paths pass. No audible-game-playback or original loader admission;
full `0047b950` and full physics orchestration remain WIP. Evidence:
`m4/sound-update/`; audit `python tools/check_sound_tick.py`.
Human will record the prologue training-target objective; source work continues
independently. Next: close the real scene load/render path, starting with a
bounded source-owned static geometry view. No automated gameplay authorized.

M4 sound listener/configuration, 2026-10-24: restored complete listener setter
`0047b710` and voice-limit policy setter `0047b920`. The listener stores exact
signed active state, position, velocity and Y-X-Z right vector, now consumed by
real source voice priority. Source native PCM registration can capture the
configured limit (original default 2); old resources retain prior limits. The
70-TU target passes 646,266 real-data / 164,286 clean checks (50 new checks).
Evidence: `m4/sound-listener/`; audit `python tools/check_sound_listener.py`.
Historical queue source/header variants are retained. Deferred playback owner
`0047c200`, full loader `0047b950` and full physics orchestration remain WIP.
No Play calls, audible-playback claim, game or original code execution.

M4 native packet send, 2026-10-24: restored complete `00476d50` transport
wrapper and `00423710` packet-send owner. Real SDK DirectPlay3A Send forwarding,
reliability flag selection, XOR framing and unpadded wire length now link with
real source definitions. The 68-TU target passes 646,216 real-data / 164,236
clean checks (24 new checks). Native unopened-interface Send failures match
direct controls; no provider/session/player was initialized and no send succeeded.
Evidence: `m4/network-send/`; audit `python tools/check_network_send.py`.
The additional SDK header is separately pinned and scoped to two new TUs;
existing VC5 comparison headers/flags remain unchanged. Original session/player
lifecycle, actual delivery, dispatchers and full physics orchestration remain
unvalidated/unfinished. No gameplay or original code execution.

M4 packet codec, 2026-10-24: restored complete world position encoding
`004234d0`, vector compression `00423660`, receive checksum
`004237a0` and player lookup `004239a0`. The 66-TU target passes 646,192
real-data / 164,212 clean checks, including 1,599 codec checks. Golden LE bytes,
all 256 payload lengths, padding/corruption controls, position lattice/error
bounds, the discontinuous upper clamp, real CRT low-EAX conversion and full
32-bit local ID comparison pass. Source physics packets and the saved M3 normal
also exercise the encoder. Existing decoder `00423840` is reused through a
framing bridge, not counted again. Evidence: `m4/network-codec/`; audit:
`python tools/check_network_codec.py`. Framing helpers are adapters; full sender
`00423710`, transport `00476d50`, packet dispatchers and full physics remain
unfinished. No network traffic, game execution or original code execution.

M4 debris physics receiver, 2026-10-24: restored complete response `0043c8f0`
and deferred registry removal `0043c8b0`, using real source entity callbacks,
physics packets, vector math and queued sound. The 64-TU target passes
644,593 real-data / 162,613 clean checks, including 52 new response/lifetime
checks. Analytic blast/torque, rotated body coordinates, Y force float-spill,
strict double audio threshold, masked zero divisions, generation reuse and
retirement order pass. Evidence: `m4/debris-response/`; audit:
`python tools/check_debris_response.py`. Explicit source bindings are adapters;
original debris constructors/tick/render and full physics `004124d0` remain WIP.
Next: full physics side-effect orchestration and actor receivers, with sound
playback/listener and network transport still separate dependencies. No game
execution, audible playback, original code execution or byte-match claim.

M4 cached collision contact, 2026-10-24: restored complete `0044d500`
(`bh_collision_cached_contact`) and routed the real sweep bisection through it.
The 62-TU target passes 644,541 real-data / 162,561 clean source checks,
including saved M3 contact output contracts, 91 sweep, 28 dispatch and 114
physics-response checks. Trial transforms, cached misses and rollback pass.
Evidence: `m4/cached-contact/`; audit `python tools/check_cached_contact.py`.
Entity dispatch remains complete. Full physics `004124d0` stays WIP for actual
actor response callbacks, network transport and effects/audio orchestration;
no whole-owner admission is inferred from numerical tests. Next physics work:
recover a concrete event-2 actor receiver and its dependencies. No gameplay,
original code execution, reference changes or byte-match claim.

M4 sound voice/queue ownership, 2026-10-24: seven complete resource/voice
allocation, priority, request/update/stop and teardown functions now link with
real SDK DirectSound interfaces. The 62-TU target passes 644,538 real-data /
162,558 clean checks, including 112 sound checks. Native PCM upload,
duplication, cursor weighting, actual duplication failure and Stop/Release pass;
streaming lists, 1.25x table growth, double generation increments, signed-handle
validation and dB/frequency values are retained. Zero Play calls, primary/system
volume writes, game launches or original code execution. Evidence:
`m4/sound-queue/`; audit `python tools/check_sound_queue.py`.
Next: deferred playback/update `0047c200` and listener transform `0047b710`.
Full loader `0047b950` and physics orchestration remain WIP; queue admission is
not audible-playback or source-game admission. Functional, no byte matching sweep.

M4 contact effects, 2026-10-24: recovered six particle/impact spawn, tick and
refresh functions plus the three required draw producers and queue reset.
Real source pool callbacks now drive lifetime, movement, growth, camera-facing
impact geometry and ordered draw commands. The 60-TU target passes
644,426 real-data / 162,446 clean checks (57 queue / 30 effect checks).
Allocation failure, release/recycle, shared RNG, full/partial command arenas and
non-dyadic PC53 carry/reload controls pass. Evidence: `m4/contact-effects/`;
audit: `python tools/check_contact_effects.py`. Original draw function pointers
are explicit source semantic selectors; the actual Direct3D primitive consumers
remain unrecovered and are not counted. Full physics `004124d0` stays WIP for
remaining effects orchestration, sound, transport and response/damage callbacks.
No game execution. Functional admission; no byte-match sweep needed.
Next selected family: sound voice allocation and queued requests (`0047b410`,
`0047bc60`, `0047bd40`), with real buffer/interface ownership. Static assessment
is in `m4/sound-queue/assessment.json`; no sound implementation admitted yet.

M4 entity dispatch / physics response, 2026-10-24: complete `0040b410`,
static overlap `0044d480` and world X/Y rotations `00401510` / `004016e0`
now compile/link with the existing real collision owners. The 56-TU target passes
644,339 real-data / 162,359 clean checks, including 28 dispatch and
114 physics checks. Tests exercise broad-phase binding, relative actor movement,
ordered multi-hit scaling, integration → sweep → tangent slide → event-2 packet
production → finish, signed mass rules, material flags, shared CRT RNG and retained
PC53 mass-denominator/reciprocal precision. Evidence: `m4/entity-collision/` and
`m4/physics-response/`; audit: `python tools/check_physics_restoration.py`.
**Full `004124d0` remains WIP:** actual effect/particle, sound, network and entity
response/damage callback owners plus their orchestration are not yet recovered.
Numerical helpers do not stand in for those features or count as full recovery.
No game launches or original code execution.

M4 swept collision, 2026-10-24: complete `0044da20` plus X/Z rotations
`00401610` / `004019a0` are compiled and linked with real tree/face/segment owners.
The 52-TU build passes 644,197 real-data / 162,217 clean
checks, including 91 focused sweep/rotation checks and repeatable sweeps on 454
real models. Original three bisections, cached-face fallback and one bounded
segment refinement are retained. Saved M3 fraction/normal now pass through actual
swept source on explicit analytical geometry; original inputs remain unavailable.
Evidence: `m4/swept-collision/`; `python tools/check_collision_restoration.py`.
Next source boundary is entity/candidate collision dispatch and physics response;
full scene/script/game integration remains open. Zero game launches.

M4 collision traversal, 2026-10-24: recovered `0044c200` / `0044d2f0`
with real BSP bounds, face flags, traversal order and restored ancestor counters.
The 50-TU integration passes 644,105 real-data / 162,126 clean checks, including
29 focused traversal checks and repeatable queries over 454 real models. Rotated
geometry exposed and now verifies a corrected in-place transform in the prior
face helper; its failing control is retained. Evidence: `m4/collision-traversal/`.
Assessment: full face selection is now available; the next dependency closure is
`0044da20` swept bisection, its X/Z rotations and segment fallback. No gameplay.

M4 face contact, 2026-10-24: restored complete `0044cc20` in
`src/bh_collision_model.c` with explicit BSP/transform/normal inputs. The 48-TU
integration passes 644,075 real-data and 162,097 clean
source-only checks, including 191 new face checks / 180 independent interval and
winding cases. Saved M3 contact output is audited and checked on analytical source
geometry; its missing detector inputs prevent an exact replay. Evidence:
`m4/face-contact/validation.json`; audit: `python tools/check_face_contact.py`.
The enclosing swept detector/face selection and collision response remain missing.
No game execution or playable-source admission.

M4 candidate audit, 2026-10-24: all ten active ledger candidates reviewed;
complete collision model loading, optional RIFF scanning and the two buffered
PCX routines now supersede their old subsets. Six larger level/script/render/
sound/image owners remain unfinished. The six timer/timer2 callbacks also now
use real entity allocation, dispatch and cleanup. Current target: 62 TUs,
644,538 real-data / 162,558 clean checks, 31 missing-module controls. Ledger:
90 current linked functions, four comparison variants, fifteen historical
variants, seven unfinished owners including physics. Evidence: `m4/candidate-admission-validation.json`.
No game execution or playable-source-game admission is claimed.

M4 navigation and route consumers, 2026-10-24: full `00407f30`/`00407d90`
source graph construction derives 1,454 nodes / 283 junctions and recomputes every
one of the 80,089 TEST.PTH cells exactly. World owner lookup, portal scratch
center/normal and junction route projection are also linked (`0040a8c0`,
`00433f70`, `00434040`, `00429480`). Current 34-TU build passes 574,362 real-data
and 93,958 clean checks, with seventeen missing-module controls. Evidence is in
`m4/navigation/` and `m4/navigation-consumers/`; full AI/waypoint/entity/script
installation remains unfinished. No game execution. M4 notes are now kept brief;
contracts live in source headers and detailed results in the ledger/JSON.

M4 geometry/ledger-audit continuation, 2026-10-24: five complete geometry face
consumers now have real metadata ownership and linked source. The next selected
world-construction family restores material classification, model/world bounds,
Y rotation and `004084a0` apertures. Source construction of all 5,815 real placements
reproduces the saved 1,454 node bounds, 3,739 portal records, 3,120 links and four
owner resolutions exactly. The 30-TU integration passes 567,512 real-data checks;
the clean source-only checkout passes 93,824. Fifteen missing-module controls
reject. See `m4/GEOMETRY.md`, `m4/WORLD_GEOMETRY.md` and `m4/construction/`.
The ledger audit currently separates 42 linked completed variants, four object-only
byte matches, ten historical/superseded variants and eleven active unfinished
candidates across 53 unique recorded original addresses. Zero are game-admitted;
none of these counts is whole-game completion. Source model grid inputs are explicit
fixture values, not full script execution. No game or original code executed.

M4 resumed, 2026-10-24: restored complete finite-input portal matching
`00408940` as `bh_portals_match` in `src/bh_portals.c`. The real integration
target compiles 25 translation units and passes 434,189 checks with real assets;
a clean checkout without original assets passes 93,709. The 57,578 portal checks
reproduce all 3,739 captured portal records and 3,120 neighbor links, plus ordering,
clipping, double-threshold and malformed-ring cases. The missing-module control
rejects the absent matcher with LNK2001. Ledger variant is `func` / `linked_harness`,
not game-admitted. See `m4/PORTALS.md` and `m4/portals/validation.json`. No game
launch or original code execution occurred. `004084a0` geometry/aperture construction
and full `00407f30` graph ownership remain source dependencies; M4 is not complete.

Plan revision, 2026-10-24: user explicitly directed skipping M3. `RESTORATION_PLAN.md` now removes M3 as a dependency gate and makes M4 decompilation/source restoration the next phase. This supersedes every earlier log entry proposing a next flight, gameplay replay or continued M3 capture. Historical results and missing acceptance evidence are unchanged; no milestone pass, implementation admission or new test execution is claimed by this documentation revision.

- Original English EXE SHA256: `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
- Four matching functions: `analysis/astra-review/matched_math.cpp`; two clean combined builds reproduce 335/335 raw bytes. Direction's isolated form differs; preserve the combined context.
- Positive-control command: `python analysis/astra-review/verify_fixed_math.py`.
- Historical C variant: `m2/math/baseline-bh_math.c.txt`; `analysis/math/differential-results.txt` records 60,000 moderate finite fixtures and five PC24 discrepancies. Current corrected C evidence is separate in `m2/MATH.md`: exact tested finite PC53/64 output bits, one residual PC24 inverse-point discrepancy family.
- Padscan adaptation reviewed against real VC5 output; baseline 4/4 raw matches, source unchanged; declaration changes affect emitted code. Initial evidence: `analysis/padscan/run-b6420c063204/results.json` and `run-2ba29f93c1fe/results.json`.
- Explicit positive-control gate also passed: `analysis/padscan/run-3dc5596e4e33/results.json`. Translation remains exact across all five tested declaration counts.
- Library survey has not identified QvLib or another reusable open-source engine library in BHunter. Required unknown engine behavior must still be covered.

## Open blockers / admission conditions

1. The M2 integration harness links the assigned math, allocation/container, VFILE, path/config and bounded scene/geometry/image/audio data foundations with real owners. Completed data adapters do not supply the original full world/entity construction, scripts, navigation graph, texture surfaces or sound playback. Those source-game consumers and their lifetimes remain M4 work; original-address subset records remain WIP.
2. SP3 LINK 5.010.7303 reproducibly exits `0xC0000139` before diagnostics; exact missing export unresolved. Stock VC5 LINK 5.00.7022 passes the integration build. This isolated historical-tool issue does not block M0; see `m0/sp3-link-probe.json`.
3. Existing matching objects have not been integrated into the functional C harness or a game target. They remain separate implementation variants.
4. The M2 harness explicitly establishes/verifies PC53/nearest/masked exceptions. Original driver/thread precision transitions remain unverified and PC24 inverse-point is unsupported; source-game admission must preserve or resolve this contract.
5. M1's nine assigned VFILE entries now have functional linked variants and explicit safety/lifecycle deviations. The broader static-plus-reviewed-callback graph still has 159 unknown function records and 304 unresolved computed-transfer sites (including COM/register calls). Those are review queues, not gameplay recovery assignments. Review each new frontier before implementation.
6. M3 is skipped, not passed. Startup/feature/media evidence exists, but combat/objective/retry, broader timing/audio lifetime and peer behavior remain incompletely validated. Preserve those uncertainties in the relevant source contracts; do not reopen automated gameplay capture or hold source restoration behind M3.

No new byte-match sweep is needed before M2. No milestone is complete merely because the plan or source file exists.

## M0 exit evidence — 2026-10-23

- `../BUILDING.md` documents Windows/Python/Git and proprietary local VC5/SP3 prerequisites, preparation, relocatable root overrides and scope. `../build.cmd` compiles the real C math translation unit and test driver, links the named target, runs the source-only scenario and requires a missing-module link to fail. No original code mapper is linked.
- `reference-manifest.json` preserves SHA256 and sizes for EXE/BIN/MDF/MDS, the compiler/helpers, supplied stock/SP3 include/lib trees and integration/differential/matching fixtures. All four original reference hashes verified unchanged after work. No reference files were patched or replaced.
- `m0/clean-checkout.json` records the tested Git source tree, commands, host/Python, source hashes, resolved dependencies, 14 checks / zero failures, and the LNK2001 negative control. Checkout uses a temporary Git index and includes current uncommitted work; the user's real index is unchanged. The isolated checkout contains no GAME_DATA, original media or compiler binaries, and builds from another working directory with spaces in its source path.
- `m0/link-map.txt` retains the clean linked symbol/object map. The JSON includes verbose library resolution showing SP3 LIBC and stock KERNEL32 import-library ownership. Generated build products remain ignored.
- CL driver 11.00.7022, C frontend 11.00.7113, C++ frontend/backend 11.00.7303 and stock LINK 5.00.7022 are the declared, hash-pinned combination. The SP3 LINK probe is separately recorded; no end-to-end historical toolchain claim is made.
- Optional matching positive control re-run: two clean combined C++ builds retain 4/4 complete raw matches, 335/335 bytes, no relocations. Direction's isolated mismatch remains explicit in `m0/matching-verification.json`. This evidence belongs only to that C++ variant.
- Implementation ledger functional-C records now name the source-only integration target and preserve historical differential evidence separately. Implementation classifications and game admission did not advance. PC24 discrepancies and runtime precision, media and networking contracts remain open.

## Repository and status tooling

The repository is named `BHunter-revived`. Original game/media, local toolchains, Ghidra projects and large generated runs are excluded. Small curated evidence snapshots are retained under `analysis/evidence/`.

Run `mkstatus.cmd` to regenerate README accounting from the full role catalog and implementation ledger: 887 mapped, 160 excluded helpers/libraries, 727 potential game/engine functions; 90 restored and linked, 7 in progress, 630 not started. The 116 implementation variants include 15 historical/superseded and four object-only byte matches, without inflating recovery. Playable-game admission remains zero. Counts are provisional and do not measure effort or playable completion.

## M1 initial-frontier exit evidence — 2026-10-23

- `m1/REVIEW.md` records scope, startup/scene graph, role/boundary corrections, library disposition, next assignment and admission contracts. `m1/role-catalog.json` promotes all 887 observed function records while leaving 672 roles unknown; it does not relabel them as proprietary gameplay. `m1/review-decisions.json` holds 57 manual decisions; prior math/CRT evidence remains distinct.
- Startup, VFILE, assets/scripts, renderer, simulation, MCI/DirectSound, DirectInput and active DirectPlay roots were reviewed. Selected OS/application/render/progress callbacks are distinguished, including stdcall OS callbacks versus the cdecl message hook. `00450930..00450978` remains part of `00479090`, and the common RET at `0047d680` is excluded from independent gameplay recovery.
- The approximate import-thunk region is corrected: only four JMP/IAT thunks; `0047d380..0047d657` is DirectInput format data. Jump tables, callback data, COM storage, structures, globals and ownership/lifetime limitations are enumerated in `m1/abi-inventory.json` with raw reference evidence.
- Seven complete allocation/file/FP runtime bodies match the pinned SP3 LIBC after masking only actual COFF relocation operands; positive and corrupted-byte negative controls pass. `m1/crt-frontier.json` records the bounded investigation. Helpers and verified CRT are excluded from gameplay assignments. No reusable open-source engine library has been established.
- `m1/first-slice-dependencies.json` has the startup → archive → test.lev/test.pth scene graph and unresolved indirect sites, with an explicit nine-function `archive_vfile` next family. All assigned entries have reviewed boundaries/roles and dependency ownership. This initial gate does not claim full dynamic gameplay closure or complete binary provenance.
- Verified CRT default initialization selects PC53; runtime/driver/thread precision transitions and PC24 discrepancies remain open. Archive shared FILE/cached-handle lifetime, bounded malformed reads, CD music and networking contracts are explicit blockers for later admission.
- `python tools/m1_catalog.py --check` passes catalog/dependency consistency. `build.cmd` still passes 14 source-only checks and the LNK2001 missing-module control. All four original reference hashes verified unchanged. `m1/validation.json` records this block's evidence. Implementation variants and game admission counts are unchanged.

## M2 archive/VFILE block — 2026-10-23

- Contract, role/dependency decisions, tested behavior, safety extensions and remaining limits: `m2/VFILE.md`. Source: `../src/bh_vfile.c` and `../src/bh_vfile.h`. Nine `func` variants recorded in the implementation ledger; six source lifecycle/configuration helpers recorded separately. Target: `build/m0/bhunter-integration.exe`.
- The seven-function core was built/linked/tested before preload and line reading were added. Final combined checkpoint passes 20,475 checks with zero failures. All 1,229 entries and 12,839,409 payload bytes match independent raw reads, covering all six real asset classes. Representative cached assets compare an additional 1,168,813 bytes; real TXT line reads exercise both archive and memory backends.
- Synthetic tests cover valid/malformed/truncated directories, ownership/recycling/cache sharing, callback progress, shared FILE cursor quirks, seek/EOF/partial and overflow reads, guarded line windows and repeated reload/teardown. A clean source checkout runs without original game files. Real BIN verification is opt-in via `build.cmd --archive GAME_DATA/Bhunter.bin`; no original code is read or executed.
- `m2/vfile/real-archive-result.json`, `m2/vfile/link-map.txt`, `m2/vfile-clean/clean-checkout.json` and `m2/vfile/validation.json` retain commands, source/tool/fixture hashes, resolved link ownership and results. Both missing-module controls fail with LNK2001. Original EXE/BIN/MDF/MDS hashes remain unchanged; historical experiment/fixture records are retained.
- This does not complete M2: runtime x87/PC24, remaining container/path/config/payload foundations and source-game lifecycle integration are still open. No renderer/audio/network features were removed and no image mapper was expanded.

## M2 math numerical block — 2026-10-23

- `m2/MATH.md` specifies the supported pinned C profile, original boundaries/dependencies, alias and exceptional-value contracts, bounded investigation, validation and residual limits. Cancellation tests justified grouping fixes in matrix multiplication, point/direction transforms and local translation; no compiler flags or historical C++ context changed.
- Ten new functional-C-PC53 implementation variants link into `bhunter-integration.exe`. Historical C source/evidence is retained separately, as are the four matching C++ objects. Three explicit FP environment adapters are support implementations, not recovered original functions. No source-game admission is claimed.
- The isolated unchanged ten-function mapper ran 188,928 comparisons per precision, 566,784 total. PC53 and diagnostic PC64 have zero finite-bit/return/classification/observed-status/guard/control differences. Rigid inverse has 54 NaN payload differences per precision; payload/sign are excluded from equivalence. PC24 inverse-point still has 3,596 finite-word differences and is unsupported. Original driver/thread precision transitions remain unverified.
- Normal numerical testing uses 4,032 captured original-result cases as data plus analytical and FP scope checks: 28,302 checks pass. The clean checkout without game files passes 29,128 combined checks. The real-data build passes 48,777 checks and all 1,229 payload comparisons. The separately linked archived math variant is rejected by 839 numerical failures while VFILE still passes; both missing-module controls reject absent definitions.
- Evidence: `m2/math/oracle-final.json`, `m2/math/integration-real.json`, `m2/math/link-map.txt`, `m2/math-clean/clean-checkout.json`, `m2/math/validation.json`. All original reference hashes remain unchanged. Full M2 remains open for container/path/config/payload foundations and source-game lifecycle integration; CD audio/DirectPlay requirements remain tracked.

## M2 config/profile-container/path block — 2026-10-23

- `m2/CONFIG.md` and `m2/config/review.json` document the next four complete boundaries/roles and dependency closure. This is a supplemental review; M1's historical initial archive assignment remains intact. Three game-config operations (`00401fd0`, `004022d0`, `00402370`) and the engine-candidate path helper (`00473910`) now have linked functional variants. CRT allocation/stdio/string and Win32 imports are reused, excluded from recovery counts. No shared tail or generic engine pool was assigned.
- Source uses explicit initialized ownership/path/device inputs instead of guessed original globals. The 416-byte settings block and 76-byte profile records retain opaque fields and defaults; links are rebuilt, custom snapshots have real owners and reload failure preserves the old state. Paths preserve original names/separator behavior with explicit bounds. The native adapter reads actual Win32 directories; test persistence never changes machine configuration.
- The new family passes 2,508 checks: default/custom disk roundtrips, singleton and 4,096-node rings, partial lengths/names, per-allocation and late-parse rollback, snapshot lifetime, device-name reconciliation and temporary hardware flag persistence, readonly failure, path limits/native discovery and a path-driven VFILE/config composed fixture. Four missing-module controls reject absent definitions with LNK2001.
- All eleven real translation units compile/link in `build/m0/bhunter-integration.exe`: 51,285 real-data checks pass, including all 1,229 archive payloads. A clean source checkout without originals passes 31,636 checks. Evidence: `m2/config/integration-real.json`, `m2/config/link-map.txt`, `m2/config-clean/clean-checkout.json`, `m2/config/validation.json`. Reference hashes are unchanged; no original code adapter or mapper was added.
- Limits: no actual user-created CFG supplied; tested data are exact default captures and constructed legacy fixtures. No version marker identified, so structural checks are not a version guarantee. Legacy saves include live 32-bit pointer words; reload never trusts them. Source lifecycle/error/bounds behavior deliberately fixes unsafe original failure paths. Rendering enumeration, binding consumers, other engine containers, registry/CD source checks and source-game integration remain open.
- Next M2 gate: review and integrate a bounded real-asset reader family required by the `test.lev`/`test.pth` scene, with real ownership and malformed-input checks. M2 remains incomplete; original runtime precision scenarios, CD audio and DirectPlay requirements remain tracked.

## M2 bounded scene-data reader block — 2026-10-23

- `m2/SCENE.md` and `m2/scene/review.json` record boundaries/roles, file formats, bounds, source ownership and remaining dependencies. The level function `004100d0` mixes decoding and world/entity construction; navigation `00407f30` builds a graph before cache reading/recomputation and calls recursive `00407d90`. Two reader-subset records remain WIP, not completed original functions. Eight explicit data/lifecycle APIs are admitted as source support adapters; no registry/entity/callback/graph stubs were introduced.
- `src/bh_scene.c` owns and validates placement/link arrays and route cells through memory or borrowed VFILE input. It preserves all flags, signed coordinates, opaque link values, file order and exact route bits. Failure retains prior output; VFILE rejection restores cursors where possible. Count/byte products are bounded; malformed names/owners/slots, truncated input, nonfinite/negative distances and mismatching graph count are diagnosed. No format version marker was identified, so structural checks are not called version proof.
- Independent raw archive reads pin TEST.LEV (196,944 bytes; 5,815 placements / 543 link records) and TEST.PTH (640,716 bytes; 283 nodes / 80,089 cells). Every decoded record/cell matches through embedded, preloaded and loose backends. The 69 asymmetric distance pairs and three nonzero diagonals are retained. Expected count 283 is supplied from captured data; a recovered graph has not yet established that count.
- Normal source-only scene checks: 566, including guarded memory boundaries for truncated inputs, every temporary/output allocation failure, cursor/ownership rollback, repeated reload, opaque-value controls and maximum placement/link/route limits. With real data the scene scenario passes 259,927 checks; all thirteen TUs compile/link and the combined target passes 311,212 checks. Five missing-module controls reject absent definitions with LNK2001. Clean checkout without original assets passes 32,202 checks. Evidence: `m2/scene/integration-real.json`, `m2/scene/link-map.txt`, `m2/scene/data-validation.json`, `m2/scene-clean/clean-checkout.json`, `m2/scene/validation.json`.
- No original code executes in the new readers/tests; the math mapper/matching context is unchanged. Original reference hashes remain verified. Full M2 is open: review the block-definition/name registry and bounded source data path next, then actual entity/container and navigation graph ownership. Original reference/precision scenarios, remaining image/BSP readers, source-game integration, CD audio and DirectPlay remain tracked.

## M2 BSP/block data block — 2026-10-24

- Complete loader/registry/script and relevant topology consumer review: `m2/blocks/review.json`, `m2/blocks/original/`, `m2/blocks/consumers/`. Engine authorship is unknown; BSP technique/custom layout is not proof of in-house or Quake lineage. Four original-owner data subsets remain WIP; eleven real data/lifecycle/round-store support APIs are linked adapters, not completed original behaviors.
- All 505 BSPs (3,167,776 bytes) decode through three VFILE backends (1,515 decodes), checking exact six-section bytes, bounded material/index references, iterative acyclic edge/leaf/partition topology and instruction-derived collision/root bounds. The registry loads 454 models, retains 202 scripts explicitly as unapplied, and resolves every one of the 5,815 TEST.LEV placements.
- A bounded numerical investigation exposed mixed x87/float32 collision arithmetic and pinned VC5 excess-precision reuse across volatile stores. `m2/blocks/bounds-investigation.md` retains counterexamples/rejected candidates. A real separate-TU binary32 storage helper preserves observed stores/reloads, with no flag changes, inline asm or original-code execution. Final real-data bounds match independent instruction-derived controls; no universal FP/byte-match claim.
- New source-only BSP/registry checks: 786. All six count caps, deep iterative traversal/cycle rejection, guarded short inputs, invalid references/floats, every nested allocation failure, lifetime/cursor rollback, search order, registry bounds and unapplied script retention are covered. All 17 TUs compile/link; 337,313 real-data checks and 32,988 clean source-only checks pass. Eight missing-module controls reject with LNK2001. Evidence: `m2/blocks/integration-real.json`, `m2/blocks/link-map.txt`, `m2/blocks/data-controls.json`, `m2/blocks-clean/clean-checkout.json`, `m2/blocks/validation.json`.
- Original references remain unchanged; no original-code mapper expanded. M2 stays active. Next: actual entity allocation/container contracts used by registry/scene, then complete the required foundation/dependency exit audit. Script execution, texture/image consumption, complete world/navigation behavior, original runtime precision scenarios, CD music and DirectPlay remain explicit outstanding work.

## M2 entity allocation/list block — 2026-10-24

- `m2/entities/review.json` and complete bodies classify seven actual pool/list/dispatch functions. Nearby `00476d50` is a DirectPlay COM wrapper, excluded from this recovery assignment. `src/bh_entities.c` implements initial/growth arenas, generation/recycle rules, query registration, observed active-chain query traversal, spatial/event-3 filtering, focused/all stepping and zero-argument refresh. Seven functional variants link with three explicit lifecycle/validation adapters; no fake game callbacks or vtables.
- Exact 864-byte layout/offsets compile-check. Initial slots are zeroed; recycled/growth payload retains the original selective-initialization contract. Explicit chunk ownership, 65,535 cap, bad-release/allocation handling, consistent backlinks and callback mutation/reentrancy rules are documented in `m2/ENTITIES.md`. Original RET sentinel is NULL/inactive, not an empty source function. Spatial queries retain observed PC53/float-store widths with 768 independent numerical boundary cases.
- Entity scenario: 819 source-only checks; 12,454 with real placement data. The observed 9,600-slot arena acquires/releases all 5,815 placement records without invented gameplay callbacks. All 19 TUs compile/link, 349,767 combined real-data checks and 33,807 clean source-only checks pass. Nine missing-module controls reject LNK2001. Evidence: `m2/entities/integration-real.json`, `m2/entities/link-map.txt`, `m2/entities/query-controls.json`, `m2/entities-clean/clean-checkout.json`, `m2/entities/validation.json`. Original references and math mapper/context remain unchanged.
- The full `m2/EXIT_AUDIT.md` keeps M2 active: PCX image and WAV sample readers required by the slice are not admitted yet. Full world/script/navigation and texture/audio runtime owners remain unimplemented; CD music, DirectPlay and original numerical/runtime scenarios remain tracked rather than removed.

## M2 bounded WAV data block — 2026-10-24

- Complete bodies of pure RIFF scanner `0047b830` and DirectSound/resource owner `0047b950` were reviewed; provenance remains engine/library candidate, not established proprietary authorship. The bounded all-output scanner and owning PCM decoder are five source data/lifecycle adapters. Two original-address subset records stay WIP: optional scanner ABI/early exits and complete playback/resource ownership are not supplied. Contracts and differences: `m2/WAVE.md`, `m2/media/wave-review.json`.
- All 60 real WAVs use mono PCM, 11025/22050 Hz, 8/16 bits. Independent struct-based controls include all metadata, chunk spans and sample fingerprints. All 180 backend decodes compare 5,762,313 sample bytes exactly. LIST/cue chunks are traversed safely; no cue/playback semantics or sound objects are fabricated.
- WAV scenario passes 226 source-only and 1,791 real-data checks, including guard-page truncations, RIFF/chunk length overflow, codec/frame/extension validation, unknown/duplicate/reordered chunks, allocator and cursor rollback, exact sample bytes and reload/disposal. All 21 TUs compile/link, 351,558 combined real-data and 34,033 clean source-only checks pass; ten missing-module controls reject LNK2001. Evidence: `m2/media/wave-integration-real.json`, `m2/media/wave-link-map.txt`, `m2/wave-clean/clean-checkout.json`, `m2/media/wave-validation.json`.
- Original references remain unchanged and no original-code mapper or math comparison context changed. M2 stays active. Next: bounded PCX/RLE image data contracts used by `0044f0a0`/`0044efa0` and `0046f490`, then the complete foundation gate audit. Texture surfaces, sound playback/resource ownership, game/script/navigation, CD music and DirectPlay remain separate requirements.

## M2 PCX/palette block and full exit — 2026-10-24

- `m2/PCX.md` and `m2/media/pcx-review.json` record complete stream, palette and image-owner boundaries before coding. `src/bh_pcx.c` owns raw indexed pixels/palette with a bounded original-dialect decoder and borrowed VFILE lifecycle. The complete `0044f1c0` integer palette/index transformation is a linked functional variant with the observed identity-map loader precondition; three larger original entry/owner subsets stay WIP. Four source data/lifecycle helpers are adapters. No DirectDraw surface/cache stub is supplied.
- All 459 actual PCXs use v5 indexed 8-bit RLE, one plane, zero origin, unpadded rows and final palette marker. All 1,377 backend decodes preserve exact independently decoded pixels/palette, plus four full-image palette/index transformations. The source-only scenario passes 2,098 checks, including every truncated protected-page prefix, RLE/dialect/version/cap errors, exact 4096x4096 cap, allocator/cursor rollback, 96 independent palette fixtures, aliasing, thresholds, ties and duplicate keys. Real PCX scenario passes 25,053 checks.
- All 23 real TUs compile/link; the combined real-data run passes 376,611 checks and the clean source checkout without original assets passes 36,131 checks. All eleven missing-module controls reject LNK2001. Original EXE/BIN/MDF/MDS hashes are unchanged. The unchanged four-function C++ comparison artifact separately reproduces 335/335 raw instruction bytes in two builds; none of that byte evidence is transferred to linked C variants.
- The complete M2 gate is now **passed**, not inferred from source counts. `m2/EXIT_AUDIT.md` maps every plan requirement to evidence and explicit limits. `python tools/check_m2_exit.py` validates identical real/clean/current source hashes, real symbol owners, all assigned scenarios and real asset classes, isolated original-code boundary, historical variant separation, numerical controls/captures and preserved references. Durable report: `m2/final/exit-validation.json`. Family evidence: `m2/media/pcx-validation.json`, `m2/media/pcx-integration-real.json`, `m2/media/pcx-link-map.txt`, `m2/pcx-clean/clean-checkout.json`.
- Handoff is to M3 original reference/scenario/media evidence, then M4 world/script/navigation/texture/audio source-game consumers. No playable source game, universal malformed-input guarantee, modern toolchain/backend admission, original runtime precision equivalence, CD audio deletion or DirectPlay replacement is claimed. Those obligations remain in the restoration plan and source-game admission conditions.

## M3 media/source-check reference block — 2026-10-24

- User instruction: no computer-use skills or visual identification. After that instruction, work uses command-line/native debugger traces only; no UI control, screenshots or visual inspection. The original reference remains separate from source integration.
- `tools/m3_media.py` validates pinned MDS/MDF, reads the single-session track table and Mode1/Joliet filesystem, fingerprints both complete track spans and 1,495 files. Real Copyright.txt is recovered under ignored build staging; its complete-u32 XOR matches 6b405d08 exactly. Track 2 contains 98,716 stored audio sectors (21:56.2133); CD audio is available outside the BIN. No installer/legacy driver ran. Evidence: `m3/media-inventory.json`.
- Separate native source-check probe compiles/links and actually runs a byte-identical original EXE copy through the Windows loader. Hardware-only trials timed out and are retained as failed evidence. Revised two-breakpoint instrumentation restores/verifies original instruction bytes; only two source-registry call arguments redirect to a fresh private HKCU key, removed afterward. Original check returns 0 for missing registry, 1 for recovered copyright, 0 for a privately altered byte. Each first return is captured before WinMain UI creation. Early x87 controls are 027F before/after all three cases. This does not establish renderer/thread/gameplay numerical state.
- `m3/DEBUGGER_DESIGN.md`, `m3/source-probe.json` and the reference-diagnostics ledger entry isolate this original-backed experiment. It is absent from `bhunter-integration.exe`; there is no normal-target/original mapper expansion, source implementation admission or disk patch. Original HKLM views and all four reference hashes are unchanged. Probe termination is not orderly game shutdown.
- Static complete MCI/caller review identifies actual track 2, 00:00..21:50 play range, completion-notify looping, stop/close state and device-failure handling. `m3/cd-contract.json` distinguishes this from unverified runtime/audible playback. Full M3 remains active: isolated configuration/render bring-up, actual features/modes/controls and deterministic startup/menu/loading/gameplay/failure/retry/shutdown evidence remain uncaptured in `m3/scenario-matrix.json`.

## M3 isolated config/graphics startup block — 2026-10-24

- Native CLI debugger now runs the original beyond source validation with a private Windows-directory path adapter. Original config load/bootstrap/save execute and write an actual 644-byte CFG (416-byte settings plus three profiles). The first real acceleration-prompt text is captured as API arguments; the dialog never executes. No computer-use, screenshots, visual identification or UI input is used. Original/System CFG/INI files and HKLM views remain unchanged.
- Explicit software-high preset retains the original-produced CFG except the measured configured/hardware/resolution fields. Full native normal-show control attempts six 16/8-bit modes and reaches the original Fatal Error / No suitable screen mode. First DirectDraw SetDisplayMode returns 80004001 (E_NOTIMPL); this is real failure, not a source or dialog stub. Earlier short timeout trials are retained as incomplete observations.
- The official DDrawCompat v0.7.2 prerelease asset is SHA256-pinned and staged only beside separate local reference copies. With identical preset/show mode and the same C probe variant, actual mode setup returns S_OK, renderer returns 1 and original WinMain reaches 00479200. Probe stops before game bootstrap executes. No normal-target/original-mapper expansion or M6 backend choice is implied. Exact source snapshots, logs and dependency hashes are retained.
- Evidence/limits: `m3/STARTUP_DESIGN.md`, `m3/REFERENCE.md`, `m3/startup/c57437263011.json`, `m3/startup/62d00616a929.json`, `m3/startup/e1ee374903ef.json`, `m3/compat/ddrawcompat-v0.7.2.json`, `m3/startup/validation.json`. `python tools/check_m3_startup_evidence.py` passes. Startup x87 remains 027F at captured boundaries; later worker/gameplay state is unmeasured. M3 stays active for actual bootstrap/scene/menu/input/gameplay/audio/network/shutdown traces; goal scope is unchanged.

## M3 original bootstrap/world-loading block — 2026-10-24

- Opt-in `--through-bootstrap` runs the original to its actual return of 1 at `004733f2`. Seven stage checkpoints and their owning direct calls are verified. Real DirectSound init returns 1. DirectInput returns 0 with an actual interface and null joystick pointer, consistent with its no-selected-joystick branch; keyboard/mouse behavior is unmeasured.
- Actual world loading reaches 454 models, 283 navigation nodes and 7,831 used entities in the original 9,600-slot arena. Registry/level/navigation intermediate used counts are 755/7,796/7,797. Main-thread x87 controls stay 027F at all captured stages. No game rendering worker starts before this stop; initial system-thread controls are recorded without claiming later precision.
- Explicit diagnostic cursor guard returns FALSE for three SetCursorPos requests and never fabricates input success. aux/wave volume guards stop before system-wide writes; neither is reached. All 79 breakpoint bytes restore/verify. Original references, Windows CFG/INI and original registry views are unchanged; fresh private keys are removed. Original-backed diagnostics stay outside normal source integration and do not count as recovered source functions.
- Final compiled/linked/run variant: `m3/startup/f7a780ff1c4d.json`; preceding variant `m3/startup/89994fa7c8f0.json` retained. `python tools/check_m3_bootstrap_evidence.py` audits actual stages, variant hashes, ownership boundaries and preservation. `m3/bootstrap-validation.json`, `m3/REFERENCE.md`, `m3/STARTUP_DESIGN.md` and `m3/scenario-matrix.json` retain limits. No computer-use, screenshot, visual identification or input injection.
- Full M3 remains active. Next: actual menu/control/seed/time/state capture and bounded gameplay traces. Sound-device success is not audible playback; CD looping/network modes and normal shutdown/reload still need evidence. Diagnostic termination at bootstrap return is not game shutdown.

## M3 first simulation/render/Flip block — 2026-10-24

- The previous turn made concrete bootstrap/world-load progress. This block extends the CLI-only original trace to the actual game render worker and first main-thread simulation. Reviewed complete callback, input-binding allocation, CRT allocation/heap and shutdown boundaries remain diagnostic evidence, not recovered source counts. `m3/FIRST_FRAME.md` records roles and ownership; the render callback's shared tail stays owned by `00479090`.
- Default heap profiles fault at original `004755ce`, a deliberate write to address 4 when a binding-list pointer has a nonzero high nibble. The pointer is valid committed private read/write memory above 256 MiB. Failed first/second-chance traces are retained, including the earliest variant without verified breakpoint-restoration evidence. Later failure handling restores/verifies bytes before child termination.
- Per-child bottom-up-ASLR-off trial fails at the same check; actual policy flags are zero in both it and the default control. That hypothesis is rejected for these runs. A bounded explicit HeapCreate initial-size input adaptation (4 KiB to 128 MiB, flags 1/max 0 unchanged) lets the real first simulation/render/Flip execute; original allocation/free APIs and assertion remain intact. No allocation pointer, list/global or success result is injected. The adaptation is a reference-profile limit, not a shipping decision or guarantee for later allocations.
- Final same-source/same-CFG control `m3/startup/b8497ba5a485.json` fails as observed; adapted case `m3/startup/75fef4730703.json` completes first simulation tick 0→1, original render callback and actual surface Flip with HRESULT 0. The game worker is identified from actual CreateThread entry `00471e00`. Both main and render controls are 027F at captured callback/Flip boundaries. Debugger timing/cursor guards preclude deterministic-input/equivalence claims.
- `python tools/check_m3_runtime_evidence.py` audits actual controls/returns, memory-fault context, source snapshots, registry/Windows files and original preservation. Original references and normal source integration are unchanged. No computer-use/visual tooling or input injection. Full M3 stays active for actual menus/controls/feature inventory, deterministic seed/time/state and movement/collision/objective/failure/retry, playback/looping, networking and normal shutdown/reload.

## M3 menu inventory and child-input block — 2026-10-24

- Previous turn made verified first-simulation/render/Flip progress. Current bounded CLI trace completes 300 idle callbacks and captures actual constructed menu objects: 18 menus / 118 entries. GAME/NETWORK/SETUP/QUIT, three ships, nine episode labels and video/audio/input/network pages are actual runtime label/link/callback data, not screenshot inference or full feature execution. Reviewed menu-family boundaries/roles and complete exports are in `m3/menu-review/`; no source recovery counts change.
- Child-only key messages now exercise original GAME → STINGER → prologue callbacks over 600 simulation returns. Every PostMessage HWND is checked against the created child PID; original key-array transitions and active-menu/selection preconditions are recorded. Actual `0042ca10` receives index 0 and stores the observed action flag/index. No desktop-wide input, computer-use skill or visual identification. The original selected ship is 1/STINGER; the trace uses that selection rather than patching it.
- Denied warps previously caused repeated absolute mouse offsets and selection cycling. An explicit relative-coordinate adapter now executes native GetCursorPos, preserves its success result and translates observed physical deltas only in the original poller's stack POINT. Native failure stops the probe. Original following instructions and callback returns are stepped/rearmed with byte/length checks; no gameplay instruction, callback or success is substituted.
- Earlier incomplete paths and guarded failure remain separate: `9e7445abb9a7` drifts/returns from episode page, `1ad993c8a666` follows wrong fixed-Up schedule, `3e8c09b297f2` rejects assumed ship 0 and has no verified restoration event. `1a9aaba8c7d0` is the preceding successful variant; final `73beffa6a63f` records the explicit action-flag name and actual requested-path result. The legacy `start_pending` field name does not establish a transient pending-operation meaning.
- `m3/MENU_TRACE.md`, `m3/menu-inventory.json`, `m3/menu-validation.json` and `python tools/check_m3_menu_evidence.py` retain inventory, native input/callback results, source snapshots, unsuccessful variants and limits. All originals, Windows CFG/INI and original registry views remain unchanged; private keys are removed and final instrumented sites restore/verify. Source integration remains unchanged. M3 stays active for mission initialization and actual movement/collision/objective/failure/retry, deterministic world/seed/time, playback/loops, network modes and normal shutdown/reload.

## M3 bounded player/contact/shutdown evidence — 2026-10-24

The repeated prologue route is consolidated in `m3/PLAY_TRACE.md` and `play-validation.json`. Same-source motion/idle controls record actual acceleration and displacement; the final separately instrumented variant captures a real player collision. Five runs complete original abort/menu/quit, six cleanup returns and natural process exit 0. All references remain unchanged. Further launches stopped after the user flagged repetition. This is original-backed evidence only, no source admission or M3 completion. Objectives/failure/retry, saved-config reload, deterministic world replay, audio/CD looping and network modes remain open. `python tools/check_m3_play_evidence.py` audits retained artifacts without launching the game.

## M3 failure/retry contract review — 2026-10-24

No game launches. `m3/FAILURE_RETRY.md` and `m3/failure-contract.json` separate actual destruction-counter/menu/respawn branches from previously captured abort. Complete original mission-event callback identifies the historical `failure_raw` field as ending/control state (completion event writes 1; separate event writes 2), not a general failure flag. Read-only audit verifies 48 instruction records and two constants against immutable EXE bytes. Runtime failure/retry remains missing; no M3/source admission count changes.

## M3 mission-event producer review — 2026-10-24

Zero game launches. `m3/MISSION_EVENTS.md` and `m3/mission-inventory.json` connect parser broadcast opcode 18, its real event-0x0e entity-query branch and the mission callback. Raw LEV sensor/relay slots locate candidate introductory/ending events with coordinates and script text. The installed script callback at 00443ff0 was missing from decoded function boundaries; read-only instruction inspection separates its provisional code from jump-table data, with ownership unresolved. Progress persistence, world event-0x0f and delayed sound side effects are separately reviewed. `python tools/m3_mission_inventory.py` validates pinned code/data and archived artifacts. No source admission or runtime objective/failure/retry completion is claimed.

## M3 saved configuration reload — 2026-10-24

One focused pre-game launch reloads the exact CFG saved by verified natural quit, without menu/prologue replay or input. `m3/CONFIG_RELOAD.md`, `m3/reload-validation.json` and run `c42a7051b694` prove all 416 settings bytes and three profile non-pointer payloads survive; original links form a new reciprocal ring and baseline pointer words change. Original bootstrap disk-save zero-word versus restored-memory behavior is explicitly verified. New read-only snapshot variant compiled, linked and executed; historical play evidence stays bound to older snapshots. Protected files/registry and originals remain unchanged. Bounded shutdown/config reload is captured; M3 remains active.

## M3 native media/transport availability — 2026-10-24

No game launch. A separate real x86 source diagnostic compiles/links/runs MCI and DirectPlay3A queries. Zero CD-ROM drives, no aux devices, two wave outputs; original-style cdaudio open fails 266/MCIERR_CANNOT_LOAD_DRIVER, a concrete blocker for current original CD playback/loop capture despite preserved MDF track 2. DirectPlay3A creation and provider enumeration succeed for TCP/IP, serial and modem; a missing legacy API cannot be claimed as the network blocker. Actual original sessions/peers remain missing. `m3/HOST_AVAILABILITY.md`, `m3/host-validation.json` and `python tools/check_m3_host_evidence.py` preserve results, separate diagnostic success from feature validation, and verify original GUID/file/registry invariants. No playback, volume write, network session or source-game admission.

## M3 exposed feature/control inventory — 2026-10-24

No game launch. `m3/FEATURES_CONTROLS.md` / `m3/feature-inventory.json` tie actual 18-menu/118-item objects to all 44 profile control bytes and 132 values across three loaded profiles, using original key/button/axis names. Four display callbacks confirm field mappings including swapped camera axis ordering. Actual labels expose five primary/five secondary weapons, six cameras, three ship choices and nine episode slots; those counts are not completed gameplay, unlocked missions or player limits. `python tools/m3_feature_inventory.py` reconstructs and verifies the inventory. No source-game admission; M3 runtime gaps remain explicit.

## M3 batched selected-feature execution — 2026-10-24

One launch combines mission event observation, selected primary/secondary fire dispatch and camera control, then natural quit. `m3/FEATURE_TRACE.md` / `m3/feature-trace-validation.json` verify actual Ctrl/Space dispatch/return (primary 0, secondary 9/static coating; one held callback is known common RET sentinel) and F1 camera index 1→0. F2 reselects current 1. Original mission consumer receives 9991/3332 during prologue initialization; objective/ending IDs remain uncaptured and sampled destruction counter stays zero. New diagnostic variant compiles/links/runs, original cleanup/exit 0 verifies, references remain unchanged. No follow-up replay, projectile/damage/audio/visual claim or source-game admission. Older reload/play audits validate their own source snapshots.

## M3 actual damage-trigger contract — 2026-10-24

No game launch. Complete player event callback 0041d7c0 connects real physics event 2 and direct-damage event 0 to separate transient/permanent fields, protection/cooldown gates, 0.995 transient decay and destruction-counter trigger. A contact-detector hit alone does not establish damage delivery, and one maximum-severity ordinary collision contributes only 0.2 transient damage; repeating the single-wall-contact route would not be a justified failure test. `m3/DAMAGE_TRIGGER.md`, `m3/damage-contract.json` and `python tools/check_m3_damage_contract.py` verify 48 instruction records/eight constants against immutable EXE. Actual combat/impact damage, destruction and retry remain unvalidated; no source admission.

## M3 original DirectSound API playback boundary — 2026-10-24

One bounded idle launch with no gameplay/menu keyboard input captures original primary-loop and secondary-one-shot DirectSound Play calls and real S_OK returns. Sound-start only queues; actual backend calls occur in initialization/deferred update owners. Loop-secondary/status sites were armed but not reached before the 300-callback stop; no playback lifetime, waveform/audibility or CD acceptance is fabricated. `m3/SOUND_TRACE.md`, `m3/sound-validation.json` and `python tools/check_m3_sound_evidence.py` verify new variant compile/link/execution, call arguments/results, restored instrumentation, reference/private-file invariants. Child stop is diagnostic. No second launch, source integration change or M3 completion.

## M3 original TCP/IP setup and retained stall — 2026-10-24

One NETWORK/STINGER/TCP-IP route uses a private saved-CFG fixture changed only to 127.0.0.1 and verified-child key messages. Original provider selection/address construction execute; actual InitializeConnection and first EnumSessions return S_OK with zero sessions, and owning initializer returns 1 with real interfaces. Subsequent simulation callbacks stop; 45-second watchdog terminates child before session-list/600-callback acceptance. No breakpoint-restoration event exists for timeout, explicitly retained; original/reference/protected-file/registry invariants verify. `m3/NETWORK_TRACE.md`, `m3/network-validation.json` and `python tools/check_m3_network_evidence.py` separate evidence-audit success from scenario failure. New network path stays WIP; no second launch, host/join/peer or M3/source-admission claim. Next diagnosis requires actual thread/module evidence at the stall rather than identical replay.

## M3 TCP/IP session-list capture and debugger control — 2026-10-24

One revised diagnostic replay completes original loopback TCP/IP setup and session-list activity through 600 callbacks, both API results S_OK/zero sessions, with verified restoration. Module/thread lifetime recording was added for stall diagnosis; the old stall does not recur and its cause remains unresolved. A separate source-built waiting-child control executes the real stopped-thread capture and breakpoint restore helpers on three threads; compilation/link/execution passes without original game code. `m3/NETWORK_SETUP.md`, `m3/network-setup-validation.json`, `m3/timeout-control.json` and `python tools/check_m3_network_setup.py` separate bounded setup success, older failed variant, and control scope. No extra game replay for the control, host/join/peer acceptance, source-game admission or M3 completion.

## M3 original TCP/IP host creation — 2026-10-24

One bounded network-host trace advances actual new-game action through original DirectPlay Open CREATE and server-player creation: both S_OK, player ID 1, helper EAX 1/host-ready 1/mode 2. Actual descriptor requests capacity 8 (not an eight-peer test). `m3/HOST_SESSION.md`, `m3/host-session-validation.json` and `python tools/check_m3_host_session.py` verify new variant compile/link/execution, native API results rather than unconditional mode flag, checked input, restoration and reference/private-file invariants. Join guard remains active. Stops after host callback return; no continued hosted gameplay, peer exchange or original session-shutdown claim. Prior setup stall remains unresolved; no M3/source admission completion.

## M3 original introductory sensor activation — 2026-10-24

One keyboard-flight attempt reaches actual sensor placement 4939 world bounds and event 2010 executes/returns through original mission consumer at callback 544. Native diagnostic policy calibrates actual steering signs, reads actual pose/velocity and posts verified-child keys; no position/health/mission-state or event injection. The original zero-range sensor uses resolved owner bounds, not raw model radius/placement alone. `m3/INTRO_SENSOR.md`, `m3/intro-sensor-validation.json` and `python tools/check_m3_intro_sensor.py` verify 23 recorded flight observations, key-state transitions, bounds entry, sender/event/return, source/header compile-link-execute provenance and unchanged references. Intro activation is a new captured scenario; objective/episode completion and destruction/retry remain open. No second attempt, visual tools, source admission or M3 completion.

## M3 uninterrupted training handoff and scoped repeatability — 2026-10-24

One mission-tour flight visits original sensors 4939/5055/5064, delivering 2010/1100/2011 with actual handler returns at 544/601/655, without restart between waypoints. Runtime bounds entry and keyboard-only policy are verified; no gameplay/event writes. Original briefing/LEV/launcher data identify the later slow-target objective without claiming spawn/kill/completion. Existing independent intro and tour runs also match 20 selected raw player samples, 37 key events and 23 printed flight observations in their common prefix; this is scoped frame-indexed repeatability, not universal determinism. `m3/TUTORIAL_HANDOFF.md`, `m3/tutorial-handoff-validation.json`, `m3/intro-prefix-trace.json` and the two checkers retain provenance/limits. No extra replay for comparison, normal source admission or M3 completion.

## M3 tutorial target creation/damage boundary — 2026-10-24

Zero game launches. Read-only review identifies immediate target allocation inside `0043a920`, distinct from the launch wrapper's unreliable EAX return. The type-6 target uses curve-evaluated positions rather than the player's `+0c` position contract. Its damage receiver admits player-attributed damage scaled by 0.8/capped near 0.45, followed by update decay and actual destruction-path broadcast of stored death ID. `m3/TARGET_CONTRACT.md` defines a single attributed spawn/motion/hit/kill observation chain before another runtime attempt. `python tools/check_m3_target_contract.py` verifies ten complete body fingerprints, 48 instruction anchors, seven constants and three dispatch-data spans against the pinned EXE. Roles are classified before decompilation, CRT excluded, engine provenance unresolved. No runtime spawn/damage/kill, source implementation admission, M3 completion or new repeated route is claimed.

## M3 actual target spawn and failed motion probe — 2026-10-24

One `mission-target` launch captures the real allocator result, completed constructor and original script launch return for placement 4827/type 6/bounty 5000/speed 10/death ID 2013 at callback 655, before original event-2011 consumer entry. At 660 the same target's curve/phase/random/navigation data changed; the diagnostic then failed an incorrect lifetime-equality assertion on mutable `+1a8`. Verified restoration/cleanup and original preservation pass; the complete scenario remains failed. `m3/TARGET_SPAWN.md`, `m3/target-spawn-validation.json` and `python tools/check_m3_target_spawn.py` separate actual spawn evidence from unvalidated evaluated motion/damage/kill. The corrected predicate separately compiles/links with snapshots in `m3/target-probe-build.json`, but has not run. No second launch, source-game admission or M3 completion.

## M3 original target motion and unsuccessful combat approach — 2026-10-24

One new `mission-combat` launch executes the corrected identity predicate and captures 32 actual original event-3 target positions at callbacks 655–686, with 6.8780752736 units displacement. The combat policy remains in its climb phase across 191 observations through callback 1796; two brief thrust pulses occur, no fire input, ammo remains 300. The 1800-callback combat acceptance fails truthfully, with verified restoration/cleanup and no original exception. `m3/COMBAT_ATTEMPT.md`, `m3/combat-attempt-validation.json` and `python tools/check_m3_combat_attempt.py` separate accepted bounded target-motion evidence from unsuccessful approach and missing shots/hits/kill/retry. A source-only 18-coordinate control validates diagnostic curve fixtures, not original numerical equivalence; no source-game admission. No second launch. Next attempt requires resolving the policy's unsupported vertical-flight/alignment assumption, not repeating it unchanged. M3 remains active.

## M3 forward-sign correction and retained route failure — 2026-10-24

Read-only gun-vector/input/projectile review establishes positive basis.forward for thrust and shot direction: negative original -30/-10/-150 coefficients and -1/30 time constant invalidate the diagnostic's inherited negative-forward convention. Seventeen instruction anchors/four constants pin this contract. The corrected combat policy reverses its calibration response signs, removes the climb prerequisite and records full orientation/gun/velocity context; source-only Cartesian direction/turn fixtures and native TU compile/link pass. One corrected original run `469b5ddce4f6` verifies actual gun vector equals positive forward basis and captures acceleration requests in 181 policy samples, but remains near the start and never reaches firing range. The 1800-callback scenario fails with verified restoration/cleanup and no original exception. `m3/FORWARD_CONTRACT.md`, `m3/forward-contract-validation.json`, `m3/forward-attempt-validation.json` and their checkers separate the corrected sign contract from unsuccessful pursuit. No second launch, collision-cause claim, source-game admission or M3 completion. Next work is real level/navigation exit ownership and passable routing, not another unchanged direct approach.

## M3 completed navigation graph and candidate portal route — 2026-10-24

One startup-only launch captures the completed original graph, with no menu/flight replay or input: 1,454 nodes including 283 junction/end nodes and 1,171 corridors, 3,739 portals / 3,120 connected reciprocal records. The runtime matrix exactly matches `TEST.PTH`'s payload. The old raw navigation count 283 is junction count, not all graph nodes. Reviewed portal construction/matching owners establish real pointer/slot/extents ownership. A 15-edge candidate route connects the last introductory sensor's owner to the initial target-launch area, and its four junction choices match the original matrix. `m3/NAVIGATION_REFERENCE.md`, `m3/navigation-route.json`, `m3/navigation-validation.json` and `python tools/check_m3_navigation.py` preserve provenance, source/header compile-link-execute evidence and original preservation. No ship-clearance, obstruction cause, route flight, target pursuit/kill or source-game admission is inferred from graph connectivity. M3 remains active; next flight must follow real portals/current owners rather than the failed direct line.

## User stop and withdrawal of automated gameplay — 2026-10-24

**NEXT STEP ON RESUMPTION: DECOMPILATION AND SOURCE RESTORATION.** Select a missing function or small dependency family, recover its source, compile it in the real translation unit, link with real definitions into a named target and execute relevant source-built checks. Record the resulting source implementation and its actual integration/validation evidence. Do not resume automatic-play controller work or substitute further reference-scenario collection for source restoration. This directive supersedes the earlier next-flight and pursuit plans above.

Work is paused at the user's request. The user explicitly abandoned all automatic gameplay and limited any game execution to a bounded startup test. All earlier proposed menu/tutorial/flight/combat/network scenario replays and the proposed portal-following run are superseded and must not be resumed. The saved portal-controller edits are unvalidated, abandoned diagnostic work, not admitted restoration; do not continue developing or testing that controller. Existing historical evidence is retained without changing its scope. M3 remains incomplete with no source-game admission from these diagnostic blocks. On resumption, focus on source implementations, real translation-unit integration and source-built checks within the startup-only execution restriction. No game was launched for this update.
