# Timed restoration block: input, rendering and actor routes

2026-10-25. User-authorized 90-minute source restoration block, with medium Sol restoration agents and a low Sol static-review agent. Exact start/stop times are in `session.json`.

**33 complete original owners admitted to the source harness; 267/727 total.** The 21 real implementation translation units and their tests are part of the full 249-TU `build/m0/bhunter-integration.exe`. Every new owner has its complete contiguous original body verified against the immutable EXE, a classified role and boundary, real linked dependencies, and affected source checks. Engine authorship remains unresolved where marked `engine_candidate`; no CRT, compiler support, import or shared tail is counted. No instruction-match claim is made.

| Family | New complete owners | Source evidence |
|---|---:|---|
| Horizon queue, geometry and depth subdivision | 5 | `../horizon/REVIEW.md` |
| Material queue | 2 | `../material-queue/REVIEW.md` |
| General sprite quad and its queue | 3 | `../sprite-quad/README.md` |
| Screen quad and its queue | 3 | `../screen-quad/REVIEW.md` |
| Radar marker | 1 | `../radar/REVIEW.md` |
| Weighted targeting, clamping, route choice/curve/guidance, axis snapping, actor route promotion/update/deadline/cleanup | 10 | `../target-selection/review.json`, `../target-steering/review.json` |
| Input messages, collection, device initialization and shutdown | 4 | `../input-messages/REVIEW.md`, `../input-collection/README.md` |
| Three-buffer initialization, acquire and publication | 3 | `../frame-buffers/README.md` |
| Generic queue reservation and real render-mode callback | 2 | `../queue-reserve/REVIEW.md` |

The actual integration passed **3,839,979 checks**, including **303,179 focused checks across 28 scenarios**. An asset-free Git-index checkout built from another working directory passed **3,347,025 checks** with identical source hashes. Both ran **120 missing-module link controls**. Static fixtures cover 20,561 original bytes / 5,649 instructions as boundary and contract evidence, not a source-completion metric. The final queued mode callback additionally flushes an actual pending red native triangle and verifies the device mode and pixels. Native fixtures render actual source geometry/textures through the existing real Direct3D2 owners, including multicolor UV controls, clipping, horizon gradients and radar rotation.

Static review also corrected the existing rectangle consumer's positional UV interpretation: U start, U extent, V start, V extent. Six prior translation-unit variants remain historical, with fresh source hashes and tests attached only to the corrected variants. The incomplete `00454fc0` native adapter remains WIP. A separate correction binds live near_z (`0049175c`, written by `00451720`) rather than its initialized value; horizon uses near_z/z, radar near_z+.001 and the screen quad near_z+.01. Non-default near-plane fixtures catch the former assumption.

Repository LF normalization changed 21 source/header/test byte variants. `line-ending-variants.json` maps each saved prior byte snapshot to its canonicalization-stage hash; the final checkpoint dispatcher subsequently adds the last family. All affected focused builds were rerun after normalization. `sources/` pins the entire final compiled context and validation tools; historical byte-match evidence is never transferred between variants.

No original code or game was executed. No automated gameplay, native input capture or cursor recenter occurred. Isolated source API fixtures validate capture/recenter requests; actual read-only device/cursor/key translation and real kernel mutex operations are separately exercised. Original EXE/BIN/MDF/MDS hashes remain intact.

The native horizon emitter, explicit frame/input/resource bindings and buffer lifetime helpers are recorded separately as adapters. They do not implement missing application/worker lifetimes or software raster dispatch. Source validation is PC53/nearest/masked and bounded finite cases; PC24, extreme/transcendental equivalence, CD audio and DirectPlay remain unresolved. Both full scene callbacks remain unfinished. There is **no playable source-built game**, M3 is skipped, and M4 remains incomplete.

Run `python tools/check_frontier_90min.py`, `python tools/audit_function_status.py` and `python tools/m0.py verify-reference` for the current frozen evidence. `build_evidence.py freeze` preserves existing matching source admissions when extending a block; it rejects changed implementation bytes under an existing admitted ID. It was run at the 31-owner stage, saved in `prior-31-checkpoint/`, then after the final two-owner extension. It must not be used to transfer tests to a different source variant. `owners.json`, `validation.json`, `final-audit.json`, `extension-continuity.json`, `integration-real.json`, `clean/clean-checkout.json` and both maps hold the exact build and per-owner evidence. See `NEXT.md` and `next-attachment-review/NOTES.md` for the next bounded source target and its reset-order contract.
