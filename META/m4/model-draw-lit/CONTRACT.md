# Lit BSP traversal 00459510

Complete candidate00459510-0045a7b3 is4772bytes/1401instructions. It is an
ordinary recursive engine_candidate owner with two calls each to itself,
prepared light evaluator00450cd0, face00458310 and list00458430. Six calls to
verified CRT __ftol are reused signed conversion dependencies, not recovered
game/engine functions. Full instruction review identifies no EH/import/shared
tail or available library source attribution.

Visibility, plane bit masks and painter order are the same contracts as unlit
traversal: six per-model local-space planes, min support>=0 rejection, max
support<0 clearing with exact16bit AND masks, camera-side child first, selected
leaf chain next, other child as tail. However, **addition scheduling differs**:
lit support normally uses Y+(X+Z); minimum ny>=0/nz<0 or maximum ny<0/nz>=0
uses Z+(X+Y). This is intentionally a separate source helper. Cancellation
fixtures retain a unit residual in lit while the actual frozen unlit owner
loses it on the same box, producing different original visibility decisions.
No shared helper or earlier source variant was changed to flatten that detail.

The node is shaded after its first child returns and before its selected leaf
is submitted. There is no persistent original node-color cache. The existing
face/vertex integer epoch caches still determine whether a face emits again.
Shading updates draw.color even when the selected leaf is negative. The next
tail child may change that color again; no artificial parent-color restore is
added. Earlier output is not rolled back after a later guard/emission error.

If dynamic lighting is enabled, point is the AABB midpoint, each coordinate
stored binary32 after retained(min+max)*0.5. The raw node normal at+0x18 is
passed to the real prepared evaluator on **both** eye sides, with the model
scale. It is not flipped for the back-side leaf. The evaluator's clamped packed
RGB bytes supply the initial channels. Prepared type/list/override semantics
are the real admitted evaluator/preparation owners, including its reference
override and mode-specific attenuation. Type1prepared_type is owning metadata
that preparation deliberately preserves; it is not synthesized by this module.

If directional configuration is enabled, the explicit init adapter transforms
its configured direction through the real model inverse direction owner,
without normalization. Dot(normal,local_direction)*scale is retained atPC53.
Negative eye side uses that dot directly; nonnegative side negates it. Negative
alignment clamps to0. Each channel computes
(alignment*directional_channel+dynamic_byte)+ambient_channel, uses signed32
truncation toward zero and then clamps0..255. When directional is disabled,
ambient is not added separately. Packed result is added to base_color with
original32bit wrap/carry behavior; it is not ORed or given invented alpha.

BHModelDrawLitContext binds existing draw state, prepared light context,
directional config, model-local direction, raw enable, scale and base color.
Changing direction/config/model inverse requires rebind, matching original
per-model wrapper setup. Mutable stack/cache storage is borrowed from the
existing unlit cache owner; callback/nonreentrancy/lifetime rules apply. New
guards reject nonfinite/out-of-signed32 conversion rather than invoke undefined
C behavior. They are explicit adapters; no flat lighting success stub exists.

`python META/m4/model-draw-lit/build_focused.py` compiles eleven real TUs and
links `bhunter-model-draw-lit.exe` using real light config/preparation/evaluation,
unlit face/list/cache, matrix transform, clipper, general inverse, round andFP
definitions. All43338checks pass:4096 varied mask/configuration cases with
independent eight-corner visibility, recursive order and analytical channel
oracles; real point lighting at changing midpoint; same raw normal on both
sides; transformed nonunit direction; truncation/clamping/sign; base wrap;
disabled modes; absent-leaf color store; numerical guard; and cancellation
positive control against the actual frozen unlit owner. Removing lit source
produces real LNK2001 errors. Isolated capture is explicitly test-only.

An initial fixture forgot that type1prepared_type is retained owner metadata,
then used too small a preparation radius for its changed point-light distance,
selecting the real evaluator's approximation mode. Fixture initialization and
radius were corrected; lit source did not change to conceal those failures.
An early cancellation fixture had its expected arithmetic order reversed;
the corrected independent lit/unlit control now verifies the actual distinction.

`audit.py` verifies all1401instructions, call counts, raw-normal/FCHS/color-store
anchors,0.5constant, source hashes, target result and four immutable references.
No game launch or original code execution occurs. Review remains WIP pending
parent real native composed scene/current integration; complete0046e390 software
dispatch/rasterizer behavior is explicitly unfinished and is not admitted here.
