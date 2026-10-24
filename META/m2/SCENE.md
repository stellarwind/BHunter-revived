# M2 bounded scene-data reader block

Work block 2026-10-23. Scope and roles are in `scene/review.json`; eight source data adapters are compiled, linked and exercised. Their two larger original owning behaviors remain WIP. All original bodies were reviewed before implementing reader adapters.

The LEV loader `004100d0` mixes binary decoding with block-name lookup, entity allocation, transforms, callback installation and entity/world initialization. The navigation function `00407f30` builds a graph, classifies/reorders nodes, calls recursive corridor analysis `00407d90`, then either reads a matching PTH cache or computes routes. Those whole functions must remain WIP when only their binary reader portions are reconstructed. The recursive graph helper is not an independent admitted implementation.

This block admits explicit owning data adapters for LEV/PTH through the existing source VFILE module. It does not fabricate the block registry, entities, callbacks or navigation graph. CRT allocation is reused. The original-code mapper is unchanged and excluded from this work. Numerical field bits and file order must be retained; no route symmetry/zero-diagonal normalization is justified by the real data.

## Binary format and boundary evidence

`004100d0..0041058f` opens the level through VFILE, reads a signed/nonnegative 32-bit count, then count placements. Each is a 16-byte name, three signed 32-bit coordinates and a 32-bit flags word (32 bytes total). The original uses only the low two flag bits for initial orientation but retains the complete word. After placements it reads until file end: a 32-bit placement ordinal and eight 16-bit payload values (20 bytes total). Those values are opaque per-block data, not necessarily placement/node indices. Real values include -50 and 9999; bounding all eight as indices would reject valid data. File order and repeated owner records are retained; later consumers can apply the original overwrite order after name/entity recovery.

`00407f30..004082c5` first constructs/reorders a graph using entity neighbors. Its PTH read at `00408178..004081b7` compares the file's 32-bit count to the computed graph count, then reads count squared 8-byte cells. Each cell is a signed local exit slot and float32 distance. Direct setup at `0040812f..00408151` uses `cells[destination * count + source]`. Six neighbor slots establish valid exit values 0..5, with initial -1 meaning no assigned exit. Preserve bits and orientation; do not transpose into a conventional source-major graph representation.

The original function's no-cache/count-mismatch branch computes routes, and `00407d90..00407f25` recursively traverses corridors and accumulates distance. Those behaviors are excluded from the admitted reader subset and remain dependencies for full navigation recovery. No recursive helper is counted as a completed independent function. Full LEV dependencies on block registry, entity allocation, transforms and callbacks likewise remain unrecovered.

The inspected formats have no identified magic/version marker. Source requires exact structural shape, bounded names/counts and an explicit matching PTH count. This diagnoses structural incompatibility; it cannot prove version compatibility for a different format with the same shape. Source tests do not execute any original function. `python tools/check_scene_evidence.py` independently inspects the preserved archive with Python struct parsing, verifies captured payload SHA256/CRC32 and complete mapped boundaries, and reports real-data exceptions to textbook graph assumptions.

## Source API and ownership

`src/bh_scene.h` defines compile-checked x86 little-endian records: placement 32 bytes, link record 20, route cell 8 with distance at offset 4. Coordinates and all flags remain raw values; names are not canonicalized, unknown block names are retained and no float placement conversion or entities are invented. Route float bits are copied exactly; quiet/signaling NaNs and infinities are rejected by bit inspection without evaluating them through the FPU.

Initialize a `BHSceneLevel` or `BHSceneRoutes` once with the real CRT allocator or a paired caller allocator. Memory decode validates the entire input before allocation, constructs a candidate, then replaces previous output only on success. Failure preserves old pointers/counts/payloads and records an explicit error. Disposal frees each owned allocation, clears pointers/counts and retains the initialized allocator for reuse. Caller-private ownership fields/allocator lifetime must stay intact; input buffers must not overlap owned output storage.

The VFILE wrappers borrow an open handle, read its remaining payload and never close it. On success the cursor is at end; rejection attempts to restore the initial cursor, reporting I/O failure if restoration fails. Before measuring length, the wrapper re-selects the handle's stored cursor with a zero relative seek. This matters because archive handles share a backing FILE and original tell observes that FILE's cursor. Tests open both scene assets before reading either to expose this case. Cache/shared handle use must remain serialized. Position probes clear EOF through the VFILE seek contract; full FILE/EOF-state preservation is not promised.

Limits are explicit safety policy: 65,535 placements, 65,535 link records, 2,048 route nodes and 64 MiB per input. Products are bounded before allocation/copy. LEV rejects negative counts, partial records, unterminated 16-byte names and out-of-range link-owner ordinals. Duplicate owners and arbitrary signed 16-bit link payloads are retained. PTH requires exactly `4 + count*count*8` bytes, local exits -1..5, finite nonnegative distances (negative zero is retained) and equality with the supplied expected node count. Allocation/I/O/truncation/format/limit/count-mismatch errors are separate.

The original can continue from short reads or trust malformed references. Source bounds and rollback deliberately replace those unsafe cases. A mismatching PTH count is reported; there is no silent success or fake recomputation. The real-data test supplies captured expected count 283. It does not establish that a recovered navigation graph generates 283 nodes; that remains future integration work.

## Executed scenarios

The normal source-only checkpoint includes 566 scene-reader checks. Independent synthetic fixtures cover signed coordinate extremes, high flags, unknown names, signed/large opaque link values, duplicate owner order, negative zero, the original unreachable-distance sentinel, asymmetric distances and nonzero diagonal values. No mathematical graph normalization is applied.

Every invalid short prefix of the synthetic level/cache is placed against a protected no-access memory page and rejected without an overread. Additional cases cover malformed names/owners/slots, negative counts, NaNs/infinities/negative distances, trailing cache bytes, graph-count mismatch, exact count caps and over-limit inputs. Maximum valid cases include 65,535 placements, 65,535 links and a 2,048-squared route matrix. Guarded real-memory allocators fail each candidate/temp allocation and verify balanced releases and rollback. Repeated reload/dispose, a nonzero borrowed cursor and handle ownership are exercised.

Real archive data:

| Asset | Bytes | Decoded payload | SHA256 |
|---|---:|---|---|
| TEST.LEV | 196,944 | 5,815 placements and 543 link records | `1da23c4537d874b15a40093038d2fec9bd8da53b83260efed5b4469c0bcf7df7` |
| TEST.PTH | 640,716 | 283 nodes / 80,089 route cells | `66f67e77a888834669649afba898b8d2d71bcc0b7029d3d765b0e30eb1efe5a9` |

All decoded fields/order/float bits match independent raw directory/payload reads through three source backends: embedded, preloaded and loose. Complete payload CRC32 controls additionally pin the data. The table has 69 asymmetric distance pairs and three nonzero diagonal distances; these are preserved, not repaired. Source-only decoding produces data, not a constructed world or navigable gameplay graph.

`build.cmd --archive GAME_DATA/Bhunter.bin` compiles all thirteen real translation units, links `build/m0/bhunter-integration.exe` and passes **311,212 combined checks**, of which **259,927** belong to the scene-reader scenario with its three real backends. Existing math/config/VFILE checks and all 1,229 archive payload comparisons still pass. Removing the scene module fails with LNK2001, as do the four existing missing-module controls. Commands, source hashes, resolved symbols and results are preserved in `scene/integration-real.json` and `scene/link-map.txt`.

`scene-clean/clean-checkout.json` retains the clean source-only run from a directory with spaces and a different launch cwd, with no original game files/compiler binaries inside the checkout: **32,202 checks** pass. `scene/validation.json` compares its source hashes to the real-data build. The math comparison C++ artifact, flags/header context and original-code mapper are unchanged. All original EXE/BIN/MDF/MDS hashes remain pinned and verified.

## Status and remaining gate

The eight new data/lifecycle interfaces are support adapters in `META/function-status.json`. Original `004100d0` and `00407f30` have explicitly partial WIP records linked to this harness; neither is counted as a completed functional original routine. `scene/review.json` supplements the historical M1 catalog. Disassembly coverage does not change those classifications.

Full M2 stays open. The next scene dependency is the block-definition/name registry and its bounded source data path, followed by actual entity/container and navigation graph ownership. Image/BSP/texture consumers and source-game lifetime integration remain unrecovered. Original reference scenarios/runtime precision transitions, CD audio and DirectPlay requirements remain tracked. No new original-backed adapter, renderer/audio/network deletion or original file modification is part of this block.
