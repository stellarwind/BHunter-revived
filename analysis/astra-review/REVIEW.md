# Independent math byte-match review

Update: a complete translation match has since been verified. See [FINDINGS.md](FINDINGS.md) and [matched_translation.cpp](matched_translation.cpp). The remainder of this document records the earlier pre-match review.

Reviewed 2026-10-23. Existing source, original EXE, toolchains, and persistent Ghidra annotations were not modified. Review probes are confined to this directory.

## Conclusion

Failure of the existing probes does **not** put byte matching out of scope. It establishes that the tested reconstructions and settings do not reproduce these routines. There is no current positive evidence that matching is impossible, that the math was handwritten assembly, or that the engine used a different compiler. Runtime matching establishes a runtime-library family much more strongly than a compiler/frontend for this particular object.

Exact historical flags may be non-identifiable even after obtaining a byte match: many tested configurations emit identical bytes. Matching code is the useful target; proving a unique historical command line is a separate, potentially impossible task.

## Verified comparator and invocation facts

- Original EXE SHA256 remains `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
- The VA-to-file mapping used for both small targets is correct. The translation target is the complete 45-byte body, ending in `ret`; cross product is the complete 67-byte body.
- These two targets have no relocations, so relocation masking cannot explain their failure to match. The comparator compares actual instructions, rather than linked addresses or PE headers.
- All 240 math comparison rows contain all ten expected functions. SP3 small-probe rows contain 146 functions each; all 240 compiler invocations succeeded. Stock/VC4.1 rows contain 74 functions each and all 80 invocations per toolset succeeded. SP3 output has no warning messages.
- `/Gy` makes extracting the function through its COMDAT section end appropriate here. Accepting only NOP/INT3/zero trailing padding is reasonable for these negative results. More general future matching should validate relocation offsets and destinations and assert expected function counts; these are robustness improvements, not an explanation for the present failures.
- Local file version metadata: `CL.EXE` **11.00.7022**, `c1.dll` **11.00.7113**, `c1xx.dll` **11.00.7303**, `c2.exe` **11.00.7303**. Calling every SP3 frontend/backend 11.00.7303 is inaccurate. The driver banner alone does not identify the loaded backend.
- Loop-based multiply/inverse are not suitable compiler-identification probes against unrolled original functions.

## Effective coverage is much smaller than the nominal probe count

The existing SP3 translation sweep contains 4,080 function/configuration entries but only **14 unique emitted byte sequences**. All diverge from the original at byte zero. Cross product contains 30,960 entries but only **164 unique sequences**. Therefore 240 flag combinations should not be interpreted as 240 independent tests of code-generation hypotheses.

The original translation begins by loading x, y, and z from the argument stack onto x87, exchanges ST0/ST2, then loads the matrix pointer and performs the additions. The original cross product preserves sequential stores between components. These are useful scheduling/dataflow signatures; simple positional equality scores hide their significance.

## Isolated experiment: source temporaries and C versus C++

`probe.py` compiled seven translation source forms under 96 configurations: `/TC` or `/TP`; `/O1`, `/O2`, `/Ox`, or `/Og /Ot`; `/G3`, `/G5`, or `/G6`; and default, `/Op`, `/Oa /Op`, or `/Ow /Op`. Every build also used `/c /ML /Gy`. Results are in `translate-results.json`, and the generated source is `translate-source.c`.

There were 672 function results and 36 unique byte sequences. No exact match was obtained. C versus C++ materially changed scheduling for some forms even though the source has C linkage and the same backend.

The most useful lead, **not a near-match**, shares the first eight bytes with the original:

```c
extern "C" void matrix_double(float x, float y, float z, float *m)
{
    double a=m[3], b=m[7], c=m[11];
    m[3]=(float)(x+a);
    m[7]=(float)(y+b);
    m[11]=(float)(z+c);
}
```

Flags: `/c /ML /Gy /TP /O2 /G5` (several other tested combinations produce the same code).

Original prefix: `d9442404 d9442408 d944240c d9ca 8b442410 ...`

Candidate prefix: `d9442404 d9442408 8b442410 d944240c ...`

Changing the same source to `/TC` moves the matrix-pointer load even earlier, after the first FLD. Ordinary direct assignments, argument updates, and float result temporaries often collapse to the same code. The changed temporary types/frontend are evidence that the unexplored source dimension matters, not proof that the original source used doubles or C++.

## Behavioral harness limits

The isolated-image harness and the four normalization constant relocations are consistent with the reported test. The inspected original normalization uses inline `FSQRT`, FCOM/FNSTSW testing, and x87 arithmetic, without an external sqrt call. The recovered C uses `sqrt` and a `double` temporary; frontend intrinsic decisions and temporary representation therefore remain byte-matching variables.

The finite dyadic fixtures explain why broad claims would be premature. Their small products/sums frequently fit within x87 precision, reducing sensitivity to expression ordering. The documented 24-bit discrepancies must remain explicit. For future exceptional-value fixtures, the tolerance comparison needs explicit NaN/infinity classification: `fabs(NaN) > tolerance` is false and would otherwise miss a numerical failure. This does not invalidate the current finite-input results.

## Ranked next experiments

1. **Use translation as a controlled compiler fingerprint.** Keep SP3 and a small fixed optimization set. Explore explicit float-to-double operand promotions and captured-input/source-temporary forms under both frontends, comparing instruction/dataflow schedules and deduplicating emitted code. Record complete compiler commands and actual tool-file identities. This directly tests the newly demonstrated source/frontend sensitivity without another mostly redundant broad flag sweep.
2. **Use normalization as an independent check.** Model the original x87 stack/order and test a small set of named float versus double temporaries and square-sum parenthesizations with intrinsic sqrt enabled. Require a second routine to support any tentative source/compiler hypothesis. Do not infer original C types solely from Ghidra's floating-point decompilation.
3. **If controlled source experiments plateau, compare another actual backend or identify engine provenance.** Inspect neighboring engine functions and object/library boundaries, and compare candidate older engine libraries or other toolchains using the same fixed probe corpus. Mixed object provenance is compatible with VC5 SP3 CRT linking, but remains a hypothesis. Linker version is not sufficient to choose another compiler.

No hard blocker currently prevents pursuing byte matching. The missing information is original source form and per-object compilation provenance. Reproducing instructions in assembly can preserve exact implementation where needed, but doing so would be reconstruction, not evidence that the original was handwritten assembly or that the C reconstruction matched its compiler.
