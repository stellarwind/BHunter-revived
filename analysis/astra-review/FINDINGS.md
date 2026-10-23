# Verified translation byte match

The reconstructed `bh_mat34_translate_world` in [matched_translation.cpp](matched_translation.cpp) reproduces **all 45 original instruction bytes at VA 00401460**, including the return. No relocation operands are masked, no instructions are patched, and no assembly or emitted byte directives are used. The COFF section has three additional alignment NOPs, which are excluded from the function comparison.

The saved source was built twice in fresh directories by [verify_translation.py](verify_translation.py). Both clean builds passed and produced the same object SHA256. The original executable remains unchanged.

## Reproduce

From `D:\Decomp\Bhunter`:

```powershell
python analysis/astra-review/verify_translation.py
```

The verifier invokes the local SP3 compiler tools with:

```text
/nologo /c /ML /Gy /TP /O2 /G5
/ID:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc/include
```

It compiles the saved `.cpp` file, which includes `<math.h>` and defines the function with C linkage. The script clears inherited `CL` and `_CL_` options and records the full command, working directory, tool hashes, source/header hashes, object hashes, comparison mask, and original bytes. The current complete evidence is [latest-verification.json](latest-verification.json), also retained in [verified-2e68d87ec8a0/verification.json](verified-2e68d87ec8a0/verification.json).

```c
#include <math.h>
extern "C" void __cdecl bh_mat34_translate_world(
    float x, float y, float z, float *m)
{
    m[3] = (float)(m[3] + x);
    m[7] = (float)(y + m[7]);
    m[11] = (float)(z + m[11]);
}
```

The 45-byte body is:

```text
d9442404d9442408d944240cd9ca8b442410d8400cd9c9d8401cd9cad8402cd9c9d9580cd9c9d9581cd9582cc3
```

Its SHA256 is `f7f73b8e266dd63922af978901b8f1b649f37b4938f519964ae9676bb9aa5ded`.

Both final clean object files have SHA256 `1f4c78e23b86991a164970defabf6880bcb257b19c9a1f90e7738d340c1411d7`. Original EXE SHA256 is still `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.

## Why the earlier probes missed it

The compiler's surrounding translation-unit context matters. A controlled mixed-operand corpus first produced the complete match under both C and C++. Extracting the same body alone removed the match. Rebuilding the corpus cleanly reproduced it.

A bounded context check then found that the actual VC5 SP3 math header, with the C++ frontend, was sufficient: **one standalone reconstructed function and the real header reproduce the match**. Dummy preceding functions are unnecessary in the delivered source. The same renamed function/header built with `/TC` did not match; removing the header also did not match. The experiments establish this context dependency without identifying the compiler's internal reason for it.

This result directly demonstrates why the earlier negative probes were insufficient to rule out VC5 or byte matching. Explicit promotions helped reveal source/frontend sensitivity, but the final matching body needs no double promotions.

## What is and is not established

- A normal high-level source reconstruction reproduces this entire routine under an optimized VC5 SP3 configuration.
- `/TP /O2 /G5` is a verified working configuration for this saved source and header context. It is not proven to be the unique historical command line or the flags for the rest of the game.
- The reconstruction does not establish the original function name, original source text, original header set, or original language. Other sources and contexts can yield the same bytes; the original mixed corpus matched with both frontends.
- File versions are recorded in [tool-versions.json](tool-versions.json): driver 11.00.7022, C frontend 11.00.7113, C++ frontend 11.00.7303, and backend 11.00.7303.
- Existing `src/bh_math.c`, its header, parent comparison scripts, and Ghidra annotations were not changed.

## Independent normalization check

Neither normalization routine (`004011f0`, 113 bytes; `00401270`, 111 bytes) matched in this bounded pass. For each routine, the pass compared 2,320 source/configuration results across captured float/double components, square-sum order and association, length type, explicit operand promotions, reciprocal multiplication order, arrays versus typed structures, header context, C/C++, and selected optimization options. Many results duplicate the same generated instructions; these are not 2,320 independent compiler hypotheses.

The best matching prefix remains five bytes. The original then loads components through x87 and stores two float temporaries; representative candidates instead copy float bits through integer registers before loading the temporaries. Some C++ candidates reproduce the original reciprocal-multiplication/store tail, but their complete bodies still fail comparison. This is a concrete remaining code-generation difference, not a matching result.

The normalizer comparators permit masking only their two genuine absolute constant operands at function-relative offsets 53 and 72. Their surrounding opcodes and original addresses are checked. Candidate relocation descriptions and referenced constant bytes are retained. No normalization match is claimed.

Further normalization work should inspect that specific component-load/temporary formation and source/header context before another broad flag sweep. This pass stops at the verified translation milestone and the normalization plateau.

## Experiment evidence

[targeted_probes.py](targeted_probes.py) contains the source generators and COFF comparison logic. Each experiment directory preserves source, bodies, compiler commands/logs, object files, per-function masks, and a unique-output table. [deduplicated-index.json](deduplicated-index.json) additionally groups outputs across 91 stages by instruction bytes and relocation identity, excluding section-tail padding; it contains 775 distinct patterns. This includes positive and negative controls and bounded context experiments.

The earlier [REVIEW.md](REVIEW.md) records the pre-match assessment; this report supersedes its no-match status.
