# Complete full-texture screen quad family

2026-10-25. Three full owners recovered in `src/bh_screen_quad.c/.h`, compiled
in the actual TU, linked with real source dependencies and exercised through
actual native rendering. No original code was executed and no game launched.

|Owner|Complete boundary|Role|
|---|---|---|
|00455410|00455410–00455832,1059 bytes|engine candidate, full screen quad geometry/UV/clipping/primitive submission|
|00477680|00477680–004776b1,50 bytes|engine candidate, actual queued callback into the full primitive owner|
|00478e50|00478e50–00478f32,227 bytes|engine candidate, actual producer whose function-pointer reference targets00477680|

Boundary/get/xref inspection preceded implementation. All are ordinary complete
RET-ended owners, not verified CRT/compiler/EH/import functions or shared tails.
Engine authorship is unknown; no reusable library attribution is claimed.
Source snapshots preserve all raw/decompiler exports. `verify_evidence.py`
checks every contiguous instruction byte and constants against immutable PE.

The full mathematical owner uses the real screen clipper00453300 and a typed
synchronous primitive callback. Native fixtures bind actual
`bh_native_primitive_emit`, real native texture/cache/vertex/run owners and a
real offscreen device. Its explicit selected callback follows the existing
model/quad-owner source contract; this does not recover software0046e390 or its
software raster kernels. There is no empty/native-success substitute backend.

The original eighth float modulation argument is absent from Ghidra's inferred
signature but is explicitly read at004557d9 from stack+128 and multiplied by
float-255 before the actual CRT truncating conversion. Color is borrowed
original base-color00491758 minus that signed result times0x10101, retaining
unsigned32 wrapping. The source callback receives this actual color.

All full-image UV flips/quarter-turn selectors and both angle branches are
present. Half widths/heights store binary32, but radius squares retain one
factor and reload the other. The first left-cull comparison uses retained
radius; the following three compare the stored binary32 radius. Rotated corner
construction preserves double cosine/retained sine, binary32 width/height
products, reloaded base sums in the second corner and retained height products
in the opposite corner. No global tool/header flag changes were used.

Crucially,0049175c is **live near_z**: copied00451720 evidence explicitly writes
it from the second argument; far_z is00491760. The full-screen depth is
binary32(near_z-(-0.01)), not initialized PE1.0 and not far_z. The real screen
clip owner first receives live near_z, then every returned ring node gets the
offset depth before primitive submission.

The complete producer preserves original bucket mappings, including invalid
selectors mapping to header0, and raw geometry/modulation/flags words. Its flags
occupy command+20 and modulation+1c. Unused words stay untouched. Queue exhaustion
drops without changing the chain. Source malformed-binding/topology guards are
explicit additions. The consumer truncates flags to a byte, calls the complete
owner and reads current next afterwards, including on source lookup failure.
BH_DRAW_SCREEN_QUAD was appended without renumbering existing selectors.

`check_standalone.py` compiles the real TU and `tests/screen_quad.c`, links
`build/screen-quad/screen_quad.exe` against real source objects and normal
libraries, then executes117 checks. Pure fixtures exercise all eight raw flag
selectors, missed modulation, non-one near depth, angle/cull/partial clipping,
raw producer/bucket/full-queue and consumer failure/link behavior. Native
fixtures render an actual red/green/blue/yellow full-image atlas under all eight
UV selectors and a geometric quarter turn; all four independently expected
pixel colors and blank exterior are checked. Real TL records independently
verify live near .125, depth .135, near/depth and reciprocal W.

Parent integration adds `src/bh_screen_quad.c`, `tests/screen_quad.c`, SDK flags,
checkpoint `bh_screen_quad_checkpoint`, and map checks `_bh_screen_quad_draw`,
`_bh_screen_quad_queue`, `_bh_screen_quad_execute`. This is three complete source
owners; no full HUD callback/software renderer/playable-game admission is made.
PC24, exceptional pointers/nonfinite original behavior and huge-angle intrinsic
equivalence remain outside the tested finite PC53 contract.
