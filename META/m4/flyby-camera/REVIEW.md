# Complete flyby camera update 00429fb0

2026-10-25. Classified before implementation as game-owned with high confidence:
the real camera constructor installs this entity update callback, and its
complete body combines selected game start records, navigation transitions,
actor/camera pose and literal BHUNTER credits. Complete contiguous boundary is
`00429fb0..0042a9ab`, 2556 bytes / 757 instructions, RET ended. It is neither a
verified library/import/CRT function, compiler EH/unwind helper nor shared tail.
The original CRT transcendental dispatch is a dependency, not gameplay recovery.

`../scene-callers-review/extract_flyby.py` verifies every exported original byte
against the immutable PE and records all numerical constants plus all fifteen
literal credits blocks in `original-review.json`. The source credits header is
generated from those exact bytes, including newlines, spacing and the game's
custom-font backslash characters. A second agent independently compared every
string/gate/coordinate rule; see `credits-data-review.json`. No original code
is mapped or executed by the new source or its tests.

`src/bh_flyby_camera.c` implements every original behavior branch:

- The fixed 1/30-second actor orbit timer, with the special credits position
  advancing by an additional 1/60 second. Raw constants are negative, so the
  original subtracts increase time. Actual32-byte start rows are reused from
  the source start-pose type. Their final two preserved words are interpreted
  as float horizontal/vertical orbit amplitudes by this owner.
- Stable orbital motion and initial selected-waypoint planning through real
  `bh_route_plan`, all three route-parameter intervals, source interpolation,
  both source route-advance crossings, and final original mode8/11/10 choices.
- The original .6/.4 actor position filter, two position histories,
  motion/curvature-derived yaw/pitch/roll, thirty-Hz velocity and rotated basis.
- The full real `bh_camera_state_update` path, including actual modes,
  transitions and any installed recovered correction owner.
- Both credits branches, yaw arm/trigger windows, shared gate/index and wrap,
  actual font-width centering, blank-line spacing, and actual deferred clear/
  text command producers. Full rings still advance gate/index and attempt all
  commands, preserving original drop behavior.

No behavior call is replaced by a test callback or constant-success stub. Route
and camera definitions are real source, and text command execution later uses
the real font/atlas/native owners restored in this same work block. The page
and atlas metrics must be live while producing; actual page image and native
surfaces must remain live through deferred consumption.

The source context deliberately represents scattered original global owners
and the actor's recovered fields rather than asserting a guessed native entity
layout. It borrows the actual source navigation/world ring, selected waypoint
positions, the same start records used by player reset, font page, text ring
and shared credits state. The selected route-position projection must match
the selected start record. Full original start-table/startup construction and
the larger scene render callbacks are separate owners.

Independent raw instruction review corrected several decompiler traps before
validation: Y/Z orbit trigonometric products have double spills; all axes form
the trig product before amplitude multiplication; current movement norms retain
one factor while reloading the rounded other; old movement norms square rounded
factors; yaw reloads rounded movement deltas; curvature quotients remain double.
Filtered positions are binary32 before movement differences. Phase comparisons
test the retained phase-plus-rate sum even after publishing binary32 state;
source regression cases exercise all three thresholds where the stored sum
rounds back to the boundary. At phase zero an initial zero parameter rate is
valid and unused until actual route planning replaces it, matching construction.

Source guards reject unsafe/uninitialized table, phase, amplitude and binding
cases before their guarded reads. Dependency failures can follow earlier
sequential actor/state mutations, as the original side-effect order does; this
is not a blanket transactional update API. Moderate finite PC53 behavior is
covered. Original PC24 behavior, extreme transcendental arguments and gameplay
fidelity remain open; this is functional source restoration, not byte matching.

`tests/flyby_camera.c` currently passes 52 checks. Coverage includes initial
zero-rate steady/transition state, a nonzero orbit fixture, all phase branches,
both real route-advance call sites, all destination mode choices, position/
velocity/angles, actual camera mode lifecycle, credits introduction/arm/trigger/
index wrap, real font-width and text-ring semantics, completely full and
partially full rings, invalid-state guards, and all retained-boundary regressions.

`check_standalone.py` compiles the actual flyby source/test TUs and actual new
route, curve, modes, camera-state, font-text and text-ring source TUs, links
named `build/flyby-camera/flyby_camera.exe` against real remaining definitions,
and executes the checks. Exact commands/output are in `standalone.json`.
Parent current integration/affected checkpoint results and ledger records are
required for admission. Source tests exercise finite state updates only; no
game, automatic controller, menu or gameplay loop was executed.
