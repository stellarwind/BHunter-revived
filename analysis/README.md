# Analysis prerequisites and retained evidence

This directory retains curated sources, maps and small evidence records. Large generated sweeps, compiler executables/libraries, game data, decompiler dumps and Ghidra databases are deliberately not in Git. Some historical reports link to those local run directories; they are records of experiments, not prerequisites included in a checkout.

## Local reference

Supply your original English `GAME_DATA/BHUNTER.EXE` and `Bhunter.bin`. The tested EXE's SHA256 is `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`. No game executable or assets are distributed here.

The current historical-comparison tools use local VC5 files under `D:/Decomp/MSCVC/MSVC5` and a prepared `analysis/toolchain-sp3/bin` directory. These paths are still local prerequisites, not a portable build installation. The versioned scripts document how the available original/SP3 files were copied into that ignored directory. Toolchain/bootstrap portability and a continuously linked source target belong to milestone M0.

## Comparison commands

```powershell
python analysis/astra-review/verify_fixed_math.py
powershell -File analysis/math/build_and_test.ps1
python tools/padscan.py --source analysis/astra-review/matched_math.cpp --targets tools/padscan-math-targets.json --counts 0 1 2 4 8
```

These commands require the local reference/toolchain and do not build a restored game. The native differential harness invokes only selected original math routines in private memory; it is not a general game loader. Its working linker is stock VC5 because the local SP3 linker currently fails DLL symbol resolution.

## Durable snapshots

`evidence/math-verification.json` retains the verified combined/isolated matching results and exact compiler/source/tool hashes. `evidence/padscan-verification.json` records the reviewed declaration scan; `evidence/padscan-negative-control.json` records rejection of an incorrect control address. Absolute paths identify the recorded local experiment and are not portable installation instructions.

Maps are observed boundaries and roles, not proof every region is original gameplay. `META/function-status.json` tracks admitted evidence separately from those maps. Consult `RESTORATION_PLAN.md` before expanding recovery.
