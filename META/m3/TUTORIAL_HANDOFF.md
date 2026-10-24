# Uninterrupted flight to the training handoff — 2026-10-24

One original run `80df3c4686e1` flies through three real sensor regions in sequence, without another menu entry or restart between them. Original events 2010, 1100 and 2011 are received and return through the original mission consumer at callbacks 544, 601 and 655. The new mode is `--play-trace mission-tour`; the original single-sensor mode and its evidence stay separate.

| Placement | Event | Actual resolved world bounds | Handler return callback |
|---|---:|---|---:|
| 4939 | 2010 | (-90,-25,30)..(-30,-10,90) | 544 |
| 5055 | 1100 | (-70,-25,-60)..(-40,-10,0) | 601 |
| 5064 | 2011 | (-70,-15,-120)..(-40,10,-90) | 655 |

The keyboard policy calibrates actual steering signs once, follows the original scene ring for each target ordinal and reads its resolved bounds. On a waypoint handler return it releases policy-held keys, reads the next target and continues the existing original flight. There are 41 flight observations. Actual captured positions fall inside every region before its corresponding event. No position, velocity, health, mission flag or event packet is written into the game. Original physics, collisions and event dispatch execute.

The original event-2011 text assigns an instructor and asks the player to locate and destroy a slow-moving training target. Raw briefing text/address and actual LEV sensor/launcher/relay records/scripts are retained in tutorial-route.json. The LAUNCH2 placement 4827 has raw slots (2011,1,2013,6,5,10,0,0), providing a next producer to observe. This is static target data, not evidence that it has spawned. The diagnostic stops immediately after the mission consumer returns; the rest of the broadcast's receivers need not have run yet. It does not prove the target spawned, died, emitted 2013, or completed the prologue. No future expected kill result is invented from briefing text.

## Observed prefix repeatability without another replay

The already completed single-intro run 14bdee1a5329 and this independently compiled/executed variant have identical common-prefix records: 20 selected player samples including raw position/velocity/control words and sampled RNG states, 37 key events before callback 544, and 23 printed flight observations through callback 542. Both capture initial RNG state 1 and x87 control 027f at recorded boundaries. Heap/module addresses are not compared as gameplay state. The exact selected frame-indexed trace and canonical digest are retained in intro-prefix-trace.json.

`python tools/check_m3_intro_repeatability.py` reconstructs that comparison from each run's own source snapshots and evidence. This is observed repeatability for sampled fields on this prefix, not instruction matching, full-world state equality, fixed wall-clock/driver scheduling or cross-machine deterministic replay. Printed flight floats are not claimed to be raw bit matches. No additional game launch was used for the comparison.

## Provenance and limits

`python tools/check_m3_tutorial_handoff.py` verifies the native/driver/policy variant's real compile/link/run results, ordered owner/event/return chain, actual bounds entry, key transitions, original LEV/script records, restored instrumentation, private cleanup and all immutable/protected reference invariants. tutorial-handoff-validation.json records the bounded scenario. The stop is diagnostic; existing normal-quit evidence is not transferred to this run. Existing DDrawCompat, heap/private profile and relative-cursor adaptations remain qualified. No computer-use skill, visual identification or desktop-wide input was used.

M3 remains active for actual target/mission completion, destruction/retry, fuller sound/network behavior and remaining evidence. Neither three sensor activations nor the scoped repeatability result substitutes for those requirements. Normal source integration/recovery counts remain unchanged.
