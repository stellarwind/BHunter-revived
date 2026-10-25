# Proven degenerate ribbon correction, bounded static question

Question (2026-10-25 10:03UTC, bounded10-minute review): how does complete
00455f70 handle duplicate projected points produced by valid short queued
paths? Positive control: retained normal two-/three-point ribbon fixtures and
the already source-restored normalization zero-input contract. Stopping
condition: establish raw zero-direction/zero-miter branch and verify a precise
source correction with fresh linked tests; do not broaden into other ribbon
matching/backend work. This question was resolved in the same bounded block.

Before editing, root preserved historical `bh_sprite_oriented.c/.h` and ledger
records in `../prior-ribbon/`, source SHA256
169cb33ab54b536bed3e3541db8375a9849c91a80c87e919f54a4898b432b77d.
The corrected source is a separately validated implementation variant. It is
not an additional original-function count and cannot inherit the historical
variant's linked evidence. Both original owners remain engine candidates:
00455f70 complete2620B/677instructions, 004569b0 complete895B/305instructions.
All instruction bytes and constants are freshly pinned to immutable PE by
`export_review.py`; no original code was loaded or executed.

## Raw contract

At0045601b the original calls normalization and immediately proceeds without
testing EAX. Recovered004011f0 leaves an exactly zero vector unchanged and
returns0. Thus a zero projected edge builds zero offsets and finite degenerate
corners; rejecting its normalization return was an extra source restriction.

At00456234 the original computes miter dot/denominator, including0/0 for
duplicate/zero-width edges. Under its currently admitted masked FP profile this
produces NaN and sets invalid sticky status. FCOM00456238 compares against exact
double-0.8. FNSTSW00456268 and TEST AH,1 at0045626e test C0 alone. Unordered
sets C0=1 and therefore takes the same branch as less-than, popping NaN and
loading **float**-0.800000011920929 at00456275. It then computes
1/(clamped_dot-(-1double)), a finite value near5. The finite corner sums yield
finite offsets; all-zero sums remain zero. This is neither an unconditional
NaN propagation nor a denominator rejection. Exact constants are in JSON.

## Precise correction and actual tests

Source now ignores the normalization return; removes denominator-zero rejection;
retains the actual floating division; and expresses the unordered fallback as
`if (!(dot >= -0.8)) dot=(float)-0.8`. All normal finite control paths retain
their previous arithmetic. No assembly, success stub, alternate rasterizer or
fabricated native callback was added. The source header explicitly requires
the existing PC53/nearest/masked profile and states sticky status is retained.

Fresh `build_check.py` compiles the real corrected TU and actual math,
attribute-clip, rounding and FP source dependencies, links named
`bhunter-oriented-degenerate-check.exe` and runs `tests/sprite_oriented.c`.
`standalone-build.json`:34checks,0failures. New fixtures exercise duplicate
two-point edge, three duplicate points and a duplicate edge followed by a
nonzero edge through the actual near-plane dispatcher, ribbon geometry,
attribute clipper and synchronous fixture emitter. They verify finite zero
corners, finite analytical2.5-unit mixed miter, shared corners and pool reset.
An isolated `_clear87`/`_status87` positive control proves the real masked0/0
still sets `_SW_INVALID`; tests clear status afterward. Existing normal path,
reversal clamp, near-plane, flags, width taper and failure fixtures also pass.

The parent path-queue target owns count1/2 end-to-end/native composition and
full fresh integration. No success from that target is invented here. PC24,
unmasked exceptional execution, huge inputs and original software dispatch
remain outside this finite masked correction's validation scope.
