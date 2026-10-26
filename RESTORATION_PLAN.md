# BHunter restoration plan

**Goal:** a faithfully playable, source-built BHunter on modern Windows, with all verified original features covered and the original executable retained only as an optional test oracle. Full decompilation means recovering the game's and required proprietary engine's behavior in maintainable source, reusing identified libraries rather than reimplementing their internals. It does not mean reproducing Microsoft's CRT source or matching every instruction.

Created 2026-10-23. This is the project execution contract. Milestones complete only when their exit evidence is recorded in `META/PROGRESS.md`; estimates, compiled files and disassembly coverage do not count as completion.

**Execution decision (user directive, 2026-10-24): skip M3 and proceed directly to M4 decompilation and source restoration.** M3 is skipped, not passed; its existing evidence is retained and its missing observations remain explicitly unvalidated. M3 completion is no longer a prerequisite for source recovery. All automatic-play plans are abandoned: no menu, tutorial, flight, combat or network gameplay automation, and no further pilot development. Any game execution is limited to a bounded startup test. User resumed implementation on 2026-10-24; M4 source restoration is active.

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
| Existing source | 443/727 admitted original functions in a409-TU source harness: math/FP, data, world/navigation/collision, effects, sound/packet, texture/model, camera/culling/clipping, lighting and native rendering; complete frame/window/lifecycle owners and native WAV loading; whole loading-image initialization, radial progress display and actual archive callback; script numeric readers and complete RC rail data construction/linking; model scale, reflected edge winding and actual draw-cache invalidation; native device enumeration without acquisition; complete rail-wagon draw/movement/coupling/lifetime behavior, wait-for query handlers and distinct basic pickup construction; menu settings/input assignment, script cleanup/delayed mission sounds, serial/modem preferences/address callback and closed entity/numeric/packet helpers; attached script sound and target movement/rotation, DirectDraw startup inventory/selection callbacks and editor input setup/live packet dispatch; particle/emitter/sensor and flash/projection owners, procedural collision-box creation, native DirectSound/CD/provider startup with explicit hardware-success limits; complete flare/target drawing, UTF16 atlas and queued text, force-feedback startup, effect/debris behavior, projectile lifetime query/cleanup and menu row construction/refresh. A native Direct3D2 target renders a static real-level shell through recovered traversal and queue owners. Four matching C++ variants remain object-only. No playable source game yet |
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

Current sequence: M0/M1 initial frontier/M2 foundations → M4 source restoration → remaining source milestones. M3 is explicitly skipped by user direction. Choose small implementation families from the source-built checkpoint's dependency frontier. Resolve missing contracts through focused static review and source-built checks; retain unresolved fidelity questions as blockers. Do not reopen original gameplay capture as a prerequisite. All automated game execution is subject to the startup-only limit throughout this plan, including later milestone validation. Gameplay acceptance requirements remain unverified until evidence can be obtained within user-authorized methods; they do not authorize automatic play.

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

### M3 — Original reference/scenario capture — SKIPPED

Skipped by explicit user direction on 2026-10-24. This is not a completed or passed milestone. Retain existing startup, asset, event, target and navigation evidence with its original scope and provenance. No more automatic-play development, menu/tutorial/flight/combat/network replays or pursuit of M3's former scenario exit gate. The saved unvalidated portal controller is abandoned diagnostic work.

Missing observations include combat hits/kill, objective completion, destruction/retry and broader timing, media and peer behavior. Carry those uncertainties into the relevant source-family contracts and later fidelity review; do not invent expected results or delete required features. Use existing evidence and static decompilation to proceed with source implementation. A bounded startup test is the maximum permitted game execution, not a routine prerequisite for each family.

**Disposition:** skipped; no exit pass claimed; no dependency blocking M4.

### M4 — First source-driven playable slice

**Next implementation phase.** Begin decompilation and source restoration now that M3 has been skipped. The first work block must select a concrete missing family on the startup → real scene dependency path, review its boundaries and dependency closure, and deliver source code in the owning module. Existing scene readers and entity containers are foundations; remaining world construction, scripts, navigation consumers and platform lifetimes must be recovered as needed. Do not replace this work with diagnostic controller development.

Recover the narrow path from startup through archive/level loading to one actual playable scenario and orderly shutdown. Implement required entity ownership, scripting, simulation/update order, collision, camera, controls, rendering submission and audible feedback with real dependencies. Start with a smaller source-rendered scene as a diagnostic checkpoint if useful, but do not call that the playable-slice exit.

Choose a minimal faithful rendering/input/audio path that can support this checkpoint. Existing API compatibility layers can be evaluated as explicit platform dependencies; broad backend redesign waits until the slice works. A runtime tracer or binary-backed gameplay session is reference evidence, not completion of this milestone.

**Exit:** the source-built executable starts, loads real game data, accepts controls, performs the scenario's gameplay/objective behavior and exits cleanly. The exercised scenario executes recovered source, with any unexercised or optional binary-backed diagnostic paths listed separately. Repeat launch/load/shutdown without lifecycle failures.

### M5 — Recover the complete single-player game

Extend by coherent dependency families: script commands, world streaming/activation, entities and AI, player movement, weapons/damage, collision, mission/objective logic, menus/HUD, audio events, persistence and configuration. Confirm actual original features before naming implementation tasks; do not infer features merely from a generic label.

For each family, link immediately and run relevant source-built unit/integration checks using real dependencies and existing captured fixtures. Do not rerun automated gameplay or reference scenarios. Track the supplied content and significant branches, including retries, transitions, long sessions and repeated loads, as acceptance requirements; mark unexercised gameplay explicitly unvalidated under the startup-only execution restriction. Preserve original update order and random-number behavior where established.

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

## 8. Next execution block — M4 decompilation and source restoration

Current checkpoint — HUD/attachment restoration, 2026-10-25: **33 complete owners added: 300/727 admitted original functions in 288 translation units**. The real integration passes **3,844,710 checks**, the asset-free clean build **3,351,626 checks**, with **138 missing-module link controls** and identical source hashes. Complete room/portal radar graph, HUD setup/markers/world/flat status, scoreboard, weapon arc/target overlays, full-image plane quads, adaptive paths, two-point segments, fade, end-frame presentation, shared weapon reset/delay, both selected weapons' lifecycle, target removal and float timers now link with real source definitions. Two corrected ribbon variants preserve duplicate-point/unordered x87 behavior with historical evidence kept separately. Real HUD/font/weapon assets render through actual offscreen DirectDraw/Direct3D2 owners. No original code, game execution, native capture or cursor recenter occurred.

Next source boundary is complete player scene callback `0041c860`; its direct rendering dependencies are now available, while player/camera/weapon construction and live resource/state binding remain explicit work. Flyby callback `00428f50`, render producer/worker `00479140`/`00471e00`, software raster dispatch, mission lifetimes and startup resource/script binding remain open. Fractional native fade has a patterned host readback; lost-surface Restore, PC24 numerical behavior, CD audio and DirectPlay retain stated validation limits. M3 is skipped; M4 is incomplete and no playable source game exists. User directed stopping at exactly 300 admitted functions; this checkpoint is stopped pending resumption. Evidence: `META/m4/hud-attachment/README.md`, `META/m4/hud-attachment/NEXT.md`; audit `python tools/check_hud_attachment.py`.

Previous input/render/actor-route checkpoint, preserved as history:

Previous checkpoint, 2026-10-25: The 90-minute input/render/actor-route block restores **33 complete owners: 267/727 admitted original functions in 249 translation units**. The real integration passes **3,839,979 checks**, the asset-free clean build **3,347,025 checks**, with **120 missing-module link controls** and identical source hashes. New owners cover horizon and quad queues/geometry, radar, complete input message/collection/device owners, target/route updates, three-buffer handoff, generic queue reservation and queued render-mode execution. Rectangle UV and live near-plane fixes retain separate historical variants. No original code or game execution, native capture or cursor recenter occurred. Both full scene callbacks `0041c860`/`00428f50`, render producer/worker `00479140`/`00471e00`, complete HUD/attachment setup, player/flyby construction, weapon/mission lifetimes and startup resource/script bindings remain open. Horizon geometry is restored; software dispatch/raster kernels remain unfinished. PC24, CD audio and DirectPlay requirements remain. M3 is skipped; M4 is incomplete and no playable source game exists. This timed block stops pending user resumption. Evidence: `META/m4/frontier-90min/README.md`; audit `python tools/check_frontier_90min.py`.

Earlier checkpoint history follows.

Current checkpoint, 2026-10-25: The timed scene-caller dependency block restores 52 complete owners: 234/727 admitted original functions in 203 translation units; 3,536,976 archive checks, 3,044,022 clean checks and 99 missing-module link controls. Every admitted variant compiles in its real translation unit, links against real definitions in build/m0/bhunter-integration.exe, and passes affected source-built checks. Real and clean source hashes agree. No original code or game execution occurred. Both scene callbacks `0041c860`/`00428f50` remain unimplemented. Full horizon/software rasterization, HUD/handoff, player/flyby constructors, physical input collection, weapon lifetimes and startup resource/script bindings remain open. Next recover a complete dependency family on that frontier; weighted target helper `00421610` is reviewed but unimplemented. PC24, CD audio and DirectPlay requirements remain. M3 is skipped; M4 is incomplete. This timed block is stopped pending user resumption. Evidence: `META/m4/scene-callers/`.


M2's reviewed foundations are complete. M3 is skipped. The latest parallel block admits 31 complete original owners, bringing the source count to 155/727. Camera, clipping/culling, light preparation/evaluation, material/render state and lit/unlit BSP traversal now compose with real native queues and Direct3D2. The static shell scene emits 1,798 polygons/3,112 triangles with 146 textures. The132-TU integration passes 3,398,563 real-data and 2,906,329 clean source checks with 66 missing-module controls; see `META/m4/parallel-hour/`. The native primitive adapter covers only the hardware branch; whole 0046e390/software rasterizers remain unfinished. Static shell filtering, camera and test-light setup are diagnostic adapters, not original world orchestration. The next model-instance block admits complete 0045aa60, bringing the total to 156/727 in 134 TUs. Its 155 focused checks and full 3,398,718 real-data / 2,906,484 clean checks pass with 67 missing-module controls. Static default/red/point images are byte-identical after replacing per-instance diagnostic orchestration with this owner. Evidence: `META/m4/model-instance/`. The following sprite/resource block admits complete 00455960, 00455f70, 004569b0 and 0045adf0: 160/727 admitted functions in 142 TUs, 3,398,887 real-data / 2,906,653 clean checks, 169 focused checks and 71 missing-module controls. Static scene pixels stay identical through the recovered recursive resource dispatcher; native sprite/ribbon output is exercised separately. Evidence: `META/m4/sprite-world/`. The 0046e430 hardware adapter remains partial/WIP. The recursive owner dispatches render resources rather than entities. The queued-resource block admits 004775f0/00477640 consumers, 00478170 producer and 004413b0 entity draw method: 164/727 source functions in 149 TUs, 3,399,044 real-data / 2,906,719 clean checks, 157 focused checks and 74 missing-module controls. A separately recorded adapter owns model/cache/instance resources and stable handles; native source entity fixtures consume both model draws and original flare image with a blank missing-binding control. Static level pixels remain identical through the queued path. Evidence: `META/m4/queued-resource/`. The entity method is payload +e8, not generic refresh. The pickup/scheduler block adds 17 complete owners: constructor 00441a00, lifetime/query/cleanup, default/label drawing, queue controls, portal clipping and actual child/entity scheduling 00409f40. A source-only native fixture constructs a pickup, schedules its real callback, renders model/glow/labels and checks an inactive-room blank control. The 00454fc0 native rectangle adapter remains WIP for software rendering. Evidence: `META/m4/pickup/`. The outer scene block admits complete 00409130: 182/727 original functions in 163 TUs, 3,399,450 real-data / 2,906,962 clean checks and 80 missing-module controls. A separate source adapter binds constructed world rings, containment sentinels, matched portals and validated model-resource handles. Real two-room native output verifies cached reuse, room change, scene gate, portal clipping, full-target resets and a world-disabled blank control. Updated scheduler variants bind the original player overlay position separately from the render eye; their older tested source bytes are retained. Evidence: `META/m4/scene-owner/`. Next trace scene callers 0041c860/00428f50 and their actual player/camera/resource initialization. Full startup construction, non-model registries, actor constructors and software rasterizers remain open. Non-model descriptor constructors/full startup registries remain unresolved; no invented tags or callbacks are installed. Atlas glyph copy and software dispatch remain explicit open dependencies. Human prologue playtesting proceeds independently. For each implementation block:

1. Inspect the current linked source target and select one missing function or small dependency family needed for source-built startup/scene construction. Name its owning source module and named integration target before coding.
2. Review original boundaries, callers, ABI/layout, globals, callbacks and lifetimes. Exclude verified CRT/compiler helpers; record uncertain engine provenance without treating it as a reason to postpone required recovery.
3. Recover maintainable source and real owning definitions. Compile the real translation unit, link the current target and execute focused source-built checks against established contracts and real asset fixtures.
4. Record the exact implementation variant, integration and validation in `META/function-status.json` and `META/PROGRESS.md`. Missing dependencies stay explicit; listings, traces and standalone objects do not count as admitted recovery.

The deliverable is integrated recovered source. No automatic pilot, gameplay replay or further M3 capture is part of this block. At most perform a bounded game startup test when relevant. No source-built playable game or M4 exit is claimed by this plan revision.
