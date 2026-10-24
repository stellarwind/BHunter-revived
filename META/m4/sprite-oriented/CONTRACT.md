# Oriented sprite path and ribbon owner

Reviewed 2026-10-24 from `sprite-world/original/004569b0.{tsv,c.txt}`,
Ghidra decompilation of `00455f70`, and its incoming/outgoing call edges.

## Classification and boundary

`004569b0` spans `004569b0`–`00456d2e` (305 instructions, `RET`). It is a
game/engine-candidate oriented-sprite near-plane path clipper, not a CRT,
import, compiler helper or shared tail. Three callers (`0045adf0`,
`004777c0`, `004779e0`) supply the sprite path and renderer argument. Its only
callee is `00455f70`, the related proprietary ribbon geometry owner.
`00455f70` spans `00455f70`–`004569ab` (677 instructions, `RET`), with real
math `004011f0` and clipper `00453c30` dependencies. It dispatches clipped
polygons through unrecovered original `0046e430`. No general provenance has
been asserted from its address range alone.

The original `004569b0` is `void __cdecl(int renderer)`; `00455f70` is
`void __cdecl(path_vertex *, int renderer)`. The context API is a source-only
binding of their globals, pool and renderer. The native path entry has a
verified 0x54-byte layout: projected path point at 0, four corner triples at
0x0c, previous/next pointers at 0x3c/0x40, flags at 0x44, scalar at 0x48,
width at 0x4c and longitudinal texture coordinate at 0x50. `used` represents
the original mutable `004edeac` count. `near_z` replaces `0049175c` and
`projection_scale` replaces `004e9c38`; callers must bind them to actual
source-owned values. The path and image are borrowed. The callback consumes
each temporary polygon synchronously and may inspect `BHClipVertex.unknown28`
as a float scalar to apply the original per-vertex grayscale adjustment.

`004569b0` walks the forward path. A behind-to-front edge creates a new
visible head; front-to-behind creates a visible tail, cuts the old next node's
back-link and calls the ribbon owner. The final visible chain is emitted at
end. Crossing attributes are interpolated in original field order, including
flags from the current vertex. Temporary pool entries are recycled after
each path dispatch; `used` resets to zero after the complete call. Input
pointer storage and chain topology are caller contracts; source guards reject
invalid pointers, nonfinite values and insufficient pool capacity.

`00455f70` projects path coordinates, scales width by `(projection_scale *
0.5) / z`, builds four ribbon corners per segment, reconstructs mitered
joints from the two adjacent segment corners and clamps cosine below -0.8 to
the stored float constant. Direction sums are stored as floats, while the
reciprocal scale remains in x87 precision through multiplication; x/y products
then store as floats before center addition/subtraction. UV bits 0, 1 and 2 flip longitudinal, flip
transverse and exchange UV axes. Tapering segments produce three polygons;
balanced widths produce one. Each polygon uses the admitted real
`bh_attribute_clip_polygon(..., 0)` owner before synchronous emission.

## Integration and validation

Real source translation unit: `src/bh_sprite_oriented.c` with
`src/bh_sprite_oriented.h`. Public `bh_sprite_oriented_draw` maps to
`004569b0`; public `bh_sprite_oriented_ribbon` maps to `00455f70` and is
also exercised directly. Dependencies: `bh_math.c`, `bh_round.c`,
`bh_attribute_clipper.c`, `bh_fp.c`, and a real bound primitive callback. A
focused 32-bit source executable linked these real definitions with
`tests/sprite_oriented.c` and passed 27 checks, covering both near crossings,
fully visible/hidden paths, split paths, projected geometry/interpolation,
UV flags, widening/narrowing polygons, an ordinary and near-reversal miter,
capacity rejection, bounded cycle rejection, guarded texture-coordinate
conversion and callback failure. Command:

```
gcc -m32 -std=gnu89 -Isrc -DBH_SPRITE_ORIENTED_TEST_MAIN tests/sprite_oriented.c src/bh_sprite_oriented.c src/bh_attribute_clipper.c src/bh_math.c src/bh_round.c src/bh_fp.c -lm -o build/sprite_oriented_tests.exe
build/sprite_oriented_tests.exe
```

This is a functional source reconstruction, not a byte match. Complete
software primitive dispatch at original `0046e430` is still unrecovered. A
native source adapter may map each vertex's scalar to its output color; using
the general uniform-color polygon emitter alone loses that behavior. Source
tests do not establish 24-bit x87 edge equivalence or source-game playability.
