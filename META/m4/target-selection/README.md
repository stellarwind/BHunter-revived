# Weighted target selection, 00421610

`src/bh_target_selection.c` restores the complete 304-byte owner in its own real
translation unit. `build/target-selection/target-selection.exe` links it against
the existing entity allocation/query registration, FP environment, float-store
rounding owner and real CRT. The standalone fixture passes **115 checks** and
the link control fails on `_bh_target_select` when its implementation is removed.
Parent integration/admission remains a separate check; this evidence does not
claim a playable source game or byte matching.

Reproduce from the repository root:

```
python META/m4/target-selection/audit_original.py
python META/m4/target-selection/generate_numeric.py
python META/m4/target-selection/build_evidence.py
```

`review.json` contains the complete static contract, provenance, dependency
ownership and limits. The existing pre-decompilation player-frontier review
classified the complete frame/RET and incoming actor behavior as game code.
The byte audit confirms every one of its 107 instructions against the pinned
immutable EXE. The live Ghidra project currently has 744 functions and lacks
this owner boundary; the retained complete export was used without modifying
the database or original image. Retained caller edges and a separate raw CALL
scan agree on call sites 00439059 and 0043a2c6.

The two distance branches intentionally differ. The player retains `dy*dy`
but multiplies retained dx/dz by their stored float values; list actors retain
`dz*dz` and use stored x/y multiplicands. `bh_round_float` supplies these actual
float stores. The independent 16-case binary64/PC53 fixtures distinguish the
branches and catch replacing them with an ordinary symmetric distance norm.
They are source-contract fixtures, not differential original execution.

Weights remain signed exact integers through conversion, including values
above float integer precision, zero and negatives. Query callbacks fill the
original 40-byte packet and may mutate the actor weight/successor before those
fields are read. No production actor callback or original global is invented.
The first eligible player remains a candidate even when excluded from the
actor-list pass. Strict comparison occurs before the new float score store.
The original FCOM C0 also selects unordered scores; tests cover NaN incumbents
and candidates with masked exceptions, plus zero/negative divisors. Two equal
retained nonaxis distances can select the later actor when the first incumbent
rounded upward to float; ordinary exact ties retain the previous candidate.

Caller supplies PC53, nearest rounding and masked exceptions, just as other
current numerical owners. PC24/PC64 behavior and historical runtime precision
transitions remain open. Signaling-NaN exception/status side effects are not
validated. Bounded traversal/null/callback failures are explicit source guards
outside the original valid-domain contract. Callback effects and partial
selection are preserved on guard failure. No game or original code was run.

Parent integration instructions: compile `src/bh_target_selection.c` and
`tests/target_selection.c`; include both headers and
`tests/target_selection_cases.h` in source hashes. Register
`bh_target_selection_checkpoint(unsigned long *)` as scenario `target-selection`,
and include a missing-`bh_target_selection.obj` link control. Existing
`bh_entities`, `bh_fp`, and `bh_round` definitions are its complete link closure.
