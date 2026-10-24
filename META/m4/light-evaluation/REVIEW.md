# Lighting evaluators 00450cd0 / 00451140

Both complete owners are engine candidates, with unknown historical authorship.
Prepared evaluator callers are 00459510; world evaluator callers are 00457590
and 00456d30. They are ordinary cdecl point/normal/scale functions returning
00RRGGBB. Their only calls are verified CRT `__ftol`; that compiler/runtime
dependency is reused, not counted as recovered game code. Jump tables and all
instructions are checked against the immutable original PE by `audit.py`.

The source takes the existing real BHLightContext global bindings and returns a
success flag plus packed output. Prepared mode uses prepared_type/local position
and direction; an active override selects local_reference. World mode uses type,
world position/direction and the raw override. Inactive/unknown tags contribute
nothing. Type1 adds effective_intensity. Type2 uses positive negative normal dot
direction times effective_intensity. Point/spot use intensity and linear range
falloff; nonpositive range means unbounded. Normals/directions are not normalized.
Spot cones test the retained cosine against outer, then reload binary32 cosine
for the inner test and ramp. Equal inner/outer on the accepted axis avoids division.

Instruction review preserves nonuniform FP stores and addition order:

- Directional and diffuse sums are `(Y+Z)+X`.
- Point deltas X/Y remain retained; Z is stored binary32 and its retained value
  multiplies the stored value for length. The diffuse Z product uses stored Z.
- Spot length uses each retained delta multiplied by its binary32 store. Both
  dots use stored deltas. Prepared cone dot is `(Z+Y)+X`; world is `(X+Y)+Z`.
- Distance, point contribution, spot attenuation/base contribution and cosine
  have original binary32 stores. Spot ramp remains retained for color products.
- Red and green accumulate retained; blue stores after each contribution.
  Green stores once before final scale while red remains retained. Conversion
  truncates toward zero, then each channel clamps to 0..255.

`build.py` compiles the real evaluation translation unit and real preparation,
math, rounding and FP definitions with pinned SP3 compiler/headers, then uses
the working stock VC5 linker to produce `build/light-evaluation/bhunter-light-evaluation.exe`.
The focused target passes 2,100 checks: analytical four-mode dyadic sweeps,
independent 3-4-12/13 controls, front/back and nonunit normals, cone/range edges,
actual prepare-to-evaluate chains, channel truncation/clamping and cancellation
controls exposing red/green/blue store asymmetry, override/prepared/world field
selection, empty/unknown modes, read-only input, pointer/output aliasing and
atomic rejection of invalid consumed inputs/cycles/extreme conversion values.

No original code runs and no game is launched. This is functional PC53 source
recovery, not byte matching. Source guards deliberately reject malformed inputs
and scaled channels outside signed32 conversion range; original CRT low32
overflow behavior for extreme values is not admitted. FP sticky exception flags
and PC24/PC64 are not claimed. Parent integration evidence is recorded separately;
the static scene renderer and gameplay are not changed by this family alone.
