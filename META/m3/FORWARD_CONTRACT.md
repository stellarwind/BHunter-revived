# Diagnostic forward-direction correction

The original STINGER thrust and ion-gun path use the **positive** player forward basis. The diagnostic combat policy inherited a negative-forward convention from the introductory sensor steering policy. This was a policy error, not evidence of an original altitude ceiling or a broken original controller.

## Evidence for the sign

The complete STINGER controller `00416120` computes local forward velocity as its world velocity dotted with basis `+30/+34/+38`. At `004165ee–0041660b` it subtracts the input at `+194` multiplied by either float `004896a0 = -30.0` or float `004894f0 = -10.0`. Thus positive acceleration input increases the requested local forward velocity, before the remaining clamp/actuator solver. For example, a zero local velocity and command +1 produce requested local velocity +30. This is not a claim of an instantaneous 30-unit world velocity change; the rest of the physical model still executes.

The retained naturally exiting motion run `dfa3b4981226` independently records positive float input `3f800000` at `+194` while the actual A acceleration key is held. This establishes the input sign without assigning a key from a label alone.

`00417c50` is the gun-direction owner called by the player step. With zero gun aim offsets at `+1f0/+1f4`, its explicit branch copies basis `+30/+34/+38` to gun vector `+280/+284/+288`. The offset branch adjusts that positive-forward direction through the right/up basis and the original nonlinear/trigonometric calculations.

The ion held callback uses float `00489554 = -150.0`: projectile velocity is player velocity minus gun vector times that constant, plus original shot dispersion. Projectile step `0043ea90` computes the next position as position minus projectile velocity times double `00489208 = -1/30`. The two reviewed operations therefore produce motion in the positive gun-vector direction. No original shot is claimed from this static calculation.

`forward-contract.json` and `python tools/check_m3_forward_contract.py` verify 17 instruction anchors and four signed constants against the pinned EXE, plus complete reviewed listings/hashes. The broad memory-write survey is only a way to locate candidate owners; matching an offset is not type/provenance or source-completion proof. Original CRT pow support is not assigned as gameplay.

## Corrected diagnostic policy

Only the combat policy changes. Its forward projection and forward velocity now use positive basis.forward. The old introductory calibration deliberately measured negative basis.forward, so the combat turn policy reverses both measured response signs when steering positive forward. The retained introductory route and its archived evidence remain separate.

The unsupported vertical-climb prerequisite is removed. The revised approach retains the player's initial altitude while crossing horizontally toward the target, then uses the predicted target aim point at close range. Orientation, actual gun direction, velocity and local aim projections are now logged together. These are diagnostic input-policy choices, not recovered original AI or flight mechanics. Obstacle avoidance, successful pursuit and combat still require actual original execution evidence.

Source-only controls exercise the actual corrected projection/turn helpers with independent Cartesian fixtures, alongside the existing 18 curve coordinates. The real native debugger TU also compiles and links. `combat-direction-control.json` binds each result to its own snapshots; `combat-direction-control-before-telemetry.json` retains the preceding build. No original code runs in either control, and neither is a source-game implementation admission.

The static contract checker deliberately does not claim a successful original combat run. Any subsequent execution must be recorded separately with its own source snapshots, original position/projectile/damage observations and outcome.

## Corrected original attempt: failed approach

One separately recorded launch, `startup/469b5ddce4f6.json`, executes the corrected policy. Its four source snapshots match the corresponding source-only control inputs. This run does not rewrite acceptance of either earlier failed variant.

All 191 policy observations use phase 2 and the positive-forward convention. Actual gun direction equals actual positive forward basis in every captured sample, and the reported forward velocity agrees with the independent dot product of recorded velocity and basis. The policy requests acceleration in 181 samples; these are held-state observations, not 181 distinct key presses. Actual input storage ends with positive acceleration command `3f800000`.

The approach still fails. Sampled player X stays approximately -68.4..-42.6 and Z -132.4..-97.2, while the nearest target-distance estimate is 871.789. No phase-3/firing state is reached, no Ctrl input is sent, ammo remains 300, and no player projectile, target damage or kill is observed. Late gun alignment exceeds 0.97, but this alone does not establish a reachable route or line of sight. The run stops at callback 1800, with verified restoration/cleanup, original preservation and native probe exit 1. No original exception, timeout or diagnostic assertion occurs.

`python tools/check_m3_forward_attempt.py` audits these facts and returns evidence-audit success with `scenario_passed:false`. Contact/obstruction was not instrumented for this variant; geometry, a route restriction or another control effect must not be asserted as the cause from these records alone. No second launch follows this attempt.

The next useful step is to inspect the existing level/navigation data and graph construction before choosing another route. M2's `TEST.PTH` reader exposes a destination/source matrix of **local exit slots**, not global neighbouring-node IDs; it does not supply a fabricated live graph. Node positions, exit ownership and passable connections must be established from their real owners. Repeating the direct approach unchanged would not resolve this missing evidence. M3 remains active, with shots/hits/kill, mission completion and failure/retry still missing.
