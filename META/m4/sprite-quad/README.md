# Arbitrary-plane quad and queue restoration

The complete engine owners00457590,00478730 and00477760 are restored in
`src/bh_sprite_quad.c` and `src/bh_quad_queue.c`. These are ordinary RET-ended
geometry/queue functions called by game scene/HUD paths, not CRT, imports,
compiler support or shared tails. Engine authorship remains unresolved. The
existing billboard/ribbon types did not implement this arbitrary-plane owner.

`export_review.py` verifies every original instruction byte and body boundary
against the immutable PE. `owners.json` records the three complete bodies;
`constants.json` pins original float values. This is static provenance, not a
claim that the new C instructions match the executable.

The quad owner negates the input normal, tests its eye-facing side before
normalization, orthogonalizes/normalizes up, forms the cross-product basis,
rotates and scales it, builds both windings and UV mappings, transforms all four
vertices and calls the real polygon clipper. It preserves flat modulation,
world-light evaluation and directional/ambient shading, including the two
separate integer conversions in each channel. The decompiler missed float
argument15 (modulation); raw stack offsets establish it. Texture UV inputs are
start/extent pairs. The queue producer preserves all nineteen payload words,
including the original flags/modulation word swap. Its consumer reads the low
flags byte and publishes the next link after the actual draw returns.

Independent instruction-derived cases catch asymmetric binary32 geometry
spills and retained UV sums; see `GOLDEN_CASES.md`. Rotated Z basis values remain
retained until scaling; `ROTATION_REVIEW.md` records the instruction anchors.
Focused fixtures exercise real math, clipping, lighting, queue storage and the
native primitive/texture/render-state owners. Offscreen pixel controls cover
white, half modulation, rejected/admitted backfaces and directional red. These
are source-only static compositions, not game execution or gameplay automation.

The selected emitter is an explicit synchronous dependency as in the existing
model owners. The native binding uses the recorded partial native primitive
adapter; neither0046e390's software dispatch nor raster kernels are admitted by
this family. Source guards reject malformed bindings and nonfinite/overflowing
consumed values; they do not claim original malformed-pointer behavior. Exact
PC24, extreme/transcendental edges and universal original numerical equivalence
remain unvalidated. Parent integration/clean results and per-variant ledger
admission are recorded separately in `../frontier-90min/`.
