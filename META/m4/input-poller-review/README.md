# Input snapshot evaluation: 00474020 / 004756b0

The directory name records the initial hypothesis. Inspection of the complete body established that 00474020 has no calls: it evaluates bindings against existing input snapshots and writes a compact packet. Device acquisition and DirectInput lifecycle remain outside this implementation.

The source owners are `bh_input_evaluate` and `bh_input_set_enabled` in `src/bh_input_evaluate.c`. Their explicit `BHInputSnapshot` preserves the original key-array aliases, axis selectors, history consumption, packed pairs, Boolean batch padding, and packet tail lifetime. It does not fabricate device state or install production callbacks.

`tests/input_evaluate.c` exports `bh_input_evaluate_checkpoint`. The focused `input-evaluate.exe` target passed 37 checks, including all binding kinds, every axis selector, key edges and full state, modifier priority, NaN clamping, alias-sensitive history mutation, both nonempty Boolean batches after an analog record, and round trips through the real restored packet decoder. No game, hardware, or original code was executed.

`standalone-build.json` binds that focused result to exact source hashes and command lines; its `sources/` snapshots retain those exact bytes. Parent integration may normalize line endings afterward, so the canonical integration evidence must bind its own source hashes and must not inherit byte-match claims. No byte match is claimed here.

Invalid counts, unterminated names, invalid indices/history heads, and packets exceeding the real 0x120-byte state allocation are outside the original valid-memory domain. The source guards reject invalid inputs without publishing a partial packet. The caller must keep the list and snapshot nonoverlapping. In the wrapper's valid mode-zero path, publishing flags zero after successful evaluation is equivalent to the original earlier store; invalid-input atomic rejection is an explicit source adapter behavior.

Original bounded decompilation and full instruction TSV exports live in `original/`. Their bytes were checked against the immutable pinned EXE. `review.json` records classification, contracts, source ownership, and limitations. The separate canonical independent review records the raw branch and packet audit, including the corrected unordered analog clamp.
