# Failure/retry static contract — 2026-10-24

No game was launched in this block. This review resolves what a later failure/retry scenario must actually observe; it does not validate that scenario.

## Correct the raw-state interpretation

The archived player probes call global `004dfabc` `failure_raw`. That name is too narrow. Complete callback `004225f0–004233a2` accepts event type 0x0e and dispatches event ID at message +8. ID 0x956 displays a completion message, clears player fields +0x238/+0x234, calls `004215c0` and `00422570(0)`, then writes state 1 at `00422eb5`. ID 0x960 writes state 2 at `00422fa6`. The player step consumes state 2 in both paused and unpaused return-to-menu paths. Keep the historical raw field intact, but interpret it as ending/control state; it does not prove mission failure. Event producers and the meaning of the intervening callbacks remain to be traced.

## Destruction sequence and subsequent branch

In original player step `0041e850`, counter `004df6b4` starts at 1 at `004211a9` when the relevant player damage/effect accumulator reaches its threshold. At `00421152`, subtraction of double -0.2 increases player +0x24c by 0.2; comparison at `00421189` is against double 1.0. This is separate from the ending/control state. Assembly uses ESI = player +0xe8 here; ESI+0x164 therefore means player+0x24c. Do not confuse those coordinate systems or trust the decompiler's float-typed player parameter.

The counter increments in the player step and has effect checkpoints 75 and 95. At 200 (`0042027a`), mode 1 with ending state 0 takes reset(0), inventory/effect reset and the original menu-return path. The other branch finds the nearest of eight spawn-table entries, resets there and clears inventory/effects. These are update counts; debugger wall-clock time does not prove their timing. The observed earlier Escape/Y abort never entered this counter sequence and cannot stand in for destruction/retry.

## Next runtime evidence needed

Observe an actual unmodified damage/destruction trigger, counter values 1/75/95/200, mode and ending state at the terminal branch, actual resulting menu or respawn, then a subsequent original episode selection and spawn if retry is claimed. Record mission event type/ID separately for objective completion. Do not force globals, damage values, RNG or success callbacks to manufacture coverage. Batch these observations in a future planned scenario instead of rediscovering them through repeated startup-only runs.

`failure-contract.json` pins evidence hashes. `failure-review/` contains role/boundary classification, complete owning callback assembly/decompilation, state cross-references and small instruction/constant exports. `tools/check_m3_failure_contract.py` compares all 48 exported instruction records and both numerical constants directly to the immutable original EXE. Ghidra's initial function-disassembly CLI request returned only ten instructions even with unlimited rows; that incomplete sample was discarded and the existing read-only ReviewFrontier script supplied the complete body through RET at 004233a2. No reference bytes, annotations, source implementations or integration target changed. M3 remains active.
