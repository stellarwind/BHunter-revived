# Complete weapon-index3 timer callback00433170

This is the sole replacement owner needed after duplicate portal-center credit
was removed. Classification preceded decompilation (`CLASSIFICATION.md`): the
verified ten-weapon table supplies00433170 as index3 method+12c. Its ordinary
complete79-byte/18-instruction RET-ended body has no direct calls and no
CRT/import/compiler/shared-tail signature. Historical authorship remains
unresolved; no constructor or callback table is recovered here.

`export_review.py` freshly verifies every body byte against immutable EXE SHA256
`c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
Full exports and `raw-review.json` pin body SHA256
`ed3614a4b6405995d82d85072ea0c6e11e0e4debfa82d80e58e0bc61e0f541dd`,
end exclusive004331bf. `DefineWeaponLatchBoundary.java` adds only the exact
function boundary to analysis metadata, without executable modification.

Source `src/bh_weapon_latch.c/.h` borrows the actual source BHEntity weapon
allocation through the already-admitted BHWeaponCommonState accessor. Both
original+f0/+f4 words are float timers. Each STRICTLY positive timer subtracts
the actual DOUBLE1/30 constant004891f8, then stores binary32. A small positive
timer becomes negative and remains negative on later calls: there is no clamp,
integer decrement or integer wrap. Zero, signed zero, negative and unordered
values retain exact memory bits. All other allocation bytes remain untouched.
The source preserves masked NaN-comparison invalid sticky behavior and inserts
no floating-point status clearing. No callbacks, globals or vtables are guessed.

`generate_cases.py` pins the raw constant and produces20 independent scalar
instruction-operation raw-word cases plus40 sequential timer states. Cases
include neighboring float values around double1/30 (detecting an incorrect
float step), positive/negative zero, smallest/largest subnormals, normal and
large values, infinities, and both-sign NaN payloads. `static-cases.json` retains
the oracle facts and finite-profile limits; this is not original-code execution
or a universal x87/PC24 equivalence claim.

`tests/weapon_latch.c` passes **63 checks** in the real translation-unit linked
target `build/weapon-latch/bhunter-weapon-latch.exe`. Whole-allocation comparison
checks both timer fields with independent oracle words and preserves unrelated
state. The sequential source-built case crosses below zero and remains there;
the explicit NaN case checks payload retention and actual masked invalid status.
`focused-result.json` records exact source/dependency-object hashes, compile/link
commands and successful run. Attachment's independent whole-body/raw-constant/
source review is `../review/WEAPON_LATCH_REVIEW.json`; it found no discrepancy.

Final current integration and ledger admission belong to parent evidence. No
game/original code, gameplay, input or sound playback executed. No additional
owner was assigned or started after this one; the requested stopping count is300.
