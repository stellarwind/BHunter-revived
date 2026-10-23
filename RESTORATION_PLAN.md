# BHunter restoration plan

**Goal:** a faithfully playable, source-built BHunter on modern Windows, with all verified original features covered and the original executable retained only as an optional test oracle. Full decompilation means recovering the game's and required proprietary engine's behavior in maintainable source, reusing identified libraries rather than reimplementing their internals. It does not mean reproducing Microsoft's CRT source or matching every instruction.

Created 2026-10-23. This is the project execution contract. Milestones complete only when their exit evidence is recorded in `META/PROGRESS.md`; estimates, compiled files and disassembly coverage do not count as completion.

## 1. What we know, and what is still provisional

| Evidence | Current position |
|---|---|
| Original reference | English `GAME_DATA/BHUNTER.EXE`, SHA256 `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`; original MDF/MDS retained |
| Runtime | Strong VC5 SP3 single-threaded LIBC matches, including full startup; 147 Ghidra functions tagged verified CRT |
| Working math configuration | SP3 C++ frontend/backend, `/TP /O2 /G5 /ML /Gy`, real SP3 `math.h`; four routines reproduce 335 raw instruction bytes in two clean combined builds |
| Limits of that configuration | A useful matching baseline for this block, not unique historical flags for every object. Direction transform depends on preserved translation-unit context |
| Tool versions | CL driver 11.00.7022; C frontend 11.00.7113; C++ frontend and optimizer backend 11.00.7303 |
| Linker | SP3 LINK currently fails DLL entry-point resolution on this host. The native differential harness uses working stock VC5 LINK 5.00.7022. Record this split; do not claim an end-to-end original toolchain reproduction |
| Executable map | 887 function records, including 243 inferred callback candidates; 98.7% of decoded instruction bytes covered by current bodies. Neither number measures source completion |
| Asset archive | All 1,229 `Bhunter.bin` directory entries validated. File payload formats are only partially understood |
| APIs | DirectDraw2/Direct3D2, DirectInput 0x0500, DirectSound, MCI CD audio and active DirectPlay3A references |
| Existing source | Ten math reconstructions link into a differential harness. Four separate matching C++ implementations compile into a verified object. Neither is an integrated playable restoration |
| Floating point | Moderate finite fixtures match at 53/64-bit x87 precision; five reconstructed routines have 24-bit rounding differences. Actual runtime precision transitions remain unverified |
| Third-party code | No QvLib or other reusable open-source engine library established in BHunter. Math, image loading and VFILE provenance remain open |

Supporting evidence: `analysis/RESEARCH.md`, `analysis/math/RECOVERY.md`, `analysis/astra-review/FIXED_SETUP_FINDINGS.md`, `analysis/function-map.tsv`, and `analysis/subsystems.tsv`.

## 2. Rules that prevent the Squeezils failures

### Every admitted change must link

Before accepting a function or module, compile it in its real translation unit, link it into a named executable target with its real dependencies, and run a test that exercises it. Start with a linked integration harness, then the source game target. Preserve the actual source/header arrangement where compiler context matters.

An isolated decompilation or a matching COFF object is a **candidate**, not an admitted implementation. No undefined-symbol allowances, empty game functions, constant-success returns, guessed globals, placeholder vtables or inline assembly may be introduced just to make a link succeed. Real test doubles belong only in explicitly isolated tests and must never enter the game target or count as recovered behavior.

Each work block updates its dependency list before coding: declarations, owning definitions, globals, calling conventions, layouts, callbacks and lifetimes. A missing dependency is assigned or recorded as a blocker, not silently invented. Do not build a pile of disconnected `.cpp` files and postpone integration.

### Classify before decompiling

Every assigned address needs a reviewed boundary and a role: game, proprietary/reused-engine candidate, verified library/CRT, compiler support, import/thunk, shared code, data, or unknown. Record evidence and confidence; address range alone is insufficient.

Identify exception/unwind machinery, cleanup/destructor thunks, stack probes, security/runtime helpers, shared epilogues, import thunks, jump tables and callbacks before assigning gameplay recovery. Verify callers, flow, metadata and library signatures. Preserve necessary exception/destructor behavior through source language constructs and runtime dependencies. Do not hand-transcribe compiler scaffolding into assembly and call it recovered gameplay.

Survey library provenance early, then revisit specific unknown regions when new evidence appears. Match code, layouts, signatures, constants and call patterns; a familiar string or absence of strings is not enough. Reuse a verified available library at the correct ABI/version where practical, with provenance recorded. Unknown engine code that the game needs still has to be covered; unknown attribution is not permission to delete it or an indefinite block on reconstruction.

### Functional fidelity is the default objective

Byte matches are valuable evidence, not the primary completion target. A correct, integrated, tested function can be admitted as `func`. Differences limited to register allocation, scheduling or layout do not justify endless matching work once behavior and ABI have adequate evidence.

Optional matching work needs a specific question, positive control, bounded hypotheses and a stopping condition. Default budget: one 30-minute investigation per function/family after a functional implementation exists, or stop sooner when outputs repeat without new information. A user-requested matching investigation can explicitly extend that budget. Record unresolved differences and move to a useful dependency or checkpoint.

No blanket flag changes to rescue a single function. Deduplicate emitted code. Preserve tool/header/source hashes, full commands, original function bounds and actual relocation masks. Never mask arbitrary differing bytes or count a prefix as a complete match.

## 3. Progress framework

Keep four independent fields. They are not one maturity ladder.

| Field | Values and meaning |
|---|---|
| Role/provenance | `game`, `engine_candidate`, `library_verified`, `compiler_support`, `import_thunk`, `shared_tail`, `data`, `unknown`; evidence and confidence required |
| Implementation | `not_started`, `wip`, `func`, `bm`, `reused`, `adapter` |
| Integration | `not_built`, `object_only`, `linked_harness`, `linked_game`, `scenario_exercised`, `release_admitted`; name the target/build |
| Validation | Separate evidence for ABI/layout, fixtures, differential behavior, scenario tests, raw-byte match or relocation-normalized match; explicitly state limits |

Implementation shorthand:

- **`not_started`:** no recovered implementation. A decompiler listing alone does not change this.
- **`wip`:** candidate source or partially specified behavior; do not count as finished.
- **`func`:** reconstructed behavior/ABI has passed its stated tests, with gaps documented. Admission still requires a linked target.
- **`bm`:** complete function body reproduced; record raw versus relocation-normalized comparison and source context. It can still be unintegrated.
- **`reused`:** identified library/runtime dependency reused with version, ABI and provenance checked; excluded from game-code recovery percentages.
- **`adapter`:** deliberate platform replacement with an explicit behavior contract, not a claim of original source recovery.

Use one record per implementation variant where both functional and matching sources exist. Do not combine the matching C++ artifact's `bm` evidence with the different C implementation's harness-link evidence. The ledger starts at `META/function-status.json`; expand it as work is admitted.

Report milestone results, source-only scenario coverage, linked/admitted functions by role, unresolved dependencies, and original-backed dependencies. Never headline an undifferentiated percentage of all 887 candidates. Keep compiler support and known library code out of the gameplay denominator; revise totals when boundaries change.

## 4. Build and execution architecture

Maintain three distinct targets/profiles:

1. **Historical comparison:** pinned VC5 tools, headers and translation-unit context; compile/match focused code, with clean positive controls. It is an analysis facility, not the modern shipping requirement.
2. **Continuously linked integration:** initially an executable harness of real recovered modules, evolving into `bhunter-integration.exe`. Build every accepted source file, resolve all definitions, exercise the active checkpoint, and audit any remaining original-backed adapters explicitly.
3. **Modern source-only game:** initially a 32-bit Windows build that can run on modern x64 Windows. Select and pin a supported compiler and backend after compatibility experiments. Compile source modules, verified dependencies and real platform adapters. A 64-bit port and additional operating systems are later goals, not prerequisites for the first faithful release.

Do not copy `/O2 /G5` blindly into a modern compiler configuration. Preserve struct layout, calling conventions and numerical contracts while validating modern code generation separately. Keep byte-matching variants in their own target; functional equivalents are not failed work.

The original is an oracle, not an implicit substitute for unrecovered code. Prefer selected pure-function differential calls and captured scenarios. The current math private-image mapper is **not a general game loader**. Broader original-backed integration, if needed, requires a separately reviewed loader/adapter design covering imports, initialization, global state, callbacks, exception behavior and ownership. Such a diagnostic path must be opt-in, report what remains binary-backed and be absent from the release target.

Final acceptance requires the game to build, launch and play without loading, mapping or executing original game code. Tests may still use a separately supplied original EXE. System libraries and verified redistributable dependencies are normal dependencies, not unrecovered game code.

## 5. Milestones and exit gates

Use the milestones as dependency gates, with small functional families chosen from the next observable checkpoint. Adjacent research can proceed when it removes a concrete blocker; broad decompilation does not replace the active gate. In particular, start M3 reference bring-up during M1/M2 when needed: measuring the original runtime's x87 state may be necessary to satisfy M2's numerical gate. Do not wait for M2 to finish before obtaining that evidence.

### M0 — Preserve the reference and make linking routine

Deliver a hash manifest for EXE, BIN, MDF/MDS, toolchain, relevant SDK headers/libraries and test fixtures. Establish version control with generated builds/large experiments ignored and required evidence retained. Do not rewrite the historical experiment records to appear cleaner.

Create one reproducible entry command for compile → link → run of the integration harness. Verify the declared compiler/linker combination; isolate the SP3 LINK failure as a tracked issue rather than blocking useful source integration. Preserve the clean four-function match verifier as an optional check. Capture a clean dependency/link map and a machine-readable test result.

**Exit:** a clean checkout plus documented local prerequisites builds and runs a linked harness; missing definitions fail loudly. Original hashes remain unchanged. No original game code is needed by the normal harness; its separate differential mode may require the reference EXE.

### M1 — Review the source map and eliminate false recovery targets

Promote the current observed function map into a role/provenance catalog. Review reachable startup, math, VFILE, assets, renderer, simulation, audio, input and networking boundaries. Check candidate callbacks and shared tails. Inventory globals, structures, vtables, jump tables and ABI contracts alongside functions.

Identify CRT and compiler-generated helpers before assigning recovery. Record library-survey outcomes, available source candidates and evidence for accepting/rejecting them. Build a dependency graph for the first playable slice. Unknown regions retain explicit ownership and hypotheses.

**Exit:** every address assigned to the first slice has a reviewed role/boundary and dependency owner. Known runtime/unwind scaffolding is excluded from gameplay assignments. No requirement to pretend the entire binary's provenance is already certain.

### M2 — Admit foundations into the linked build

Integrate one chosen implementation of each math function with shared, checked vector/matrix types; keep the alternative matching artifact separately reproducible. Resolve the 24-bit x87 discrepancies or explicitly constrain and verify the intended numerical environment before gameplay admission. Extend tests beyond the current moderate dyadic fixtures to zero, aliasing contracts, varied magnitudes and relevant exceptional cases.

Recover allocation/container contracts, archive/VFILE access, path/config handling and bounded asset readers required by the slice. Reuse CRT rather than decompiling it. Validate all archive entries and reject malformed/version-mismatched inputs without out-of-bounds access. Add real owning definitions for globals and lifecycle functions as each module enters the target.

**Exit:** foundations compile, link and execute together in the integration target; readers load representative real assets and diagnose invalid data. No unresolved or fabricated definitions. Functional variants can pass without byte matches.

### M3 — Establish a playable original reference and capture behavior

Diagnose portable source-path discovery against the documented `SourceDir` and copyright checksum behavior. Recover needed installation markers from the preserved disc image as source material. Keep any changes to an original reference launch isolated and reproducible; do not silently overwrite the oracle.

Inventory actual original features, modes, controls, missions, save/config formats, rendering options, sound, music and networking. Inspect MDF/MDS tracks for CD audio and identify track numbering, loop/control behavior and availability. Capture deterministic input/seed/time traces where possible, plus observable startup, menu, loading, movement, collisions, objectives, failure/retry and shutdown scenarios. Screenshots alone are insufficient for simulation equivalence.

**Exit:** a documented runnable reference environment and scenario set exist. If a hardware/driver/media limitation prevents a scenario, record the blocker and the evidence still missing. Do not invent expected behavior or mark it validated.

### M4 — First source-driven playable slice

Recover the narrow path from startup through archive/level loading to one actual playable scenario and orderly shutdown. Implement required entity ownership, scripting, simulation/update order, collision, camera, controls, rendering submission and audible feedback with real dependencies. Start with a smaller source-rendered scene as a diagnostic checkpoint if useful, but do not call that the playable-slice exit.

Choose a minimal faithful rendering/input/audio path that can support this checkpoint. Existing API compatibility layers can be evaluated as explicit platform dependencies; broad backend redesign waits until the slice works. A runtime tracer or binary-backed gameplay session is reference evidence, not completion of this milestone.

**Exit:** the source-built executable starts, loads real game data, accepts controls, performs the scenario's gameplay/objective behavior and exits cleanly. The exercised scenario executes recovered source, with any unexercised or optional binary-backed diagnostic paths listed separately. Repeat launch/load/shutdown without lifecycle failures.

### M5 — Recover the complete single-player game

Extend by coherent dependency families: script commands, world streaming/activation, entities and AI, player movement, weapons/damage, collision, mission/objective logic, menus/HUD, audio events, persistence and configuration. Confirm actual original features before naming implementation tasks; do not infer features merely from a generic label.

For each family, link immediately and rerun the slice plus relevant reference scenarios. Exercise all supplied playable content and significant branches, including retries, transitions, long sessions and repeated loads. Track both visible gameplay differences and hidden state/timing differences. Preserve original update order and random-number behavior where observable.

**Exit:** all inventoried single-player content and features work from source with a documented scenario matrix. No silently unsupported script opcodes or missing behaviors; any remaining original-backed code is an explicit blocker for this gate. This can be a preview release while multiplayer work remains, not the final full-restoration declaration.

### M6 — Make the platform layer reliable on modern Windows

Consolidate window/display, rendering, input, sound/music, timers, threading, files and settings behind narrow interfaces. Compare a bounded set of backend approaches against the actual Direct3D2/DirectDraw requirements; record the choice and its fidelity costs before a broad rewrite. Replace platform-dependent code one interface at a time while the playable checkpoints continue to run.

Provide portable data discovery and per-user writable configuration/saves, launch from arbitrary working directories, robust focus/window transitions, aspect-correct presentation, correct pointer mapping, and appropriate modern input handling. Verify actual supported hardware/render modes rather than promising every original driver path. Preserve simulation timing independently of presentation; high refresh rates do not automatically change game speed.

Replace CD drive/MCI assumptions with a verified local-media path that preserves track selection, timing, looping, pause and volume behavior. Audio outside `Bhunter.bin` is part of preservation. Document the user's asset/media import process. Shipping should not require a physical disc, machine-wide installation markers, administrator privileges, or legacy driver installation.

Treat x87 precision, rounding, float conversions, matrix convention, depth/blend/texture behavior, threading and timer order as fidelity contracts. Do not enable faster math or redesign the simulation to get a smoother frame counter before those contracts pass.

**Exit:** source game and its verified media run on the declared modern Windows test matrix; input, sound, display, timing and persistence scenarios pass. Proposed baseline matrix: Windows 10 and 11 x64, common integrated/discrete GPU classes and high-DPI/high-refresh displays. Record which combinations were actually tested rather than asserting universal compatibility.

### M7 — Restore verified networking features

Trace the active DirectPlay3A paths to establish actual host/join/discovery, transport, message ordering, state ownership, player-count, reconnect and synchronization behavior. Preserve protocol/layout facts before choosing a replacement network adapter. Do not assume obsolete transports can simply be removed without recording the lost behavior.

Select interoperability requirements from evidence: source-build peers first, original-peer compatibility where feasible and testable. A compatibility claim requires a real peer test. If an original transport cannot reasonably be retained, make the deviation explicit and obtain a scope decision rather than silently deleting multiplayer.

**Exit:** all verified in-scope multiplayer modes run between independent modern-machine peers, with join/leave, session lifecycle and representative gameplay/synchronization tests. Disabled networking is not full completion. Any deliberate feature reduction requires the user's explicit scope change.

### M8 — Source-only audit and regression closure

Audit the full reachable game dependency graph, data tables, callbacks, exception/lifetime behavior and global initialization. Every required proprietary function has recovered source or a justified tested replacement; identified runtime/third-party components have verified dependency records. Reachable unknown behavior is a blocker even if the current playthrough did not hit it.

Remove all diagnostic original-game-code adapters from the shipping target. Build and run with the original EXE absent. Audit link maps and loaded modules; test a package containing only the source-built game, declared runtime dependencies and user-supplied assets. Keep reference tools outside that package.

Rerun the complete content/feature matrix, media checks, persistence/version compatibility, timing, long-session and multiplayer tests. Review known deviations as accepted limitations or release blockers; tolerate documented instruction differences, not missing behavior.

**Exit:** the game is fully source-built and feature-complete for the agreed scope, the release target has no original-game-code execution dependency, and outstanding fidelity issues have explicit dispositions.

### M9 — Release and preservation handoff

Produce a clean distributable with one documented asset/media import path and straightforward launch. Include build instructions, pinned dependency provenance, source and type/format documentation, reference hashes, accepted differences, supported/tested platforms and regression scenarios. Keep game assets and original EXE out of a source release unless separately supplied as part of the user's local installation.

Verify installation, launch, gameplay, media, configuration and uninstall/move behavior in a clean location with spaces in its path and no developer checkout/toolchain present. Reproduce the release build from a clean checkout, recording the compiler and dependencies used.

**Exit:** another person can build the project or install the release, supply their game data, and play the verified game without the original EXE or development environment.

## 6. Per-family work checklist

1. Pick a family on the next milestone's dependency frontier; name the observable checkpoint it unlocks.
2. Review provenance, boundaries, callers, ABI, globals, side effects and library/compiler-support exclusions.
3. State the behavior contract and tests, including aliasing, lifetime, failure and numerical constraints where relevant.
4. Implement in the owning source module and update shared declarations once. Avoid duplicate competing type/global definitions.
5. Compile the real translation unit, link the full current target, and run the focused check plus impacted checkpoint. Preserve source/header context for matching variants.
6. Update the ledger, dependency/blocker list and concise milestone evidence. Leave unfinished work marked unfinished.
7. Attempt byte matching only if it answers a useful question within its budget. If it plateaus, retain tested `func` source and the actual diff; no asm fallback or declaration clutter enters production automatically.

When agents are explicitly requested, each receives a narrow family, shared ABI contract, owned files and named link target. One owner integrates shared headers. The receiving owner must compile, link and run the result before accepting it. Returning many source files is not successful integration.

## 7. Padscan policy and compiler fit

The Squeezils `padscan.sh` is a **declaration-context scanner**, not a function-boundary scanner. It adds prototype/forward/extern/class-shape declarations at selected source locations to investigate compiler allocation/context sensitivity.

The original is preserved in `tools/upstream/squeezil/padscan.sh`. Its missing `match.sh` dependency and in-place mutation made it unsuitable to run unchanged. The reviewed adaptation `tools/padscan.py` uses the fixed VC5 SP3 C++ configuration, an EXE-hash-checked target manifest, stable snapshot source paths, explicit compiler-error handling, deduplicated outputs and no automatic installation of candidates.

Compatibility check: the four-function 335-byte baseline remains exact in a copied source context. Small declaration variations alter some generated sequences under VC5, confirming that the diagnostic is relevant. Evidence is in `analysis/padscan/`; usage and restrictions are in `tools/PADSCAN.md`.

Use it after real declarations/header/source context have been recovered and a function is already understood. Scan one bounded question with fixed flags and an exact positive control. Synthetic padding can demonstrate a compiler-context explanation; it does not recover historical declarations or establish functional fidelity. Do not promote a padded match into the main source unless its context is understood and integration/behavior are verified. A `func` implementation never needs a padscan win to ship.

## 8. First execution block after this plan

**Start M0, not another math sweep:** establish version control and a reproducible named compile/link/run target; record the compiler/linker split and preserve the matching verifier. Then enter M1 on the dependency closure of startup → VFILE/archive → one scene. Identify compiler helpers and provenance before assigning new decompilation families.

This plan does not claim that those targets, the original playable reference, backend selection or a restored game already exist. Current evidence and the first open gates are recorded in `META/PROGRESS.md`.
