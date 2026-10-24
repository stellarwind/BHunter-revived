# Mission event producers and capture targets — 2026-10-24

This block launched no game. It traces the producer side of the ending/control states identified in FAILURE_RETRY.md and narrows future observations to actual data rather than repeated menu exploration.

## Original event path

World bootstrap 004474c0 allocates an entity and installs callback 004225f0 through the real event-registration owner 00476e90. The scene loader creates timer entities whose step callback 00442de0 sends event 8 with the placement owner. Script entity factory 004457c0 installs event callback address 00443ff0, stores opcode at +0xf8 and parameters at +0xfc/+0x100/etc. The existing parser maps `broadcast` to opcode 18.

00443ff0 is absent from the decoded Ghidra function catalog. Read-only objdump inspection establishes a provisional code region ending at RET 004456e2, followed by jump tables through 004457bf. This region is an unresolved engine/game boundary, not a newly admitted gameplay function. The linear objdump artifact includes table bytes decoded as apparent instructions after 004456e4; those lines are data, not valid executable coverage. No function creation or project annotations were made.

The 25-entry opcode table at 004456e4 routes index 18 to 00445488. That branch builds event 0x0e, supplies the event ID from script entity +0x100, and calls the real entity query owner 004770f0 at 004454c4. This connects script broadcasts with the event type consumed by 004225f0. Field substitutions from placement slots are visible at the start of the callback, but complete define/slot and activation semantics remain unreviewed.

## Real data for targeted capture

The archive has nine object sidecars containing `broadcast` statements. Raw TEST.LEV link slots provide these useful candidates:

| Placement | Object | Position | Relevant raw slots |
|---|---|---|---|
| 4939 | SENSOR2.BSP | (-60, -20, 60) | (1, 0, 0, 2010, 3332, 0, 0, 0) |
| 5278 | RELAY3.BSP | (-520, -40, 480) | (2382, 2390, 0, 0, 0, 0, 0, 3) |
| 5279 | RELAY.BSP | (-480, -40, 480) | (2390, 2391, 0, 0, 0, 0, 0, 6) |
| 5282 | RELAY.BSP | (-450, -40, 480) | (2393, 2400, 0, 0, 0, 0, 0, 20) |

The sensor script waits for an activation message and then creates its sensor behavior. RELAY waits for a trigger and timer before broadcasting; RELAY3 nests three trigger waits. These raw slots and script texts identify candidates, not proof that the player activated them or a complete mapping of each slot to a named define. 2390/2400 correspond to dispatcher cases 0x956/0x960; 2010 is the introductory case 0x7da. Future capture must record actual event sender, ID, activation state and resulting behavior rather than force callbacks or infer activation from proximity.

## Completion side effects

004215c0 walks the world list and sends event 0x0f to entities with category field +0xfc equal to 1. 00422570 raises signed progress value 00496a3a only when its argument is greater, calls the existing persistence owner on increase, and schedules a sound notification. 00437a80/00437ab0 likewise allocate notification entities using two sound globals. Their step 00437a30 counts down, plays the selected sound and frees its entity. These are distinct from a menu transition, mission restart or an actual audible-playback validation.

`mission-inventory.json` contains all candidate records, original script text/hashes, 25 table targets and review artifact hashes. `python tools/m3_mission_inventory.py` recomputes them from pinned EXE/BIN bytes without executing game code. The normal source integration and all original files remain unchanged.

M3 still needs actual objective completion and destruction/retry captures, saved-config reload, broader feature/mode execution, sound/CD loop and network observations. Input schedules and sampled RNG state do not yet establish deterministic world replay. Static closure of this producer path does not close those runtime gates.

The later original keyboard-flight capture in INTRO_SENSOR.md reaches placement 4939 and delivers event 2010 through the real handler. It supplies actual resolved world bounds and activation evidence for that one introductory candidate; ending events and wider activation chains remain separate.
