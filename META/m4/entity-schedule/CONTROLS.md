# Native draw-control closure

`src/bh_draw_controls.c/.h` owns source variants of these directly observed functions:

| Original | Bytes / instructions | Role and boundary |
|---|---:|---|
| 00478990–00478a41 | 178 / 57 | game/engine native queue producer for one texture-mask state command |
| 00478a70–00478b36 | 199 / 63 | game/engine native queue producer for one rectangle command |
| 00477f40–00477f55 | 22 / 9 | queue state consumer; invokes 00458300 and returns the current next link |
| 00477e40–00477e70 | 49 / 20 | queue rectangle consumer; coalesces when the next command is another rectangle, otherwise invokes 004515b0 and rereads next |
| 00458300–00458309 | 10 / 3 | shared texture mask setter; stores the argument at 00491778 |

These are runtime render behaviors with direct callers/command payloads, not CRT, imports, compiler support or detached tails. 004515b0 is already recovered as `bh_view_clip_rect`. The source queue uses semantic command kinds and stable indices instead of original x86 code pointers. The source mask setter binds the one shared scalar to the render context and propagates it to the prototype and every loaded model instance so later model draws see the same value. This is an explicit source binding of the observed global behavior; no game code is loaded.

`tests/draw_controls.c` exercises producer payload bits, all nine bucket mappings, saturation and invalid queues; consumer state and rectangle dispatch, adjacent rectangle coalescing, asymmetric/inverted view bounds and propagation to model/screen/attribute clipping contexts. Its real-data branch mounts the immutable archive as data, loads `BSP\\PU_9A.BSP` through the real source model/resource owners and checks mask propagation to the owned model instance. The focused linked target passed 37 clean checks and 45 real-data checks. There were no original-code calls or game launches.

Original static float bytes at 00489448–0048945b specify overlay distance/fade boundaries 30, 180, 60, binary32 1/30, and 150. 00489288 is 0.5 and 0048930c is 1.0. The portal near and far values are not static constants: 00409130 copies its incoming arguments into 004d1a7c and 0049631c immediately before calling 00409f40. The scheduler binds those two values from `BHViewConfig.near_z` and `far_z`. Exact 30/60/150/180 overlay transitions and each clip plane are covered by `tests/entity_schedule.c`; its focused linked target passed 64 checks.

Source masks, queue operation order and rectangle state passed linked checks. Actual main-game renderer output, software rendering, and exact 24-bit x87 transitions remain open. The full current integration target and ledger are owned by the parent block.
