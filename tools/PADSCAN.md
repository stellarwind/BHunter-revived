# Reviewed Squeezils padscan for BHunter

`padscan.py` adapts the declaration-context experiment in the preserved `upstream/squeezil/padscan.sh`. The original archive path was `D:/Decomp/Squeezil/__ARCHIVED/2026-10-21-msvc41-restart/tools/flagsearch/padscan.sh`; its copied SHA256 is `77788515b95d9312bda9ba5d2c996b8252b8f9ebd1dd02fbc85383e442754b6c`.

This tool does **not** discover function boundaries. It adds unused prototype, class-forward, extern-variable or class-shape declarations before a file, after its last literal include directive, or at its end, then checks the emitted code. It is relevant to our VC5 compiler because declaration/header/translation-unit context has already changed register allocation and scheduling in real matches.

## Why the archived tool needed adaptation

- It calls an adjacent `match.sh` that is absent in the archive, so its scoring/toolchain cannot be reused as-is.
- It edits live input sources and relies on a shared backup directory and shell traps to restore them.
- Shell whitespace splitting does not safely handle arbitrary source paths.
- Filtering scorer output can hide a compilation failure or missing target.
- A summed byte score does not distinguish independent outputs, source correctness or admitted implementation status.

The Python version retains the declaration kinds/placements but uses the verified SP3 C++ flags, snapshots, explicit exit checks and the COFF reader already used by the clean math verifier. It never installs a result into source. Per-run evidence includes candidate sources, compiler output, objects, full commands, tool/source/header hashes and distinct-output groups. Source basenames and absolute snapshot paths stay fixed across variants in one scan.

## Use

From `D:/Decomp/Bhunter`:

```powershell
python tools/padscan.py --source analysis/astra-review/matched_math.cpp --targets tools/padscan-math-targets.json --counts 0 1 2 4 8
```

Additional dimensions, for a deliberately bounded experiment:

```powershell
python tools/padscan.py --source analysis/astra-review/matched_math.cpp --targets tools/padscan-math-targets.json --counts 0 1 --kinds proto fwd extern shape --placements top inc eof
```

Every run includes an unmodified snapshot baseline even if zero is omitted. At least one target must be marked `positive_control`; a failed baseline control aborts interpretation. Each variant reports whether its control remains intact. Do not promote a result from a variant that loses its control.

The checked math manifest contains the original EXE hash, exact COFF symbol names and original function bounds. Add a target only after reviewing its boundary. Multiple `--source` arguments compile multiple source files; a target must have exactly one owning definition. Flat local header dependencies may be supplied with repeatable `--support` arguments. Basenames must be unique. The tool does not silently create missing headers/definitions or infer include dependencies.

## Compiler-fit verification

The baseline reproduced **4/4 complete routines, 335 raw bytes** in the copied context. For prototype declarations after includes, counts 1 and 2 retained four matches; 4 retained three; 8 retained two. Distinct outputs were 1 cross-product sequence, 2 point-transform sequences, 2 direction-transform sequences, and 1 translation sequence. Small class-shape context changes also altered point-transform output. Thus the diagnostic does affect this VC5 SP3 compiler, not just Squeezils' older toolchain.

Initial evidence: `analysis/padscan/run-b6420c063204/results.json` and `run-2ba29f93c1fe/results.json`. Those initial runs predate the explicit positive-control field; the later run exercises the baseline-control gate. All runs hash-check the reference and verify inputs remained unchanged.

The explicit-control run is `analysis/padscan/run-3dc5596e4e33/results.json`. A deliberately wrong translation address was then rejected by the baseline gate with a nonzero exit; evidence is `analysis/padscan/negative-control-check.json`.

## Restrictions and interpretation

- Fixed comparison setup: SP3 CL/C++/C2 tools under `analysis/toolchain-sp3/bin`, actual SP3 `math.h`, `/TP /O2 /G5 /ML /Gy`. Paths reflect this local project; the portable prerequisite manifest belongs to M0.
- Raw matching only: target relocations are rejected rather than arbitrarily masked. Relocated functions need the separately reviewed relocation-aware comparator before extending this tool. A differing-position count is a diagnostic, not an admission grade.
- It uses PE section mapping, not a hardcoded VA-to-file offset. Only up to 15 trailing alignment bytes are allowed beyond a complete target body.
- The copied parent directory differs from the input directory. The baseline exposes that difference but does not prove arbitrary paths never influence code generation. Preserve exact commands/paths and verify a proposed result cleanly in its intended translation unit.
- Unusual multiline/conditional includes require manual review of `inc` placement; it inserts after the last literal include line, matching the original experiment's intent. It is not a C++ parser.
- No concurrency against shared snapshots; every invocation gets its own run directory. Each compiler invocation times out after 60 seconds; maximum 128 requested scan configurations. This is not an automation or recurring task.
- Synthetic declarations demonstrate context sensitivity. They are not recovered historical declarations or evidence of correct gameplay. Prefer real headers/declarations and a readable functional implementation over production padding chosen solely to force a score.
- This remains optional `bm` diagnostic tooling. `func`, link admission, playable milestones and release completion never depend on winning a padding search.
