# Batched original feature trace — 2026-10-24

One combined launch `5fc01fdfc4f6` shares startup and GAME/STINGER/prologue selection across mission-event, weapon-dispatch and camera observations, then executes original abort/menu/quit and natural process exit 0. No follow-up replay was needed. The new native probe variant compiled, linked and ran; its source snapshots/hash chain is separate from older play/reload variants.

## Input and actual dispatch

The actual selected profile is checked before replay: primary Ctrl (17), secondary Space (32), tail view F2 (113), cockpit F1 (112). All key messages target only the verified child HWND. Ctrl is held at callbacks 420–428 and Space at 440–448. Four original indirect callsites execute and return:

| Input | Callsite | Callback | Meaning of evidence |
|---|---|---|---|
| Primary trigger | 00420ac9 | 0042f730 | Actual selected weapon trigger entered and returned |
| Primary held | 00420ae8 | 004305f0 | Actual selected weapon held callback entered and returned |
| Secondary trigger | 00420b61 | 00433dc0 | Actual selected weapon trigger entered and returned |
| Secondary held | 00420b80 | 0047d680 | Known original common RET sentinel, not independent gameplay behavior |

Callsite stack weapon/player arguments are checked against actual player/weapon pointers. Return EAX is recorded raw, not interpreted as a success code. Primary selector is 0 (ion gun by original selection/menu ordering); secondary is 9 (static coating), consistent with original reset supplying primary 0 and secondary 9. This is not homing-missile fire evidence. Callback dispatch does not establish projectile creation, impact/damage or audible output; entity pool counts include unrelated background activity and cannot substitute for projectile attribution.

F2 at 470 requests selector index 1, which was already current. Its actual selector call receives the player's body pointer and index 1. F1 at 480 changes captured player camera index from 1 to 0 by callback 482. That establishes an actual camera control/state transition without visual fidelity acceptance. Neither field nor callback result is patched.

## Original events and explicit negative scope

A checked, stepped/rearmed breakpoint at the complete original mission consumer captures event type 0x0e, receiver, sender and ID. At prologue initialization callback 400 it receives IDs 9991 and 3332. No introductory ID 2010 or ending/completion ID 2390/2400 is captured in this stationary scenario. These two events are observations, not completed objectives. Sampled destruction counter remains zero. Objective completion and destruction/retry remain unvalidated.

## Lifecycle and provenance

Original quit action, worker-finished/shutdown flags and all six cleanup-stage returns occur; instruction bytes restore and verify before original WinMain/CRT exit code 0. The probe does not terminate this successful child. Existing private SourceDir/profile, pinned DDrawCompat, explicit 128 MiB original CRT initial-size input, denied cursor warp and native relative-coordinate adapters qualify the environment. Original EXE/BIN/MDF/MDS and protected Windows files/registry views remain unchanged. No computer-use skills, visual identification or desktop-wide input. Sampled x87 controls remain 027f; this is not deterministic world replay.

`python tools/check_m3_feature_trace.py` verifies the retained evidence without launching the game. `feature-trace-validation.json` records the narrow results. Original callback ownership/boundary classifications are retained in feature-probe-review/assignment.json, including the shared sentinel exclusion. Normal source integration remains unchanged. M3 remains active for objective/destruction/retry, full feature/mode behavior, sound/CD loop and networking observations.
