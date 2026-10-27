# BHunter restoration progress

Updated 2026-10-27. Plan: `../RESTORATION_PLAN.md`.

## Current checkpoint

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

