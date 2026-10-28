# BHunter restoration progress

Updated 2026-10-28. Plan: `../RESTORATION_PLAN.md`.

## Current checkpoint

Splash keyboard skipping 2026-10-28: Space, Esc and Enter now use the existing
mouse-click timer-expiry path; repeated keydown is consumed without advancing.
Original0044a160/0044aac0 and stage/deadline xrefs show five-second timer stages;
00473c40 records input without a splash skip. This is a requested adapter change,
not original-backed skipping. 300-TU marker build links. Bounded HAL startup exits
0 and logs completion; existing Python wrapper reports false because the release
build omits its DEBUGGING-only summary. No Python tests changed or gameplay input
automated. Human keyboard confirmation pending; ledger records evidence.

Runtime guard audit 2026-10-28: replaced repeated identity-only active-list scans
in actor/script/projectile/physics/HUD/weapon paths with the existing live-slot
lookup. Generation comparisons and error behavior retained. Flash binding and
actor route removal no longer validate the entire pool on each operation; local
ownership/room-link checks remain. Startup validators and gameplay searches remain.
Focused real weapon lookup workload (2000 calls behind8500 entities): 41.057ms
before, 0.011ms after. Membership/list oracle, growth/deletion/recycling, stale
weapon generation and flash ownership checks pass; existing actor-route lifetime
56 checks pass. Final300-TU marker build and bounded HAL startup pass; map saved
as build/game/mission-profile-v4.map. No timing/thread/RNG changes, Python tests,
software tests or automated gameplay. Human performance confirmation pending;
this is a targeted guard audit, not proof that all stalls are removed. Exact
changed-source hashes and remaining limits recorded in function-status.json.

Chapter-2 broadcast fix 2026-10-28: second human capture
mission-markers-69456234-9716.csv identifies three always_tick callbacks costing
122.950/121.789/121.178ms. VM query's added liveness guard scanned the whole active
list for every receiver; original00444008..0044402a directly rejects irrelevant
events. Reused existing pool chunk live bits through bh_entity_is_active; only VM
guard changed. No new cache, scheduler, thread, RNG or mission logic changes.
Real 8500-receiver source broadcast check: 141.623ms before, 0.785ms after;
state/RNG, deletion, growth, ownership and recycling checks pass. 300-TU game
rebuilt; bounded HAL startup passes. Markers remain enabled; map preserved as
build/game/mission-profile-v3.map. Human smoothness confirmation pending; no
gameplay automation or software-renderer tests. Ledger records exact sources.

Human chapter-2 marker capture 2026-10-28: mission-markers-69242562-15592.csv
localizes stalls to simulation before sound processing: frame151 245.469ms
(message arrival 221.219ms into it), frame181 315.726ms, frame236 417.173ms.
Sound/render/uploads do not explain these pauses; startup separately has a slow
first sound tick. Added v2 markers for input, frame acquisition, entity step/refresh
and sound duplication; callback details retained only when >=1ms. Built and HAL
startup checked, map saved as build/game/mission-profile-v2.map. Root callback and
fix pending another human capture; no gameplay automation or timing change.

2026-10-28: corrected reversed completion sound bindings using original
004225a3/004225ce bytes: ordinary completion uses misacc.wav; chapter progression
uses tk_ec.wav. High-resolution scheduler experiment reverted by user request.
Added opt-in --mission-profile build, independent of DEBUGGING: bounded 65536-event
memory ring records message arrival/panel opening, sound requests/tick, text clear,
glyph blits, texture upload/flush, chapter save, simulation/render/presentation.
Normal exit writes build/game/mission-markers-<tick>-<pid>.csv; forced termination
loses capture. QPC is measurement only; scheduling and gameplay remain unchanged.
300-TU source build and bounded HAL startup pass; CSV contains 310 glyph blits,
51 clears and 28 uploads. No gameplay automation, software tests or Python tests.
Portrait/text/audio resources are preloaded; synchronous graphics operations and
chapter config save are possible stalls, not confirmed causes. Human hitch capture
and audible cue confirmation pending. Evidence: META/function-status.json.
New rules prohibit Python integration-test authoring and Markdown updates outside
this latest checkpoint (apart from the explicitly requested AGENTS.md rule edit).

### Previous checkpoint

Split-projectile retirement correction2026-10-27: human failure capture confirms
queue saturation at frame29058 (3034 trail points+202 headers); first full frame
still retains camera/fullscreen clipping resets. No numerical draw failure;
separate actor NaN at frame747. Found source wall-impact early return on NULL
(no-op) damage callback, before cascade/retirement. Original00404eb2 proceeds
to00404ec1 after RET callback. Removed callback-required gate; missing receiver
remains error. Real split wall fixture fails before, passes after;300-TU game
rebuilt, capture retained and DEBUGGING disabled. All original owner instruction
bytes verified against immutable EXE. Evidence and human capture preserved in
m4/playable/split-retirement. No gameplay automation/software renderer execution.
Human symptom attribution is supported by reproduction, not yet visually verified.

### Previous checkpoint

User-authorized one-shot failure capture added2026-10-27. Independent
BH_FAILURE_CAPTURE build flag; DEBUGGING and periodic profiling remain off.
First overflow, model-pose argument failure and camera/clip/model draw failure
retain independent bounded memory snapshots; no routine entity scans or disk
writes. Normal exit writes failure-capture-<tick>-<pid>.json under build/game.
Records include raw command/NaN words, camera/clip state, entity callback census
and exact failure details.300-TU capture build links; focused source-only real
capture/queue check passes. Exact map/build retained in m4/playable/failure-capture.
Opened HAL/fullscreen for human retry, no automated gameplay. Root cause remains
unconfirmed; abrupt process termination loses capture.

### Previous checkpoint

Human reports freeze/corrupted geometry again with DEBUGGING disabled. Logging
is not the sole cause. Original camera/billboard enqueue review confirms both
use the same capacity gate; no reserved-camera rule found. Current polygon
clipper has no epsilon/zero-denominator skip. Exact failure remains unresolved;
queue overflow and numerical rejection need distinguishing frame-state evidence.
No further launches/tests or source changes. See queue-saturation/diagnosis.json.

### Previous checkpoint

DEBUGGING build gate, 2026-10-27: optional source logs, timing capture/allocation,
per-tick projectile scans, curve dumps, VFILE/script diagnostics and diagnostic
box tracing are compiled out by default. Build with --debugging to enable them;
normal executable rejects profiling/box switches. Both299-TU variants compile
and link. Release binary inspection confirms capture/log strings, profiler
symbols and QueryPerformanceCounter/Frequency imports absent. Launched release
HAL/fullscreen for the user (PID12864), without automated input or gameplay
tests. Human stutter comparison pending; no performance improvement claimed.
Evidence: m4/playable/debugging-gate/validation.json; ledger updated.

### Previous checkpoint

Human rendering-freeze investigation, 2026-10-27 (read-only): latest session
frame-times-00014f05-7960.csv contains 265 produced frames at the exact3500
command limit: frames930-1057 and7406-7542 (about4 seconds each). New effects
refresh before older player scene; full queue drops camera/world/clip commands
without aborting publication. Stale camera and incomplete scene are a concrete
freeze/skybox mechanism. Exact dropped commands and excess effect producer are
not recorded, so symptom attribution remains provisional. Model460 curve NaN
at tick6745 is outside these windows. No tests, builds, gameplay or source
changes. Evidence: m4/playable/queue-saturation/diagnosis.json.
### Source formatting (2026-10-27)
User-requested automated formatting of all 600 src C/header/include files using VS Code C/C++ extension bundled clang-format 23.1.0, Microsoft style. src/.clang-format preserves include order, string literals and comment text; declaration macros configured for stable output. Formatter dry-run passes. Lexical tokens (after line splicing, excluding comments) match the initial working-tree backup for every file, including pre-existing user edits. Backup: build/formatting-20261027-205220/before.zip. Final formatter evidence: build/formatting-20261027-205421/validation.json. No build or game execution; no implementation admission or fidelity claim; function ledger unchanged.

