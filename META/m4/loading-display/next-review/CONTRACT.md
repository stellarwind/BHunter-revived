# Next source boundary after loading display

Read-only review against the active 348-function ledger, 2026-10-25. No source implementation, admission, original execution, game, input, UI or audio execution occurred in this review. There is still no playable source-built game.

## Decision

Do not assign whole `00411f00` as a bounded next family. Its 654-byte / 189-instruction block-list owner is genuine application behavior, but the existing `bh_blocks` implementation is explicitly a WIP data registry: retained sidecar scripts remain unapplied. Restoring the whole owner requires a missing script parser and real registered VM callback closure. A short outer function is not a closed source assignment.

The smallest closed prerequisite within this dependency is `004109c0` + `00410ba0`, the integer/float script-argument readers (960 bytes / 376 instructions total). They require only CRT routines and explicit borrowed block-variable state; they do not allocate entities or install callbacks. This is a useful parser prerequisite, not a claim to recover block loading or script execution. The separate worker review identifies `00412190` + `004100b0` as a potentially smaller complete input-default binding family; prefer that if the next assignment must complete an immediately callable whole behavior.

## Outer owner and reused definitions

`00411f00` runs only while the global block count is zero, opens a line-oriented block list, sets the script-loading flag, reads at most 600 entries, skips semicolon comments and stops at case-insensitive `end`. Each original block record has stride `0x174`; it initializes defaults, parses its sidecar **before** loading the render/collision models, initializes bounds/state, and classifies collision faces. Only a successful render-model load advances the count. It closes its VFILE, clears the script-loading flag, computes spatial grouping parameters, and loads/flags `water.gif`.

Already admitted direct definitions: `0044b2e0` collision-model load, `0044e060` geometry face traversal, `004507c0` texture-image load, `00458140` model load, `004582f0` texture flags, and VFILE open/close/eof/gets `0047cbd0/0047ce50/0047cea0/0047d050`. Registered face callbacks `004083d0` (bounds) and `00410880` (material classification) are admitted in `bh_world_geometry.c`; public `bh_geometry_model_init` combines their real behavior. Their existing definitions must be reused, not counted twice. `__ftol` and `_strcmpi` are CRT.

The existing `BHBlockLibrary` owns its allocator, array and retained BSP/sidecar payload; it does not contain the original render handles, applied script graph, variable slots or entity links. Do not cast it to the original record layout or silently apply those fields through guessed globals. A future whole owner needs explicit real contexts for model/texture caches, geometry, entity pool and script lifetime; renderer handles must come from the actual admitted model/image definitions.

## Missing parser and callback closure

| Owner | Bytes / instructions | Required behavior / dependency |
|---|---:|---|
| `004111e0` | 3351 / 1080 | Full statement parser; depends on all five readers/builders below and the opcode factory. |
| `004109c0` | 476 / 187 | Integer literal or block-variable argument reader; CRT-only closure. |
| `00410ba0` | 484 / 189 | Float literal or block-variable argument reader; CRT-only closure. |
| `00410d90` | 454 / 153 | Sound statement builder; real admitted sound load `0047b950` plus missing factory. |
| `00410f60` | 276 / 99 | Flag statement builder; missing factory. |
| `00411080` | 350 / 127 | General opcode statement builder; integer reader plus missing factory. |
| `004457c0` | 221 / 56 | Allocates/configures opcode entity and installs **missing `00443ff0`** VM query callback. |
| `00443270` | 221 / 80 | Wait-for template query; installs missing child query `00442f40`. |
| `00442f40` | 202 / 63 | Wait-for child query; forwards completion to a borrowed parent query; switches to admitted `00442eb0` deletion tick. |

The parser's timer callbacks `00443010/00443140` and entity acquire/set-query/release owners are already admitted. The one-byte `0047d680` RET is cataloged as a shared callback sentinel, not a missing independent gameplay owner; preserve its owning callback semantics without counting an empty new function.

The current catalog has no complete function record for `00443ff0`. Historical review bounds a **provisional** executable region `00443ff0..004456e2`, followed by jump-table data `004456e4..004457bf`. Do not infer a full function by linear disassembly across those tables. The factory's raw code-pointer installation proves the callback requirement independently of any decompiler label. Historical direct-call inspection already exposes missing owners `00402e50`, `0043d2c0`, `0043bec0`, `004470c0`, `00441b80`, `0043bca0`, `0043ddc0`, with further registered callbacks/constructor closure still to resolve. `0047ddf0` is CRT rand and is excluded. `provisional-vm-callees.json` is a frontier list, not full VM boundary/admission evidence.

## Bounded numeric-reader contract

Both readers take the current token cursor, borrowed block definition, output value and output selector. They set selector `-1` first; NULL, empty or newline returns NULL while leaving the value unchanged. First-character digit, minus or dot selects CRT `%d` / `%f` parsing; otherwise they scan eight active block-variable definitions, mask at `+0x3c`, signed-short values at `+0x64`, eight names of stride32 at `+0x74`. The first case-insensitive **prefix** match wins and writes selector0..7; no token-boundary requirement is added. Integer values sign-extend; float values convert signed short to binary32. Unknown variables diagnose through CRT printf, preserve the prior value and selector `-1`, and advance to the next token. Current-token skipping and trailing whitespace consumption use CRT classification; caller token preparation is not silently moved into the reader.

`0047dec0` and `0047def0` were compared in full against the pinned VC5 SP3 `libc.lib`: relocation-normalized `_isdigit` and `_isspace`, with the established precision helper as a positive control and an altered-byte negative control. See `ctype-crt.json` and its bounded reproducible verifier. `0047de50` is printf, `0047df20` sscanf, `00488370` `_strnicmp`; these are not gameplay assignments. Preserve parse failures and value mutation semantics instead of replacing them with unconditional success.

Useful future checks are memory-only: numeric literals, signed-short extrema, eight selectors, used-mask gates, prefix-first selection, newline/NULL/empty, unknown token preservation, token cursor/whitespace advancement and locale/CRT sign-character behavior. A real definition must compile/link and execute these checks before admission. No implementation or tests were added here.

## Evidence and limits

`review.json` records complete instruction counts, raw PE equality, body SHA256, current ledger duplicates and direct calls. Every exported body is contiguous through its final RET and matches the immutable reference PE. Ghidra decompile failures for catalog-absent callbacks are retained as failures; no database mutation or fabricated decompile was used. The historical-path hint in those error records does not establish that an old C listing exists. `00443350` was exported as neighboring sound-cleanup context only and is not assigned or claimed as the parser's necessary closure.
