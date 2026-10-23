# First math recovery pass

## Recovered source and layouts

Source: `../../src/bh_math.c`; public declarations: `../../src/bh_math.h`.

These are behavioral reconstructions with new descriptive names, not the original source or original type names. No library provenance is established for these routines.

`BHVec3` is 12 bytes: x/y/z floats at 0/4/8. `BHMat34` is 48 bytes: three rows of four floats, translation column at offsets 12/28/44. This describes accessed storage; it does not establish the original project's global structure packing flag.

| Original VA | Recovered function | Bytes in original body |
|---|---|---:|
| 00401000 | bh_mat34_multiply | 408 |
| 004011a0 | bh_vec3_cross | 67 |
| 004011f0 | bh_vec3_normalize | 113 |
| 00401270 | bh_vec3_normalize_to | 111 |
| 004012e0 | bh_mat34_transform_point | 116 |
| 00401360 | bh_mat34_transform_direction | 107 |
| 004013d0 | bh_mat34_inverse_transform_point | 140 |
| 00401460 | bh_mat34_translate_world | 45 |
| 00401490 | bh_mat34_translate_local | 123 |
| 00401a70 | bh_mat34_inverse_rigid | 179 |

Important contracts:

- The cross product writes each component before computing the next. Passing an input as output therefore produces the original's unusual sequential-update behavior, rather than an alias-safe mathematical cross product. The reconstruction preserves this and tests both input/output alias choices.
- Normalization returns an integer status. Zero vectors return zero; normalize-to leaves the destination unchanged in that case.
- The inverse operations transpose the rotation basis. They are true inverses only for rigid transforms with an orthonormal basis; they do not calculate a general matrix inverse.
- Matrix multiplication and inversion are reconstructed with loops for readable behavior. The original uses unrolled instruction sequences. These loop forms are not suitable for inferring original flags from a failure to byte-match.
- Matrix multiply/inverse use distinct input and output storage. Other transform alias patterns are not comprehensively tested.

`../ApplyMathTypes.java` applied both types and ten cdecl signatures to the persistent Ghidra project. `*-typed.c` files are refreshed decompiler snapshots after that change. The header's const qualifiers express intended read access; they are not recovered source qualifiers.

## Native differential test

Run from the repository root:

    powershell -File analysis/math/build_and_test.ps1

The harness loads the original image into private memory, but never invokes its entry point, initialization, imports, renderer, or game loop. It invokes only the ten selected original math routines. Because address 00400000 is unavailable in this test process, it adjusts exactly four verified x87 constant operands in the in-memory copies of the normalization routines to point at the copied 0.0 and 1.0 constants. The original disk EXE is untouched. No other instruction changes are made.

Both sides run in the same 32-bit process. The recovered C uses SP3 compiler backends with /O2 /G5 /Oi /ML /Gy for this experiment. These are **test build settings, not identified original flags**. The SP3 linker fails DLL entry-point resolution on this host, so the harness uses the working original VC5 5.00 linker; that does not change the compiler-generated math object code being compared.

Fixtures: 2,000 deterministic cases per function per precision mode, with finite values roughly in [-32,32), in increments of 1/1024. The first input vector is zero. Cross-product fixtures additionally include output equal to either input. x87 precision modes are 24, 53 and 64 significand bits. This is not an exhaustive IEEE-754 test: NaN/infinity, subnormals, extreme magnitudes, signed-zero payload details, alternate rounding modes, and all alias combinations remain unverified.

Results in `differential-results.txt`:

- **60,000 comparisons**; no return-value mismatch or difference exceeding `1e-5 * (1 + abs(original) + abs(recovered))`.
- All **40,000 comparisons at 53/64-bit precision were bit-exact** for these fixtures.
- At 24-bit precision, multiply, transform-point, transform-direction, inverse-transform-point and local translation differ in some float bits. These differences must remain tracked; the tolerance pass does not establish bit-for-bit fidelity.
- Cross product, both normalization functions, world translation and rigid inverse were bit-exact in all three modes for these fixtures.

An eventual port must determine the precision and rounding state actually used by the game and graphics runtime. These test results do not establish that state.

## Compiler option comparison

`compare_builds.py` compiles these ten source functions under **240 combinations** of optimization, CPU target and alias/precision options. It compares each COFF code section against its original routine, excluding alignment padding and masking relocation operands. Candidate flags and per-function lengths/prefix matches are in `build-comparison.json`; the objects are retained under `builds/`.

Separately, `../run_compiler_probes.py` now includes both raw float-array and typed-struct forms, varying commutative product operand order. The SP3 probe matrix has also been expanded to 240 flag configurations. The earlier stock VC5 and VC4.1 result files retain the earlier 80-configuration matrix.

**No exact code match has been obtained.** There is no justified single winning flag set. The probes establish that these particular source forms under those tested settings do not reproduce the original instructions. They do not exclude the toolchains globally, identify a different compiler, or prove handwritten assembly. Remaining explanations include different source expressions/temporaries, per-file options, older precompiled engine objects, another compiler using VC5 runtime libraries, or assembly routines. These are hypotheses, not findings.

The SP3 runtime identification remains supported independently by the full CRT code-section matches. For the recovered routines, data layout and finite-input behavior are better established than code-generation provenance.

Next targeted work: resolve the 24-bit precision differences from instruction-level operation ordering; unroll one matrix function in original store order; inspect its callers and nearby math functions for source/ABI clues; then extend the compiler comparisons. Avoid choosing a global /O2 or /G5 setting merely because the test harness uses it.
