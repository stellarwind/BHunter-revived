# Original target movement; unsuccessful combat approach

One new original launch, `startup/454bf71975bd.json`, captures actual evaluated target motion. The intended combat scenario **did not pass**: the diagnostic policy remained in its climb phase until the 1,800-callback observation limit and never pressed fire. `python tools/check_m3_combat_attempt.py` audits this distinction. There was no second launch or retry with the same policy.

## Actual movement observation

The same attributed training-target creation contract executes at callback 655: allocator result, completed type-6 constructor and script launch return agree on owner, bounty 5000, speed argument 10.0 and death ID 2013. The corrected `+1a8` predicate allows the original navigation pointer to change without treating it as an identity failure.

A breakpoint at `0043a3df` captures the original event-3 position-query result after all three position stores and before its original epilogue. ESI identifies the target, EDI identifies the query packet, and packet `+4..+c` supplies the actual float result. The debugger executes and rearms the real `POP EDI`; it neither calls the query itself nor substitutes its result.

The trace contains 32 distinct raw position triples, one per callback 655–686. The first is `(770,20,-440)`; the last decodes to approximately `(768.877258,13.2437954,-439.366699)`. Their displacement is 6.8780752736 units. These observations establish bounded motion of this original entity, rather than inferring movement from a speed field or changing control points. They do not establish constant speed, complete navigation behavior or a full-world deterministic replay.

The render-position hook produced no sample. That absence does not negate the simulation-query results or establish whether the target was visible.

## Diagnostic estimate and input policy

`combat-review/` classifies and exports four complete owners before use: interpolation `004341a0`, actual selected ion-gun trigger `0042f730`, held callback `004305f0`, and projectile constructor `004408e0`. Their provenance remains engine-candidate, not established game ownership. Verified runtime helpers remain outside recovery assignments.

The diagnostic steering estimate reads original curve controls `+120/+144/+150/+12c`, phase/duration `+180/+184` and displacement `+15c`. It approximates the reviewed endpoint branches and interior blend in host arithmetic. Every estimate is labelled `original_position_observation:false`. It is not a recovered/admitted numerical implementation or a substitute for the original query trace. Main-return estimates and queries occur at different points within updates, so equal callback indices are insufficient for a numerical differential comparison.

The policy-only source control compiles, links and executes 18 coordinate fixtures covering both endpoint branches and two interior samples. It executes no original code. Its C/header snapshots match the executed debugger variant; `combat-policy-control.json` retains commands, snapshots and limits. It does not prove original numerical equivalence.

The policy tried to climb to Y below -80 before crossing toward the target. All 191 policy observations remained in phase 1; minimum sampled Y was about -18.728. It posted two brief thrust pulses at callbacks 848 and 884, with steering/brake input at other points, but no Ctrl/fire message. Ammunition remained 300. The climb/alignment precondition therefore prevented the planned combat approach. This is evidence against the diagnostic policy, **not proof of an original altitude ceiling, broken original controls, absent weapons or an invulnerable target**.

The STINGER controller's existing complete export implements coupled orientation/velocity and actuator behavior, rather than the free-flight model implicitly assumed by a vertical climb waypoint. The current records do not isolate a hard vertical limit or the cause of the failed alignment. A new attempt must resolve that control assumption and record the necessary orientation/actuator state; repeating the unchanged climb policy is unjustified.

## Weapons and lifecycle limits

Static review confirms the trigger callback itself does not construct a shot. The held callback checks its state/ammunition, decrements ammunition, and calls `004408e0` at `004307de`. The projectile constructor stores attacker at `+130`, actual origin at `+0c`, velocity at `+48`, and installs step `0043ea90` / event receiver `00440810`. The new native hook at its finish filters actual player attribution. These are reviewed boundaries, not successful projectile/damage observations in this run.

No player ion projectile, target damage callback, target death broadcast, target release or event 2013 was observed. The child reached callback 1800, restored/verified all breakpoint bytes, terminated diagnostically, removed its private key, and returned native probe status 1. No original exception, timeout or diagnostic assertion failure occurred. Target kill, later pickups/faster target, mission completion, player destruction and retry remain open.

All original references, protected Windows CFG/INI files and original registry views remain unchanged. Input was limited to PID/HWND-verified child messages. No computer-use skill, screenshot, visual identification, desktop input, gameplay-state/event injection or mapper expansion was used. This block changes original-reference evidence only; source-game admission counts and M3 completion remain unchanged.
