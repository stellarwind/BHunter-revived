# M2 block registry and BSP data work block

Work block completed 2026-10-24. Review before implementation is retained in `blocks/review.json`, with complete loader/script bodies in `blocks/original/` and topology consumers in `blocks/consumers/`.

The block registry loads a name list, invokes script/entity behavior, rendering objects and collision objects. This pass recovers the required owned data path rather than inventing image/entity success stubs. All 505 BSPs have six nonempty counted sections: material32, vertex60, edge16, face12, node56 and leaf-link8. Complete original registry/render/collision functions remain WIP until their remaining behaviors are integrated. Script text is retained explicitly as unapplied, not treated as executed.

## Geometry format, role and provenance

BSP here denotes binary space partitioning, but the technique and filename suffix do not establish engine lineage or in-house authorship. `engine_candidate` means unresolved ownership. The reviewed data is not the Quake BSP layout. No reusable third-party engine source has been identified; that is not proof that none was used.

The six sections are each a 32-bit count followed by fixed-size records:

| Section | Bytes/record | Reviewed fields |
|---|---:|---|
| Materials | 32 | nine opaque attribute bytes and 23-byte texture name |
| Vertices | 60 | three float32 positions; remaining 48 bytes retained opaque |
| Edges | 16 | vertex index, next-edge index, float32 u/v |
| Faces | 12 | material index, first-edge index, opaque runtime word |
| Partition nodes | 56 | AABB min/max, plane normal/distance, front/back leaf and node indices |
| Leaf links | 8 | face index and next-leaf index |

`00457e20` reads these sections for rendering, resolves used material images and derives a root radius. `0044b2e0` reads the same layout for collision, clears runtime words and allocates derived bounds/cache state. The data adapter retains original file bytes as data and derives bounds separately; it does not treat serialized runtime words as valid pointers. Consumers `0044e060` and `0044d530` establish the edge/leaf chains and partition child offsets. All direct loader bodies and those consumers are retained under `blocks/original/` / `blocks/consumers/`.

`src/bh_bsp.c` owns an exact raw storage copy, const section views and node bounds. Names, opaque fields and float input bits are unchanged. References are checked before traversal. Edge/leaf chains must terminate, and partition children must be acyclic; shared tails/children are allowed. Validation uses bounded iterative work arrays, including an explicit DFS stack, rather than risking recursion on malformed input. Input and output ownership do not overlap.

Safety policy requires six nonempty sections, at most 65,535 records each, a 16 MiB input cap, exact file length, bounded texture strings, finite known floating fields, ordered bounds and valid indices (-1 only where a sentinel is observed). All 505 real BSPs satisfy this policy. Empty sections, partial/trailing data, invalid references, cycles or overflowed derived bounds are rejected with explicit errors. No identified format version marker exists: structural compatibility is checked, not a guessed version number. The API is pinned x86/little-endian data support, not a recovered 128-byte original runtime-object ABI.

## Numerical contract and investigation

Bounds require the established PC53/nearest/masked profile; the reader returns a numerical-contract error if it is absent. Centers retain the original binary32 addition-store and half-store. Collision squared radius is not a uniformly float dot product: original x87 instructions retain dx/dy/dz at current precision, multiply dx/dz by separately stored binary32 operands, add x/y before z, then store binary32 before SQRT reloads it. The accepted expression is `(dx * binary32(dx) + dy * dy) + dz * binary32(dz)`, followed by the observed stores.

`blocks/bounds-investigation.md` records the question, analytical positive control, first counterexamples, candidate failures, exact instruction-width interpretation and stopping condition. A naive mathematical fixture was retained as an explicitly rejected candidate, rather than silently relabeled original evidence. Pinned VC5 reused extended values across volatile local stores/reads; `blocks/volatile-diagnostic-listing.txt` preserves relevant actual output for that candidate only.

The real `void bh_round_float(float *, double)` in its own translation unit provides the needed binary32 memory boundary and no floating return. This prevents cross-expression value reuse in the pinned non-LTO build without changing flags, writing inline assembly or touching the historical math C++ context. A missing-definition link control proves the helper is actually required. All final derived bounds/root words agree with the independent instruction-derived Python controls over all 505 real assets and three backends. This is coverage of the stated adapter contract, not original-code execution, a raw-byte match or a universal FP proof.

## Registry, lookup and retained scripts

`BHBlockLibrary` owns original-order block names, each loaded BSP and optional raw script bytes. The list uses blank lines, column-zero semicolon comments and case-insensitive `end`; name matching takes the first case-insensitive match, consistent with the original level lookup. Duplicate names/order are retained. Source policy bounds names to 15 visible characters plus terminator, `.BSP` extension, 600 entries, 511-byte lines, 64 KiB text/sidecars and 64 MiB aggregate stored payload. It rejects unsafe separators and embedded NULs rather than silently trimming/reinterpreting input. Registry names-only decoding leaves models unloaded explicitly.

`bh_bsp_load` implements the four reviewed path policies: prefixed-first then bare, bare-first then prefixed, prefix-only and bare-only. Prefix is explicit and includes its separator. The library resolves required BSP data transactionally; a missing/malformed required model rejects the candidate and retains the previous library. Optional sidecars use the prefixed basename with `.TXT` and are kept byte-for-byte with `script_present` and `unapplied_scripts` fields. VFILE's existing not-found policy governs absent optional files; the library does not claim to distinguish every underlying OS open failure from absence.

There are 454 actual block names before `end`, 454 loadable models and 202 associated sidecars. Every one of the 5,815 TEST.LEV placement names resolves to that loaded data registry. This establishes data/name closure, not successful script execution or world construction. The original registry invokes script/entity behavior, rendering images, collision callbacks and a water-image adjustment; those remain unrecovered. No empty callbacks, fabricated image handles or constant-success model substitutes were added.

## Executed validation

- Source-only `bsp-block-data`: **786 checks**, including exact analytical 6-8-10 bounds, raw/opaque retention, every short fixture prefix at a protected memory boundary, malformed strings/counts/indices/floats, three cycle classes, every candidate/blob/nested library allocation failure, balanced guarded ownership, repeated reloads and borrowed-cursor restoration.
- All six section caps are exercised together at 65,535 records each, including a 65,535-node traversal and a rejected deep back-edge cycle. Registry 600/601 counts, line/text limits, duplicate lookup, explicit missing required models, raw unapplied scripts, four search modes and both fallback orders are exercised.
- All **505 BSPs (3,167,776 bytes)** decode through embedded/preloaded/loose VFILE backends: **1,515 decodes**. Every raw section byte, node-bound word and root-radius word matches independent controls. The real block list and all retained scripts match captured order/CRCs, and all 5,815 placement names resolve.
- `python tools/check_blocks_evidence.py` independently regenerates expected control content in memory and compares it with the captured JSON/headers. The deliberate `--write-controls` option is the only regeneration path. The checker verifies the pinned BIN hash and does not execute original code. Raw control data/CRCs do not substitute for the numerical instruction review above.
- `build.cmd --archive GAME_DATA/Bhunter.bin`: all **17 real TUs** compile, link and pass **337,313 combined checks**, including the unchanged math/config/scene and all 1,229 archive payload checks. The BSP/block scenario contributes 26,101 checks with real data. Eight omitted-module controls fail with LNK2001 (math, VFILE, config, paths, scene, BSP, blocks, round-store).
- `blocks-clean/clean-checkout.json`: source-only checkout with spaces, different launch cwd, no original files/compiler binaries inside it, **32,988 checks** pass. `blocks/validation.json` compares exact source hashes with the real-data build. `blocks/integration-real.json` and `blocks/link-map.txt` retain the commands, resolved ownership and target results. All original EXE/BIN/MDF/MDS hashes are unchanged.

## Admission and remaining M2 work

Eleven new source support interfaces are admitted to the harness as adapters. Four original owner records (`00411f00`, `00457e20`, `00458140`, `0044b2e0`) remain explicitly WIP; data subsets do not count as finished original functions. The script parser `004111e0` is reviewed but not implemented. Historical M1 evidence and math matching variants remain separate.

The M2 objective remains active. The next necessary review is the actual entity allocation/container family used by the registry and scene, rather than substituting new test-only entity state. Script/runtime behavior, texture image consumers, full world/navigation integration and the M2 exit audit are still outstanding. Original runtime precision scenarios, CD music and DirectPlay remain tracked. No renderer/audio/network feature was deleted and no original-code mapper was expanded.
