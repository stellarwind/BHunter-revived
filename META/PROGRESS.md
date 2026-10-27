# BHunter restoration progress

Updated 2026-10-27. Plan: `../RESTORATION_PLAN.md`.

## Current checkpoint

Custom-box training sequence completed and executed, 2026-10-27. User's exact
note is now at the top of AGENTS.md and RESTORATION_PLAN.md: ALL TEST HAPPEN IN
A CUSTOM TEST LEVEL. NEVER EVER TRY SCRIPTED SCENARIOS IN THE REAL GAME!! AND
EVEN IF YOU NEED THOSE ASK HUMAN TO RECORD KEYS.

Two hardware-only881-tick box runs pass with identical recorded traces. First
target kill136; station sensor/second-target spawn170; five missiles collected174;
faster second target killed551; original ten-second relay starts next assignment
at851. Checks require actual pickup depletion, lock, missile projectile presence,
both death events, progression0->1, live next target, and continued frame updates.
The pickup/sensor timers are independent: station activation may precede the
actual pickup by four ticks; collection precedes missile firing and second kill.

The relocated POWERUP/MARKER/SENSOR2/LAUNCH2/RELAY dependency chain uses production
VM/wait/sensor/marker/actor/weapon/story owners. The disappearing marker causes
the actual cached successor to recycle1->3. A separately compiled old-guard
variant fails in THIS BOX at170 with step=MUTATION and active successor, matching
the human log. Production source was not changed for that negative control;
the fixed box executable is restored and hash-verified. No actual training or
mission automation, software-renderer test, or reference gameplay ran.

Evidence: `m4/playable/training-box/validation.json`, paired traces, original
script/placement/instruction contracts, negative-control log/build, and source
snapshots. Per-implementation ledger refreshed. The straight box is a relocated
scenario, not full original-level or PC24 equivalence. A nonfatal degenerate-curve
draw diagnostic remains. Earlier all-weapon evidence remains historical; latest
box test covers this two-target chain. M4 remains incomplete.

### Previous checkpoint

User correction, 2026-10-27: removed the mistakenly introduced actual-training
flight controller, runner, input hooks, story observer, command-line option and
generated training-run artifacts. No automated training/mission playthroughs.
Required scenario coverage belongs in the separate test box level. The removal
build compiles/links only; no game launch is requested for this cleanup.

The rocket-pickup crash work is retained separately. The new human log identifies
bh_script_proximity_tick recycling the cached successor from generation1 to3
while leaving it active. Step/refresh now follow that active recycled pointer,
as original00477325/0047734c do; inactive-successor rejection remains an explicit
source boundary. Reuse/order regressions fail before and pass after the change.
Focused source checks passed; actual training completion and next-mission
coverage have NOT been established. Add connected scenarios to the box next.

### Previous checkpoint

Rocket-launcher training exit investigation, 2026-10-27: the new human run
`frame-times-13ab1c86-26520.csv` exits with frame-service ENTITY(4), sound=0,
player/query/weapon=0. This is not attributed to the earlier sound failure.
Preserved log, CSV and exact executable in `m4/playable/rocket-pickup-exit/`.
The source cached-successor lifetime guard is a candidate: original step/refresh
cache +e0 then continue without a generation gate. Fresh Ghidra instruction
records are checked against immutable EXE bytes. Exact failing owner remains
unknown; no traversal-policy or gameplay change is claimed.

Added source-only callback/current/successor/generation failure diagnostics,
and separate step/refresh errors before later passes overwrite pool.error.
Mutation and persistence controls pass in the focused non-renderer target;
299-TU game build and hardware-only static startup pass. Per-implementation
diagnostic evidence added to function-status.json without promoting historical
matching evidence. No gameplay automation or software-renderer execution.
Next observation: user retries the second training rocket-launcher pickup using
`play-hardware.cmd`; inspect the resulting Entity traversal failure line and
resolve its callback against the preserved diagnostic-game.map. This report
is NOT fixed yet; M4 remains incomplete.

### Previous checkpoint

All-weapon diagnostic and sound-failure continuation, 2026-10-27. Box automation
now runs2100 ticks: the existing combat/objective/pickup route, then ten staged
weapon pickups followed by real keyboard selection and primary/secondary fire.
Both final hardware-only runs pass with identical recorded traces: pickup and
firing masks1023; all seven projectile callback families observed(mask493).
Lead buster/stun ray check emission counters and ammo; static coating checks
ammo and the actual boost timer. No inventory/availability edits or direct fire
callback injection. The manual box has all ten pickups in the far room;
Ctrl/Space fire,1-5/6-0 select. Launch `build/box-test/Play box.cmd` or run
`python tools/test_box_level.py` after rebuilding/staging the box.

The expanded test reproduced a fatal frame-service error at tick1930, during
weapon8 effects. DirectSound rejected a negative frequency with80070057. The
original004791c4 calls sound and004791c9 overwrites EAX without checking it;
both0047c200 SetFrequency sites also continue/cache without HRESULT gates.
All retained original instruction bytes were checked against immutable EXE.
Source now retains native audio failures as diagnostics and continues frame
publication; malformed source topology/numerics remain errors. Frequency,
Doppler, cache order and RNG are unchanged; no frequency clamp was added.
Final box logs the same rejection(-127800) at tick1929 and completes.

A connected silent-PCM/native-DirectSound fixture verifies failure reporting,
unchanged rejected cache, actual producer packet/publication continuation,
and separate fatal malformed-topology control. Focused source checks,299-TU
game build, hardware static startup and evidence audit pass. A later nonfatal
draw diagnostic remains; no visual correctness claim. The original human log
only says frame-service failure, so the exact reported rocket-pickup exit is
not conclusively attributed. Preserved human log/build identity, failing box
trace, original contracts and final traces: `m4/playable/weapon-box/`.
No software-renderer tests or original gameplay ran; originals unchanged.
Per-implementation ledger refreshed, zero original owners added. M4 incomplete.

### Previous checkpoint

Ballistic launch numerical correction, 2026-10-27. Fresh hardware-only box runs
pass before and after this change: paired900-tick traces are identical within
each pair, and the recorded trace is unchanged across the fix. The box therefore
does not independently detect this deviation; no original scenario equivalence
is inferred. Actual kill/objective/drop collection, AI movement, contact and
inventory checks pass (51 shots,6 damage packets,5 contacts,final ammo399).

Original game-owned firing004305f0 retains muzzle centers across non-popping
FST instructions0043066c/0043068b until side offsets, and retains X velocity
through0043071b/00430725 across the first spread RNG call while Y/Z reload float
spills. Source rounded these values prematurely. Restored those orders in
bh_weapon_fire_ballistic without changing RNG count/cadence/recoil. An isolated
call-free original00430627..00430731 adapter with byte-verified constants and
exact axis positive controls captures128 finite cases underPC53. Connected
real firing/projectile checks previously differed in48 muzzle and30 velocity
cases; corrected source matches all six output words in all128 cases.

Focused real translation-unit checks pass with/without archive,299-TU game
build and hardware-only static startup pass. Refreshed box executable is at
`build/box-test/BHunter-box.exe`. Evidence and failed-before controls are in
`m4/playable/ballistic-launch/`; per-implementation records are refreshed in
`function-status.json`. Original EXE/BIN/MDF/MDS remain unchanged. The oracle
is test-only, calls no original subsystem, and is absent from game targets.
No software-renderer tests or original gameplay ran. PC24, full-owner arithmetic,
other firing families and broader fidelity remain unverified; M4 incomplete.

### Previous checkpoint

Original gameplay review checkpoint, 2026-10-27. The prior two source box runs
are determinism checks, not original comparison. Latest evidence is
`m4/playable/original-gameplay-review/validation.json`; the per-function scopes
and original instruction references are in `function-review.json` alongside it.
44 owners were reviewed across AI, collision, shooting, objectives and pickups.
This is a scoped static review, not a whole-function byte-match or full1:1 claim.

Three owners corrected: basic pickup00441b80 rejected types20/21 even though
original VM00443ff0 passes those types unchanged into the22-entry resource table;
traffic damage004357d0 and response00439910 rounded displacement/damping too soon.
Restored the original FST/FMUL spill order, retained sqrt/quadratic term and
unrounded damping, including the different final X versus Y/Z operands.
An isolated original00439a52..00439ba7 instruction adapter produced171 differences
in256 cases before correction and zero after, comparing all six output words
underPC53. The second owner's arithmetic instruction sequence is independently
verified identical, excluding interleaved health/tick instructions. Both real
source callbacks check those original-captured words. No gameplay subsystem
loader was created; the existing math mapper is untouched; the adapter is not
linked into shipping targets. No recovered-owner count increase.

Focused source checks,299-TU game build, hardware-only static startup and retained
evidence audit pass. `build/box-test/BHunter-box.exe` refreshed to the new game;
no new box playthrough, software-renderer tests or reference gameplay ran.
The strict pickup-radius rounding case already matches the original on the pinned
compiler, so no speculative arithmetic rewrite was made there.

Remaining fidelity boundaries are explicit in each review: PC24 and whole-owner
numerical equivalence, original unwritten wreck orientation and degenerate
ballistic normalization output, source validation/depth/capacity guards, other
projectile families, CD audio and peer gameplay. These are not marked equivalent.

### Previous checkpoint

Pickup packet correction and direct-entry box diagnostic completed, 2026-10-27.
Launch `build/box-test/Play box.cmd` (W/S thrust/brake, arrows steer, Ctrl fires,
Esc exits); `Run checks.cmd` runs two bounded hardware-only playthroughs.
The user's latest explicit request supersedes the earlier automation ban only
for this isolated diagnostic. No tutorial/mission or software-renderer tests ran.

The pickup producer had an extra reserved word: original event13 accepted/consumed
fields are at +0x14/+0x18, while the source read +0x18/+0x1c. Inventory could
increase without removing the item. Immutable original instructions and player/
actor reply owners establish the ABI. Removing that word fixes connected real
pickup/player/inventory tests; the old layout fails four retained assertions.
Checks cover strict six-unit radius, partial capacity, full-ammo rejection and
final depletion/unlink. No numerical fallback or RNG change was added.

The separate loose-asset box has two connected rooms, a solid post, ammo/weapon
pickups, route AI, real projectile/damage/wreck callbacks and a waitfor/VM objective.
Two 900-tick runs passed and produced identical CSV traces: objective at tick136,
wreck pickup at170, collection at205, ammo299->399. Assertions also cover finite
recorded state, room bounds, solid-post exclusion, contacts and AI movement.
Final focused non-renderer checks (319 TUs), game link (299 TUs), normal hardware
static startup and read-only evidence audit passed. Original assets unchanged.

Evidence: `m4/playable/pickup-box/validation.json`, `original-contract.json`,
`playthrough.json`, paired traces and generated assets under `box/`. The diagnostic
adapter adds zero original owners; 679 owners remain. Fresh original comparison
establishes the pickup packet contract; previous AI/collision/shooting/objective
contract evidence is indexed, not promoted into a new whole-body match claim.
Full original/source differential gameplay, PC24 equivalence and complete mechanic
coverage remain unverified. Windows screenshot capture timed out; no visual
verification claimed. M4 remains incomplete.

### Previous checkpoint

Live actor clock correction completed, 2026-10-27: traffic and scripted-vehicle
route/draw consumers now read the shared player simulation clock at each call,
instead of retaining the pre-step snapshot. Original global loads/writes were
verified against immutable EXE bytes. A stale-clock control reproduces NaN at a
route transition; live-clock inputs produce the finite pose without changing
banking arithmetic or RNG. Constructor/query/damage/pursuit and packet timestamp
consumers share the binding; packet overrides restore the low word on failure.
The previous session compiled and linked the real dependency closure (319 TUs),
passed focused asset-free/real-archive checks, rebuilt the 299-TU game and passed
hardware-only static startup. This session completed the interrupted evidence
recording and ledger refresh; no further game execution. Evidence:
`m4/playable/live-actor-clock/producer-audit.json` and `validation.json`.
The historical resource475 instance still lacks producer identity/route inputs;
its exact attribution and human gameplay confirmation remain unverified.
No clamp, replacement angle, software-renderer test or automated gameplay.
M4 remains incomplete; 679 original owners unchanged.


Reported source-session exit / NaN investigation, 2026-10-27: preserved the
human log, CSV and exact reported build under `m4/playable/nan-draw/`. Log shows
resource475, command196, finite position/yaw/pitch and indefinite roll; resource
argument rejection1 occurs before model/clip dispatch. The source then called
failure13 and performed cleanup. Earlier attribution to the player car/physics
was premature: a shared model handle is not an entity/producer identity.

Confirmed original-backed control-flow mismatch: byte-verified original queued
callback004775f0 discards draw return and loads next from command+50; frame
owner00479090 follows callback-returned pointers, then runs its owned00450930
tail. Source accumulated callback failures into frame failure and application
exit. Corrected that consumer policy for every callback kind. First failure,
resource/pose and errors stay diagnostic; invalid selector/topology/missing
bindings and native failures retain explicit source boundaries. Application
logs the first diagnostic once. No arithmetic, epsilon, neutral angle, RNG,
physics state or source model/texture lifetime changed.

Unchanged original traffic banking00434920 reproduces NaN for degenerate
horizontal tangents (stopped/repeated controls/vertical motion/endpoint cases).
The original contains that unguarded arithmetic too. This is a reproduced
producer class, NOT proven attribution of the historical command: that session
has no producer/route snapshot. Bounded hardware startup now feeds the recorded
pose and all seven payload fields with NaN/indefinite/+Inf/-Inf through the real
resource/frame owners, then consumes valid commands and completes frames.
No software-renderer or gameplay test occurred. Focused source checks preserve
regressions; their count is not evidence of general original fidelity. Root
report byte-verifies the control-flow argument independently of source tests.

Similar denominator review records camera7/9 normalized offsets, coincident
blast/body forces, orbit identity/proximity interactions and actor basis group
invariants as outstanding original-contract risks. Existing flyby zero-tangent,
vector normalization and coincident-light guards were identified separately.
No blanket safety or 1:1 pixel/PC24/full-port claim is made. Original degenerate
transform pixel/device effects remain unverified. Evidence and limits:
`m4/playable/nan-draw/root-cause.json`, `validation.json`; audit
`python tools/check_playable.py`.679 original owners unchanged;M4 incomplete.


Collision/bounce comparison and correction, 2026-10-27: byte-verified retained
original whole bodies004124d0,0041d7c0,0040b410 and0044da20 against the immutable
EXE. Two confirmed discrepancies fixed. Car event2 wrote bounce acceleration,
torque and spin damping into the entity while physics subsequently republished
a separate component over those writes. The real player constructor now binds
query.contact_physics; event2 mirrors its common-field changes before physics
finish. Hardware startup verifies the identity. Original00412788/0041278d skips
last-clear overlap recovery for kind0 physical actors, but source applied it to
actors; recovery now applies to world/prop geometry (including tunnel meshes).
World penetration and actor overlap have separate real-geometry controls.

Final focused checks:1996 without assets and2001 with real archive, zero
failures;306 real translation units, including response, dispatcher, sweep,
broad phase, actual player query, physics, target lifetime and input fixtures.
The omitted-state-bridge negative control demonstrates the original source bug
without running original/game code. Retained initial failed log has two valid
force/spin failures plus a torque expectation corrected from a centered contact
to an explicit off-center packet. All final cases pass.299-TU game rebuild and
hardware fullscreen bounded startup pass, zero collected input/entity dispatch/
application frames. No software-renderer tests or gameplay execution.

Four original variants and four support variants refreshed, zero owners added;
679 owners remain. Original arithmetic for sliding, mass splitting, moving-prop
translation/1.5 contact-motion scale and damping is retained; PC24/extreme-speed
and human tunnel/prop fidelity remain unvalidated. Evidence:
`m4/playable/collision-bounce/original-contracts.json` and `validation.json`;
audit `python tools/check_playable.py`. M4 incomplete; SDL3/SDL3 GPU direction
and removal of the live software path remain the next platform phase.


Runtime fixes and user platform direction, 2026-10-27: physics roster changes
now transactionally transfer unchanged real sweep workspaces and the validated
static scene buffers. Removed geometry borrows end before collision-slot reuse;
stale/duplicate generations, busy state and eight allocator failure points are
checked. Physics lifetime counters survive rebind. The application calculates
contact deltas independently and uses growing4096-sample blocks; hardware static
startup verifies40001 ordered samples and independent reset accounting. Mouse
motion is disabled before binding evaluation, including retained analog/raw/
accumulated channels; keyboard/joystick and mouse buttons remain enabled, with
no polling/recentering of the mouse in this mode.

Final validation: a fresh301-TU focused executable passes1534 asset-free and1539
real-archive checks, executing only physics, target construction/lifetime and
input fixtures. The299-TU game builds and hardware fullscreen bounded startup
passes with zero input collection/entity dispatch/application frames. Synthetic
5815-entry scene benchmark (three iterations) measures16.23ms full-init versus
0.081ms rebind asset-free and16.43ms versus0.089ms with archive. These are source
fixture timings, not gameplay FPS. Earlier broad attempts failed a new fixture's
packed-float versus integer expectation; the corrected fixture passes focused
validation. No fresh broad-suite or independent clean-checkout pass is claimed.
Evidence: `m4/playable/runtime-fixes/validation.json`; audit:
`python tools/check_playable.py`. Five original source variants and four support
variants refreshed with old snapshots retained, zero owners added;679 remain.
Manual hitch improvement and mouse-control confirmation are pending; M4 remains
incomplete.

The user prohibits any further software-renderer tests. Do not run the broad
m0/clean-checkout runners while they execute those scenarios. Use
`python tools/check_runtime_fixes.py` for these non-renderer checks and hardware
startup only. Next phase: migrate to SDL3 and SDL3 GPU and completely disconnect/
remove the live software path, retaining historical artifacts as evidence only.
This supersedes the earlier software-preview/backend-deferral direction and is
also recorded in AGENTS.md and RESTORATION_PLAN.md. No SDL migration is claimed
in this block. No gameplay automation or original-code execution occurred.


Manual performance analysis, 2026-10-27: read the user's two hardware CSV
captures (6,697 and 36,000 presentation samples), with no game execution or
source changes. The long capture ends at168.923s; its exit log records3,088
dropped samples. Fourteen producer samples change the physics roster rebuild
count, thirteen exceeding33.333ms; the worst simulation cost is160.268ms while
rendering takes1.5461ms. Whole-context disposal/reinitialization in
`bh_target_construct_sync` is the next source-backed performance suspect;
`bh_physics_step_init` repeats world uniqueness scans and eagerly allocates
all geometry-pair sweep workspaces. Internal cost attribution remains unmeasured.
All ten apparent contact spikes above10 coincide with rebuilds: the profiler
differences a sum of resettable physics contacts and cumulative projectile
counters, so these values cannot establish a collision storm. Stable-roster
mission simulation median6.946ms, p9910.511ms; ten other stable-roster samples
still exceed33.333ms. Immutable copies, source hashes and detailed findings:
`m4/playable/manual-performance-20261027/analysis.json`. No implementation
admitted or ledger status advanced; human CSV evidence does not pass M4.


Menu frame lighting follow-up, 2026-10-27: corrected the directional dot sign
in `bh_model_draw_lit`. Original `0045a403` branches on negative camera side to
the path with FCHS at `0045a6b6`; the nonnegative path has no negation. Source
had reversed those cases, leaving visible menu faces with purple ambient light.
Literal controls now verify the original menu colors on negative, positive and
zero side. Native fixture lighting direction was corrected for its negative-side
face, preserving actual red/green pixel expectations. Real-asset integration
passes 5,530,026 checks; independent clean integration passes 5,024,071, zero
failures. The 299-TU game build and bounded static startup pass. One functional
owner variant is refreshed with historical source retained, adding zero owners;
no byte match is claimed. Human menu appearance confirmation is pending.
Evidence: `m4/playable/light-sign/validation.json`; `python tools/check_playable.py`.
The user thinks splash skipping now works. The brief wireframe ship is present
in the original `textures/bad.pcx` loading image, not a transient ship renderer.
Projectile visuals and collision slowdown remain unresolved; M4 incomplete.

Human preview follow-up, 2026-10-27: the user confirms visible company splashes
and chat panel, no automatic steering and good baseline FPS after the splash/perf
changes. Many bullets/collisions still cause slowdown; projectile size/overlap
and dark purple menu frames remain unresolved. The original screenshot shows
teal/green frames. The user also confirms the original first splash skips on a
mouse click. The WIP application window adapter now consumes left-button-down
during company stages and expires the existing display timer with redraw1;
the existing display owner performs the transition. This is grounded in human
observation; the original skip input-hook location is not yet established.
The 299-TU game rebuild and bounded static startup pass, with zero input
collection, entity dispatch or application frames. Human click verification is
pending. Harness sources are unchanged, so prior real/clean integration results
are retained explicitly. Current evidence: `m4/playable/splash-click/validation.json`;
`python tools/check_playable.py` passes. No gameplay automation or original code
execution. M4 remains incomplete.

Mission effect binding follow-up: the user reported a Space-triggered exit in a
mission and invisible bullets despite visible hit decals. The log identifies an
oriented draw with image handle zero. The WIP application adapter now binds the
player boost particle image to original `004f54d8`. It also supplies the real
camera focal scale to ribbon widths and dispatches ribbon vertices through the
scalar-shaded primitive owner `0046e430`; the former scale of1 and plain-color
callback made trails too narrow and black. The rebuilt game passes bounded startup
with two static source-only render fixtures: beam428 and boost1230 nonblack pixels,
followed by the initial static scene. No input collection, entity dispatch or
application frames occur. One WIP adapter variant is retained, adding zero original
owners. Harness source/tests are unchanged from the menu-UV checkpoint; that
checkpoint's integration results are retained separately from the fresh game test.
Human mission verification is pending. Evidence:
`m4/playable/effect-bindings/validation.json`; `python tools/check_playable.py`.

Menu text UV follow-up, 2026-10-26: the user confirmed that the menu is now visible
and its model frames render correctly, but the interior text sprites are distorted.
The quad owner had exchanged the UV pairs on its second and fourth corners, making
U vary along the vertical edge. Retained original stack slots confirm the corrected
pairing. A source-only check now sends the original 1-by-0.4 menu text quad through
the real producer, consumer and clipper; it verifies full-width U and half-page V.
Focused sprite tests pass 660 checks; full real-asset integration passes 5,529,970
and independent clean integration passes 5,024,022, zero failures. One original
owner variant was refreshed with its historical source retained; no owner was added.
Bounded static startup passes. The preview continues to default to software
rendering; hardware rendering is unvalidated. Current follow-up evidence is under
`m4/playable/menu-uv/`. Human appearance confirmation is pending.

**Source preview integration, 2026-10-26:** `play-source.cmd` launches a standalone
299-production-TU source target. Real-data source integration passes 5,529,958 checks;
the independent clean checkout passes 5,024,010, zero failures. Production contexts
agree; one test fixture has explicitly retained Git-normalized line endings.
Bounded initialization and initial static rendering pass with no input collection,
entity dispatch or application frames. No original code runs. The user observed a
moving flythrough, invisible menu and persistent selection audio. The new build
publishes final camera outputs consistently with menu placement and binds looping
audio only to the owned slide voice; human confirmation is pending. Earlier raster
conversion and horizon-table corrections are covered by the current tests.
38 source variants were refreshed with historical bytes retained, adding zero owners;
679 distinct harness owners remain. Preview composition adapters stay WIP and no
shipping gameplay admission is claimed. M4 remains incomplete; PC24, CD audio,
DirectPlay peers and native hardware retain their limits. Evidence:
`m4/playable/validation.json`; audit: `python tools/check_playable.py`.
The finish checkpoint below is historical for the subsequently changed source files.

**Inventoried function recovery complete:679 distinct admitted whole source owners**, with no remaining inventoried owner. This block adds23 to656 and reconciles the obsolete729label. Real-data integration passes **5,529,939checks**; independent asset-free checkout passes **5,023,991**, zero failures, identical compiled source/header bytes,601translation units and295missing-module controls. All679active source hashes and23new whole boundaries verify. The recovered activation VM is now in the891-record catalogue; the0042f350config-save forwarding alias counts zero. Earlier library/helper exclusions remain separate. Final player review corrected an overlapping context pointer with an explicit checked registry and verified consecutive installed callbacks; historical variants are retained. Evidence/audit: `m4/finish/README.md`, `final-audit.json`, `validation.json`; `python tools/check_finish.py`.

The user's remaining-function request is complete for the current inventory. **M4 and a playable shipping game remain incomplete**: full positive asset/native/application bootstrap and gameplay are unexecuted; actual source resource/scene/player/physics composition and later fidelity still require validation. PC24, CD audio, native hardware and actual peer gameplay remain open. M3 stays skipped. No game/original code, game execution, automated gameplay, physical input, screenshots or agents in this block. Next observable milestone: `m4/finish/NEXT.md`.

### Historical656-function checkpoint

Requested another50-function block,2026-10-26: **656 distinct admitted linked original source owners** (+50 from606), **568 translation units**, **278 missing-module controls**. Real-asset build passes **5,527,805 checks**; independent asset-free clean build passes **5,021,857**, with identical compiled source/header bytes and all656 active source hashes verified. Restored28 software raster callbacks,4 complete primitive dispatch owners,3 world-edit geometry/movement owners,4 special actor response/constructor owners,2 launch/receive owners,2 whole VM/factory owners,4 complete parser/builders and3 block loader/editor preview owners. All50 whole boundaries and77 typed constants verify;28 historical raster variants remain preserved. The requested block is complete and stops at656. Original references remain unchanged. No original/game code, game execution, gameplay automation, physical input, peer connection or audio playback. Static source TEST.LEV screenshot: `m4/target656/capture/source-software.png`; user plans SDL3 replacement, with isolated renderer retained as fidelity reference. M3 skipped; M4/playable source game, PC24, CD audio and actual peer gameplay remain incomplete. Evidence/audit: `m4/target656/README.md`, `validation.json`, `final-audit.json`; `python tools/check_target656.py`. Prior reviewed queue:24 unadmitted candidates; exclusions/aliases are not source completions. Next source contracts: `m4/target656/NEXT.md`.

### Historical606-function checkpoint

Remaining129 request resumed2026-10-26: **606 admitted linked original source owners** (+6 from600), **550 translation units**, **269 missing-module controls**. Real-asset build passes **5,318,968 checks**; independent asset-free clean build passes **4,813,020**, with identical compiled source/header bytes and all606 owner hashes verified. Recovered whole impact-ring draw, force-feedback update, paired remote pose prediction, remote rendering, startup SourceDir/checksum gate and remote query/damage-packet/list retirement. Fifty former unknown entries are verified VC5 library/internal-library labels or an import thunk; they add zero source functions. **73 catalogue entries remain and the user request is not complete.** Provenance exclusions and source admission are reported separately. Source-only fixtures; no original/game code, game launch, automated gameplay, device effect update or actual peer Send. M3 skipped and M4/playable game incomplete. Evidence: `m4/remaining129/README.md`, `validation.json`, `final-audit.json`, `remaining.json`; audit `python tools/check_remaining129.py`. Current next-source contracts: `m4/remaining129/NEXT.md`.

### Historical600-function checkpoint

Requested 600-function checkpoint, 2026-10-26: **600 distinct admitted linked original source owners** (+50 from 550), **540 translation units** and **264 missing-module link controls**. Real-asset build: **5,317,919 checks**; independent asset-free clean build: **4,811,971 checks**; zero failures, identical source/header bytes. All 600 active source hashes, 50 new whole-owner ranges and 201 constant annotations verify. New source closes projectile/weapon/effect dependencies, actor attack/update/damage/routing, basis/force controls, player aim, network scores/debug/effect receive and flyby construction. Adapters, switches and owned shared tails count zero. Final evidence: `m4/target600/README.md`, `validation.json`, `final-audit.json`, `session.json`; audit: `python tools/check_target600.py`. Original references unchanged; no original/game code, game launch, automated gameplay, physical input or desktop capture. M3 remains skipped; M4/playable source game, PC24, physical media and actual peer gameplay remain incomplete. **Work stops at exactly 600.**

Previous checkpoint (historical):

Requested550-function checkpoint,2026-10-26: **550distinct admitted linked original source owners** (+50from500), **485translation units** and **236missing-module link controls**. Final real-data build passes **4,913,560checks**; independent asset-free clean build passes **4,407,612checks**, with identical source/header bytes and all550active source hashes verified. Restored whole player query(1), projectile construction/draw(5), renderer startup(4), remote prediction/slot selection(2), segment wrapper(1), trails(5), complete script actor dependency/constructor closure(8), mission progression(1), menu actions(12), full menu step/return(2), music/quit/fades(6), native host/join/receive(3). Duplicate36670was rejected by admission gate, replaced with reuse of the existing implementation and genuine422570recovery; count-zero adapters/data/tails add no counts. Review fixed borrowed depth-attachment ownership and clarified simulation-tick packet binding;10existing source originals receive fresh context variants with old snapshots retained. Final evidence: `m4/target550/README.md`, `validation.json`, `final-audit.json`, `session.json`; audit: `python tools/check_target550.py`. Original references unchanged; no original/game code, automated gameplay, physical input or desktop capture. Positive network/native hardware and physical media limits remain explicit. A discarded valid-display-mode assumption is documented without claiming UI state preservation; final tests use impossible mode values and owned offscreen surfaces. M3skipped; M4/playable source game, PC24, physical CD, hardware-positive feedback and actual peer gameplay remain incomplete. **Work stops at exactly550.** Next contracts: `m4/target550/NEXT.md`.

### Previous500-function checkpoint (historical)

Requested500-function checkpoint,2026-10-26: **500 admitted linked original source owners** (+47 from453), **451 translation units**, **219 missing-module controls**. Final real-data integration passes **4,901,811 checks**; independent asset-free clean checkout passes **4,395,863 checks**, with identical compiled source/header bytes and all500active owner hashes verified. Restored marker/decal ownership(2), homing/ballistic steps and pulse firing(3), target constructors(3), VM-installed damage/radar queries(3), effect cleanup/expiry(2), menu presentation/light(3), menu edits/actions(17), connection/session-list ownership(6), media controls(4), and GDI/editor/queue-next behavior(4). Source adapters, tables and refreshed renderer-dispatch variant add zero original functions. Review corrected native NULL-provider error sequencing, retained target room hints and integer/float inertia ordering. Original references are unchanged; no game/original code, automated gameplay, physical input or desktop capture ran. Evidence: `m4/target500/README.md`, `validation.json`, `final-audit.json`, `session.json`; audit: `python tools/check_target500.py`. Historical candidate variants and line-ending normalization evidence are separate; no byte-match transfer. The old727catalogue denominator remains provisional. M3 remains skipped; M4/playable source game, PC24, physical CD, hardware-positive force feedback and actual peer gameplay remain incomplete. **Work stops at exactly500.** Next source contracts: `m4/target500/NEXT.md`.

### Previous453-function checkpoint (historical)

One-hour source restoration,2026-10-26: **453/727 admitted linked original functions** (+10 from443), **425 translation units** and **206 missing-module controls**. Final real-data integration passes **4,900,025 checks**; the independent asset-free checkout passes **4,394,082 checks**. Compiled inputs agree and all active owner source hashes verify. Restored menu object/input construction(1), root/secondary explosion construction/tick/refresh(4), key capture(1), whole actor/world physics(1), profile select/owning clone(1), text capture(1), and whole target lifetime/retirement/physics step(1). Typed registries, source bindings and owned switch data add no count. Three config source originals and four corrected explosion variants receive fresh current linked evidence; old sources remain frozen and historical. A PE literal audit caught explosion debris-count scale-.5 instead of-1/32768, with mirrored expectations; corrected297-check explosion and179-check target fixtures now execute actual magnitudes12/15 and exact RNG populations in both full builds. Evidence: `m4/hour443/README.md`, `validation.json`, `review.json`, `session.json`; audit: `python tools/check_hour443_final.py`. No game/original code, automated gameplay, physical input or UI capture ran; new fixtures add no native playback or Send. Event12 receiver gameplay semantics and physics roster rebuilding after target retirement remain external. Source froze11:04UTC and is stopped at the requested hour closeout. Next complete constructor candidates0043d2c0/0043d7c0/0043ddc0 are raw-verified in `m4/hour443/NEXT.md` and `frontier.json`. M3 is skipped; M4/playable source game, PC24, physical CD audio and actual DirectPlay gameplay remain incomplete.

### Previous443-function checkpoint (historical)

One-hour source restoration, 2026-10-26: **443/727 admitted linked original functions** (+19 from424), **409 translation units** and **198 missing-module controls**. Final real-data integration passes **4,898,629 checks**; the independent asset-free checkout passes **4,392,781 checks**. Compiled inputs agree and every active ledger source hash verifies. Restored complete flash/flare refresh, producer and consumer (3), target drawing (1), UTF16 atlas/queued text (4), force-feedback initialization (1), effect drawing (3), script debris (3), projectile cleanup/query (2), and menu row construction/refresh (2). Nine existing source owners received fresh dispatch-context variants without increasing the function count; historical snapshots remain separate. Evidence: `m4/hour424/README.md`, `validation.json`, `session.json`; audit: `python tools/check_hour424_final.py`. Native force-feedback finds zero devices: zero physical Acquire/effect Start; positive hardware paths remain unexecuted. No game/original code, gameplay automation, physical input or UI capture occurred. M3 remains skipped; M4 and a playable source game remain incomplete. Implementation is frozen at the requested hour closeout. Next source contracts: `m4/hour424/NEXT.md`; full projectile physics requires the real typed candidate/geometry/event bridge, and explosion callback dependencies remain unadmitted. PC24, CD audio and actual DirectPlay gameplay remain open.

### Previous424-function checkpoint (historical)

One-hour source restoration, 2026-10-26: **424/727 admitted linked original functions** (+17 from 407), **392 translation units** and **189 missing-module controls**. Final real-data integration passes **4,895,099 checks**; the independent asset-free checkout passes **4,389,271 checks**. Compiled inputs agree and all active ledger source hashes verify. Restored particle/emitter behavior (6), procedural collision-box construction (1), native DirectSound startup (1), script sensor steps (2), flash lifetime (1), projection (1), CD/audio startup and volume queries (3), provider enumeration (1), and complete provider/modem initialization (1). Context/guard helpers add no count. Four stage admissions and the aggregate audit are retained under `m4/hour407/`; audit: `python tools/check_hour407_final.py`. The native modem initialization/address queries fail with actual recorded HRESULTs, with zero successful connections/sessions; positive CD paths remain unforced on this host. No game/original code, gameplay automation, physical input or UI capture occurred. The source hour is closed with no further implementation assigned. M3 remains skipped; M4 and a playable source game remain incomplete. Next review flash refresh/queue closure or whole target drawing now that its projection prerequisite exists; full VM/parser/world construction, PC24, CD audio and actual DirectPlay gameplay remain open.

### Previous407-function checkpoint (historical)

Resumed source restoration2026-10-26: **407/727 admitted linked original functions** (+7 from400),374 translation units and180 missing-module controls. Full real-data integration passes **4,889,420 checks**; the independent asset-free checkout passes **4,383,592 checks**, with identical compiled input hashes and every active owner source hash verified. Restored attached script sound(1), script target translation/rotation(2), native DirectDraw startup inventory/selection callbacks(2), and editor input setup/live packet dispatch(2). Context/guard adapters add no count. Evidence: `m4/resume400/README.md`, `validation.json`, `session.json`; audit: `python tools/check_resume400.py`. Full VM/parser/block construction remains genuinely blocked by missing whole owners; no callback stubs were added. No game/original code, gameplay automation, physical input or UI capture occurred. M3 remains skipped; M4 and a playable source game remain incomplete. Source restoration is resumed; PC24, CD audio and DirectPlay gameplay requirements remain open.

### Previous400-function checkpoint (historical)

Requested400-function checkpoint reached2026-10-25: **400/727 admitted linked original functions** (+35 from365),366 translation units and176 missing-module controls. Full real-data integration passes **4,888,726 checks**; the independent asset-free clean checkout passes **4,382,898 checks**. Compiled input hashes agree and every active ledger source hash is verified. Added menu settings/input-profile assignment(10), script/entity cleanup and delayed mission sound(9), serial/modem preferences/address callback(10), and closed entity/numeric/packet helpers(6). Source context adapters and guards add no count. Current evidence: `m4/target400/README.md`, `validation.json`, `session.json`; read-only audit: `python tools/check_target400.py`. Work stops at exactly400. No game/original code, gameplay automation, physical input or UI capture occurred. M3 remains skipped; M4 and a playable source game remain incomplete. PC24, CD audio and DirectPlay gameplay remain open.

### Previous365-function checkpoint (historical)

Requested365-function checkpoint reached2026-10-25: **365/727 admitted linked original functions** (+17 from348),358 translation units and172 missing-module controls. Full real-data integration passes **3,871,035 checks**; the independent clean checkout passes **3,365,207 checks**, with identical compiled inputs and every active ledger source hash verified. Added script numeric readers(2), rail/path owners(4), model scale/winding(2), native device enumeration(1), wagon behavior(5), wait-for queries(2) and basic pickup construction(1). Six timer source variants were freshly validated after count-zero callback exposure; they add no function count. Source work stopped at the requested365 cap. Session: `m4/target365/session.json`; final audit: `m4/target365/final-audit.json`; latest family audit: `python tools/check_pickup_basic.py`. M3 remains skipped; M4 and a playable source game remain incomplete.

Timed loading-image restoration,2026-10-25: **9 complete owners added:348/727 linked original functions in344 translation units**, advancing the339-function baseline. Final real-asset integration passes **3,864,196 checks**; the asset-free clean checkout passes **3,364,342 checks**, with **165 missing-module link controls** and identical compiled source hashes. Complete PCX-to-loading-arena conversion and three-image initialization now feed all five whole radial progress wedge owners, the complete timed native loading display, and the actual archive first-access float callback.

All three real PCXs are checked at full/half size with indexed/native and quarter-bright tables, retained palettes, allocation/stream failures and real owning close. Geometry checks cover100 independent small masks plus40 full320x240/640x480 footprints, signed/odd pitch and protected spans. Actual offscreen DirectDraw checks retain1/2/3/4-byte full-copy behavior, the3-byte wedge no-store path, timing/history/NaN behavior, BltFast and the real offscreen Flip error. The real VFILE callback paints verified pixels through the complete source display owner. The existing PCX-close mapping is an explicit **count-zero lifecycle adapter**, not a tenth recovered function.

No game, original code, gameplay automation, physical input or UI capture occurred. New loading fixtures perform no audio operations; the existing integration sound-tick fixture retains twelve native Play calls on silent PCM per full build. Fullscreen Flip success, actual negative-pitch native Lock, lost-surface Restore and PC24 behavior remain unvalidated; signed pitch remains implemented and memory-tested. M3 is skipped, M4 is incomplete and no playable source game exists. CD audio and DirectPlay requirements remain open.

Exact current evidence/audit: `m4/loading-display/README.md`, `m4/loading-display/validation.json`, `python tools/check_loading_display.py`. The intermediate two-owner image checkpoint is preserved separately in `m4/loading-image/` at341 functions; do not rerun its current-tree checker after later integration changes or transfer byte-match evidence. The hour's timing is retained in `m4/loading-image/session.json`. Next source boundary/limits: `m4/loading-display/NEXT.md`.

The requested one-hour block is closed. Implementation is frozen and stopped pending user resumption; the next bounded source assignment is retained in the linked frontier.

### Previous frame/lifecycle/sound checkpoint (historical)


Timed source restoration,2026-10-25: **17 complete owners added:339/727 linked original functions in336 translation units**, advancing the322-function baseline. Final real-asset integration passes **3,846,313 checks**; the asset-free clean checkout passes **3,352,711 checks**, with **161 missing-module link controls** and identical compiled source hashes. The whole frame producer/acknowledged shutdown and registered renderer worker now link real begin-frame/BMP/resource teardown owners. Complete window dispatch/default callback, force-feedback activation and CD notification retain their native lifetime and message contracts. The whole five-argument sound resource loader replaces the historical PCM subset and uploads all60 archived WAVs into real native buffers with sample CRC/frequency/nonplaying checks.

The combined source fixture publishes a frame through the installed renderer and real worker. Private native-window callbacks validate nested CLOSE/DESTROY without double Release or Terminate. No game, original code, gameplay automation, physical input capture/poll/recenter or audible playback occurs. Optional BMP checks read owned synthetic/offscreen data, emit no UI screenshot and remove their files. Fullscreen Flip, actual lost-surface Restore, hardware-positive force feedback, physical CD looping, DirectPlay sessions and PC24 numerical behavior remain unvalidated. M3 is skipped, M4 is incomplete and no playable source game exists.

Implementation is frozen at the requested one-hour closeout. Next bounded source family is complete loading-image owner00449e50 plus three-image initializer0044a160, with the existing owning PCX-close behavior reconciled separately. Full WinMain/00479200 startup and world/mission/resource construction remain open; do not restart reference gameplay capture or count already admitted owners again. Current evidence/audit: `m4/sound-loader/README.md`, `m4/sound-loader/NEXT.md`, `python tools/check_sound_loader.py`. Earlier exact checkpoints: `m4/frame-pipeline/README.md` (12 owners,334 functions) and `m4/window-dispatch/README.md` (4 owners,338 functions). The hour's timing and execution limits are retained in `m4/frame-pipeline/session.json`.

### Previous HUD continuation checkpoint (historical)


HUD/attachment continuation,2026-10-25: **22 complete owners added:322/727 linked original functions in310 translation units**. The final real integration passes **3,845,398 checks**; the asset-free clean checkout passes **3,352,159 checks**, with **148 missing-module link controls** and identical source hashes. Complete player/flyby scenes and rearm, player reset, both display clears, whole renderer callback (including its owned tail), message/panel/plain-text family, two weapon timers and four empty-ammo/action methods now compile in their real units, link real definitions and execute affected checks. The640x480 source player scene now drains through the complete renderer; native panel checks distinguish plain text cursor continuity from positioned text.

The factory/weapon identity registry and panel allocation remain explicit adapters; unrecovered firing/mission/player/startup lifetimes are not counted as complete. Full render producer/worker, software raster dispatch, PC24 behavior, CD audio and DirectPlay remain open. No original code, game execution, automated gameplay or native input occurred. M3 remains skipped; M4 remains incomplete. The user requested early closeout and **work is stopped**. Evidence: `m4/hud-continuation/README.md`, `validation.json`, `owners.json`; audit `python tools/check_hud_continuation.py`.

The requested textured Blender export was delivered separately and confirmed good by the user:454 BSP models,5815 placements,10367 faces and351 packed textures, with saved-file transform/UV verification. This is an asset export, not additional source completion. No further screenshots unless explicitly requested.

### Previous HUD checkpoint (historical)

Timed HUD/attachment restoration, 2026-10-25: **33 complete owners added: 300/727 admitted original functions in 288 translation units**. The real integration passes **3,844,710 checks**, the asset-free clean build **3,351,626 checks**, with **138 missing-module link controls** and identical source hashes. Complete room/portal radar graph, HUD setup/markers/world/flat status, scoreboard, weapon arc/target overlays, full-image plane quads, adaptive paths, two-point segments, fade, end-frame presentation, shared weapon reset/delay, both selected weapons' lifecycle, target removal and float timers now link with real source definitions. Two corrected ribbon variants preserve duplicate-point/unordered x87 behavior with historical evidence kept separately. Real HUD/font/weapon assets render through actual offscreen DirectDraw/Direct3D2 owners. No original code, game execution, native capture or cursor recenter occurred.

Next source boundary is complete player scene callback `0041c860`; its direct rendering dependencies are now available, while player/camera/weapon construction and live resource/state binding remain explicit work. Flyby callback `00428f50`, render producer/worker `00479140`/`00471e00`, software raster dispatch, mission lifetimes and startup resource/script binding remain open. Fractional native fade has a patterned host readback; lost-surface Restore, PC24 numerical behavior, CD audio and DirectPlay retain stated validation limits. M3 is skipped; M4 is incomplete and no playable source game exists. User directed stopping at exactly 300 admitted functions; this checkpoint is stopped pending resumption. Evidence: `m4/hud-attachment/README.md`, `m4/hud-attachment/NEXT.md`; audit `python tools/check_hud_attachment.py`.

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

Previous input/render/actor-route checkpoint, preserved as history:

Timed source restoration, 2026-10-25: The 90-minute input/render/actor-route block restores **33 complete owners: 267/727 admitted original functions in 249 translation units**. The real integration passes **3,839,979 checks**, the asset-free clean build **3,347,025 checks**, with **120 missing-module link controls** and identical source hashes. New owners cover horizon and quad queues/geometry, radar, complete input message/collection/device owners, target/route updates, three-buffer handoff, generic queue reservation and queued render-mode execution. Rectangle UV and live near-plane fixes retain separate historical variants. No original code or game execution, native capture or cursor recenter occurred.

Both full scene callbacks `0041c860`/`00428f50`, render producer/worker `00479140`/`00471e00`, complete HUD/attachment setup, player/flyby construction, weapon/mission lifetimes and startup resource/script bindings remain open. Horizon geometry is restored; software dispatch/raster kernels remain unfinished. PC24, CD audio and DirectPlay requirements remain. M3 is skipped; M4 is incomplete and no playable source game exists. This timed block stops pending user resumption. Evidence: `m4/frontier-90min/README.md`; audit `python tools/check_frontier_90min.py`.

Previous checkpoint, preserved as history:

Timed source restoration, 2026-10-25: **The timed scene-caller dependency block restores 52 complete owners: 234/727 admitted original functions in 203 translation units; 3,536,976 archive checks, 3,044,022 clean checks and 99 missing-module link controls. Every admitted variant compiles in its real translation unit, links against real definitions in build/m0/bhunter-integration.exe, and passes affected source-built checks. Real and clean source hashes agree. No original code or game execution occurred.**

Both scene callbacks `0041c860`/`00428f50` remain unimplemented. Full horizon/software rasterization, HUD/handoff, player/flyby constructors, physical input collection, weapon lifetimes and startup resource/script bindings remain open. Next recover a complete dependency family on that frontier; weighted target helper `00421610` is reviewed but unimplemented. PC24, CD audio and DirectPlay requirements remain. M3 is skipped; M4 is incomplete. This timed block is stopped pending user resumption. Evidence: `m4/scene-callers/README.md`, `m4/scene-callers/validation.json`; audit `python tools/check_scene_callers.py`.


M4 outer scene orchestration and world/resource bindings, 2026-10-24:
**182/727 original functions now have admitted source variants** in a
163-TU harness. Complete `00409130` restores the scene gate, portal projection,
cached room bounds and exact/margin/fallback lookup, frame increment, recursive
entity scheduling, and two queued full-target clip resets. Camera dimensions
use truncated integer halves and clip bounds while preserving the draw target
extent. The source binding adapter maps constructed world rings and containment
sentinels separately from matched portal neighbors and validates resource handles.
The scheduler now binds player overlay position separately from the render eye;
three updated translation-unit variants retain their older tested snapshots.
The build passes 3,399,450 real-data / 2,906,962 clean checks, including
185 focused real-data checks and 80 missing-module link controls. Native pixels
use real TEST.LEV placements 0/5797 and model 129 through source construction,
portal recursion and real resource consumers. Cached/reacquired images match,
a room change differs, and disabled world drawing produces a blank control.
No original code or game launch. See `m4/scene-owner/validation.json` and
`../tools/check_scene_owner.py`. The bindings are an adapter, not admission of
full world/resource/startup construction or block-script initialization. Next
frontier: scene callers `0041c860`/`00428f50`, player/camera construction and
actual resource metadata. Full gameplay, software rendering and x87 PC24 remain
unfinished.

M4 pickup construction and entity scheduling, 2026-10-24:
**181/727 original functions now have admitted source variants** in the
158-TU linked harness. Seventeen complete owners cover `00441a00`, pickup
lifetime/query/cleanup, default and numeric-label drawing, queue controls,
portal clipping and recursive `00409f40` scheduling. `00454fc0` remains a
partial native rectangle adapter; software rendering is unfinished. Source
bindings preserve the actual entity `+e8` callback, room/global lists,
post-callback traversal, linked inactive sentinel, frame suppression and
portal clip recursion. Raw EXE constants correct both random constructor
multipliers. Explicit unsigned multiplication preserves original 32-bit
wrapped label totals. The real-archive native fixture constructs and schedules
a pickup through actual source owners, renders model/glow/numeric rectangles,
and verifies a blank inactive-room control. No original code or game launch.
The corrected build passes 3,399,331 real-data and 2,906,922 clean checks,
including 287 focused real-data checks and 78 missing-module controls.
See `m4/pickup/validation.json`, `../tools/check_pickup.py` and the lifecycle,
draw and scheduler reviews. Next frontier: outer scene owner `00409130`,
actual world/resource bindings and actor construction. Source gameplay,
non-model registries, software rasterizers and x87 PC24 remain incomplete.

M4 queued consumers, resource lifetime and entity draw binding, 2026-10-24:
**164/727 original functions now have admitted source variants**. Complete
`004775f0`, `00477640`, `00478170` and `004413b0` recover both queued draw
callbacks, resource submission and a game entity draw method. A separate
source adapter owns loaded models/caches/instances/descriptors under stable
handles; it does not add an original-function count. The 149-TU integration
passes 3,399,044 real-data and 2,906,719 clean source checks, with 74 missing-
module link controls. All four original references remain unchanged.

The 157 focused real-data checks include exact command copies/bucket order,
post-draw next-link mutation, owner initialization/allocation/lifetime guards,
entity field/phase/angle/saturation behavior and real native output. A source
entity slot submits two owned PU_9A model draws plus its real flare image;
both callbacks execute through actual rendering owners/state/queues. Invalid
resource/image bindings produce a blank native control. No gameplay or game
execution occurred. Static level pixels remain identical with 1,798 polygons,
3,112 triangles and 146 textures through the new queued/owned source path.
Evidence: `m4/queued-resource/`; audit: `python tools/check_queued_resource.py`.

Public resource enum tags now have a distinct namespace. Earlier functional
source variants and their tests retain separate snapshots; the renamed variant
has current linked evidence. The entity method belongs to payload +e8, not
refresh +c8. Actual constructor 00441a00 and scene scheduling 00409f40 remain
unfinished. Next review remaining pickup draw/tick/query dependencies 004412f0,
004415a0 and 00441570; world-location owner 0040a8c0 is already recovered.
Non-model descriptor constructors, full startup registries, software rendering
and atlas glyph copy remain open. M4 has not passed.

M4 sprite dependencies and recursive resource draw, 2026-10-24:
**160/727 original functions now have admitted source variants**. Complete
`00455960`, `00455f70`, `004569b0` and `0045adf0` recover billboard geometry,
ribbon production, near-plane path clipping and four-tag resource recursion.
The 142-TU integration passes 3,398,887 real-data checks and 2,906,653 clean
source checks with 71 missing-module link controls. The 169 focused checks
include actual native queues and offscreen Direct3D2 pixels for billboards,
per-vertex ribbon modulation and an empty LOD negative control. All references
remain unchanged; no original code or game execution occurred.

The static real-level renderer now calls the recursive resource owner before
the model-instance owner; its 1,798 polygons / 3,112 triangles / 146 textures and
bitmap are unchanged. Disassembly review preserves LOD retained-double near
comparison versus stored-float far/selection, unknown-tag no-op, path append
links, ninth billboard scalar and ribbon miter precision. Evidence and source
hashes: `m4/sprite-world/`; audit: `python tools/check_sprite_world.py`.

The hardware branch adapter for `0046e430` is explicitly partial/WIP, rejects
software use and is excluded from the four-function admission. Resource recursion
is not entity traversal: shell selection, placement/descriptor/cache binding and
level entity installation remain unfinished. Next review queue consumer
`004775f0`, resource constructors/lifetimes and entity bindings. Software
rasterizers and atlas glyph copy remain open; M4 is not complete.

M4 model-instance draw owner, 2026-10-24: **156/727 original functions now
have admitted source variants**. Complete `0045aa60` (899 bytes / 268 original
instructions) is recovered as `bh_model_instance_draw` in `src/bh_model_instance.c`.
The 134-TU integration passes 3,398,718 real-data and 2,906,484 clean source checks,
including 155 focused owner checks and 67 missing-module link controls. The clean
checkout has no original assets. Source/header hashes bind each tested variant;
no matching evidence or original-code execution is claimed.

The static real-level renderer now calls the recovered instance owner for sphere
culling, float-stored translation/Y-X-Z rotations, model-camera composition,
frame/color state and real lit/unlit traversal. Default, red and point-light
snapshots remain byte-identical to the previous checkpoint: 1,798 polygons and
3,112 triangles with 146 textures. Tests cover double-position boundary rejection,
tangent acceptance, cull-state retention, an independent rotation-matrix oracle,
modulation truncation, light dispatch and emitter failure. No game was launched.
Evidence: `m4/model-instance/`; audit: `python tools/check_model_instance.py`.

Next recover recursive world/entity draw owner `0045adf0` after its sprite owners
`00455960`/`004569b0` and real entity binding contracts. Static shell selection,
placement iteration and diagnostic lights remain adapters. Software dispatch and
atlas glyph copy remain unfinished; M4 has not passed.

M4 parallel restoration block, 2026-10-24: **155/727 original functions now
have admitted source variants**, up 31 from 124. This block recovers 10,007
original instructions in 31 complete functions; the count excludes the partial
native primitive dispatcher and all diagnostic adapters. All new owners compile
in their real translation units and link into the 132-TU integration target.
It passes 3,398,563 real-data checks and 2,906,329 checks in a clean checkout
without original assets, plus 66 missing-module link controls. No original code
was executed and no game was launched. Evidence: `m4/parallel-hour/`; audit:
`python tools/check_parallel_hour.py`.

Recovered families include main-camera transforms, point/directional/spot/ambient
light evaluation, native material/depth/blend/mode state, two additional clippers,
view culling, lit/unlit BSP traversal, texture/model controls, software palette
tables and atlas image/page/clear/cursor owners. The atlas glyph-copy owner and
software primitive/rasterizer subsystem remain unfinished. The native-only
primitive adapter is explicitly partial and rejects software use.

The static real-level scene now passes through recovered BSP traversal, clipping,
native queue growth/flush and actual Direct3D2 drawing: 1,798 emitted polygons,
3,112 triangles and146 textures. A separate 71-check native composition scenario
verifies unlit blue, directional red/green, point-light blue and mask-zero blank
images, with missing-owner link rejection. Saved city red/green/point presets
exercise the real light evaluator but are diagnostic lights, not recovered level
lighting. Main-camera positive Y, original near/z depth and zero depth clear are
now explicit source rendering contracts. Static shell filtering, camera setup and
placement binding remain adapters; world/entity/script installation and player
controls still prevent a playable source game. M4 is not complete.

Next frontier: recover the owning world/model draw orchestration and its entity
bindings, replacing static shell selection and diagnostic scene bindings one
small dependency family at a time. Keep software dispatch/rasterizers and atlas
glyph copy explicitly open. Human prologue observations remain independent.

The entries below preserve earlier checkpoint totals and unfinished dependencies
as historical evidence; the current block above supersedes their frontier.

M4 model-space light preparation, 2026-10-24: complete `00450980`
now compiles/links as `bh_light_prepare` in `src/bh_light.c`. The 91-TU
integration passes 664,011 real-data / 171,777 clean source checks and46 missing
module controls. The 991 focused checks cover256 analytical mixed-light cases,
positive-range culling, exact distance/8 radius threshold, spot cosine ramp,
model-space point/direction transforms, original cache retention and override
reference semantics, plus source preflight of invalid/cyclic borrowed lists.

The original0x5c light record layout is retained. Near lights keep point/spot
state; distant lights become attenuated directional approximations. Inactive
branches only clear the prepared tag, preserving stale caches as original.
Globals are explicit context bindings; no light constructor/lifetime claim.
Shading evaluators00450cd0/00451140, visibility and native/software primitive
dispatch remain unfinished. This source owner removes a dependency from the lit
model traversal; it does not change the saved static scene or establish gameplay.
No original code or game execution. Evidence: `m4/light-prepare/`; audit:
`python tools/check_light_prepare.py`. Next: original shading evaluators and
primitive/visibility closure, then scene/entity/control. Human recording stays
independent.

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
5,881 triangles with 146 textures and264,231 nonblack pixels; it was inspected.
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
