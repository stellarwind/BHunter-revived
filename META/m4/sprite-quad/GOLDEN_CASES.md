# Independent PC53 geometry and UV golden cases

`generate_cases.py` reads no restored C source and executes no original code.
It generates `tests/sprite_quad_cases.h` and `golden-case-evidence.json` using
binary64 Python arithmetic, round-to-nearest binary32 only at explicit original
stores. Zero angle avoids dependence on host sin/cos. Basis construction uses
the separately recovered normalization/cross-product contracts and00457590's
Gram-Schmidt/half-dimension stores.

The original instruction stream has asymmetric memory reuse. With O=center,
R=scaled right and T=scaled up, final stores are:

| Corner | X/Y | Z |
|---|---|---|
| A | f32(f32(O-R)+T) | f32((O-R)+T) |
| B | f32((O-R)-T) | same |
| C | f32((R+O)-T) | same |
| D | f32(f32(R+O)+T) | same |

Anchors:00457a9d/00457abf spill O-R for X/Y,00457aed/00457b01 reload
those spills for A;00457ad0/00457af6/00457b0a retain O-R for B.
A.Z at00457b57 retains subtraction.00457b30/00457b4d/00457b6a spill
R+O,00457b34/00457b51/00457b6e reload for D; C subtracts T from retained
sums at00457b41/00457b5e/00457b7b. The symbolic original stack review is
`../material-queue/quad_geometry_stack.py`.

Six deterministic selected fixtures cover all twelve corner/axis obligations:
A.X/Y require spills, A.Z rejects an extra spill, B/C reject extra spills,
D requires spills. `detects` in JSON records comparison against the opposite
spill policy per coordinate; the generator asserts all obligations covered.
Large and small origin terms create cancellation/rounding-sensitive cases.

UV sums remain on the x87 stack from0045796e..00457985 through optional flips
at004579bc/004579de. The exact constant is1.0 at00489ee8. Thus endpoint is
f32(1-(u+extent)), not f32(1-f32(u+extent)). Chosen binary32 operands have
a sum1+2^-25; premature binary32 rounding produces zero after flipping,
where original retained arithmetic produces -2^-25. Eight flag fixtures cover
U flip, V flip and transpose independently and together.

## Applying these fixtures

Parent tests should include the generated header and memcpy input words into
floats, avoiding decimal literal/compiler rounding ambiguity. Draw with angle0,
flags0, modulation1 for geometry; copy normal/up/size bit words from header.
Set camera.position equal center (side exactly zero). To retain all corners
without clipping, use projected matrix with X/Y scaled by2^-30, constant Z=4,
and zero Z row; image projected-UV policy must remain disabled. Set clip bounds
to +/-1 and near/far1/100. No inverse is needed while image policy disabled.
Capture emitter's `.local` coordinates and compare raw bits to expected A/B/C/D.
This tests full draw source through real clipping, no exposed helper required.

UV cases can use ordinary axis aligned center(0,0,4), normal(0,0,-1),
up(0,1,0), width/height2. Pass UV inputs from bit words and flags from fixture;
compare captured U/V words exactly. Compare head order A,B,C,D; all flag cases
remain front-facing. These are static original-instruction oracle fixtures,
not original differential execution or PC24/PC64 equivalence claims.
