# Actual tutorial target spawn and retained probe failure

One original launch, `startup/df50ceaa4826.json`, establishes target creation. The longer `mission-target` scenario **failed** at callback 660 because the diagnostic asserted that entity `+1a8` would remain the launch owner. No second launch was made. `python tools/check_m3_target_spawn.py` audits the retained spawn evidence and the failure separately; its passing result is not scenario acceptance.

## What actually executed

The new native variant compiled, linked and executed through the normal Windows loader, using the already recorded private CFG/registry, DDrawCompat, relative-cursor and CRT heap-input profile. Only verified-child keyboard messages navigated the menu and the three introductory sensors. No computer-use skills, visual identification or desktop-wide input was used. No gameplay position, damage, mission state or event packet was injected.

At completed simulation callback 655, the original constructor's allocator returned entity `250286592` for resolved launch owner `248998368`. The runtime arguments were type 6, bounty 5000, float speed bits `41200000` (10.0), death ID 2013 and seed 629539987. The diagnostic checked that the owner was the scene-ring placement 4827, resolving its `+1c8` surrogate if present.

At the original constructor finish, the same entity had update `004398c0`, event receiver `0043a1d0`, expected type/bounty/death/speed, and both initial owner pointers equal to the captured launch owner. Its five initial curve vectors were all `(770,20,-440)`. This is a bounds-centre initialization, not the raw LEV placement `(760,10,-440)` and not an evaluated moving position.

The script launch returned with the matching arguments and its owner's child head equal to the captured entity. Both allocation/construction and launch-return observations precede entry into the original mission consumer for event 2011, and therefore precede its return. This receiver ordering is observed in this variant; the older handoff run did not instrument creation and receives no retroactive spawn-validation claim.

At callback 660 the same entity retained `+1a4` as the original launch owner, but `+1a8` had changed to `248562912`. Its random state, interpolation phase/duration and curve vectors also changed; damage remained zero. `00438ff0` consumes the current `+1a8` navigation owner in its path-selection work. The constructor's initial equality of `+1a4/+1a8` is not a lifetime identity contract. This was an avoidable probe assumption.

## Failure, preservation and correction

The callback-660 state record is immediately followed by `probe-error` kind 17, verified breakpoint restoration, diagnostic child termination `4d330107`, successful private-key cleanup and native probe exit 1. No original exception or timeout occurred. Original EXE/BIN/MDF/MDS, protected Windows files and original registry views remain unchanged. Source snapshots and compile/link/startup outputs belong to this failed executed variant.

The current `training_snapshot` predicate checks `+1a8 == launch owner` only at constructor finish; subsequent snapshots retain it as observed mutable data. The corrected variant separately compiled and linked, with its own three source snapshots and commands in `target-probe-build.json`. **It has not executed.** No test or runtime evidence is transferred between variants.

The render-position, damage, kill-broadcast and release hooks were armed. None produced a target observation before the stop. Curve-state changes establish path advancement, not an accepted evaluated position/velocity trace. Actual target motion, hits, destruction, event-2013 completion, later pickup/faster target, player failure and retry remain open. This block admits no source-game implementation and does not complete M3.

The next gameplay run should target combat as a new outcome, use the corrected navigation predicate, and obtain the target's actual position through an original simulation query or a reached render submission. The absence of a render sample is not evidence that the target was stationary, invisible or absent.
