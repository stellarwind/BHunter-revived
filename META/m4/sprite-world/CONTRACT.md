# Sprite and recursive resource draw integration

Four complete ordinary rendering owners are admitted: billboard `00455960`,
ribbon geometry `00455f70`, near-plane path clipping `004569b0`, and recursive
resource dispatch `0045adf0`. `owners.json` records their complete contiguous
instruction bodies, immutable EXE byte hashes, source owners and classification.
The 1,554 instructions are evidence of reviewed boundaries, not a completion
percentage. Historical engine provenance remains unresolved. CRT conversion and
math dependencies are reused; no compiler helpers or shared tails are counted.

The dependency family compiles in its real translation units and links with real
definitions into `build/m0/bhunter-integration.exe`. Its 142 translation units
pass 3,398,887 checks with real assets and 2,906,653 in a clean checkout without
the originals. All 71 missing-module controls fail with unresolved symbols.
The focused source checks comprise 50 billboard, 27 path/ribbon, 25 resource
dispatch and 67 native composition checks. Family contracts describe their
layout, precision, mutation and borrowed-lifetime details separately.

The native composition check exercises the actual recursive dispatcher, source
sprite geometry, clippers, texture queues and Direct3D2 on an offscreen target.
Saved billboard, half-modulation, ribbon and rejected-LOD bitmaps establish real
pixel output and its negative control. Queue fixtures independently check depth,
per-vertex modulation and software rejection. Native RGB565 readback of half
modulation allows one quantization step; queued colors remain exact fixtures.

`bh_native_attribute_emit` is an explicit partial hardware adapter for
`0046e430`, converting each vertex's scalar using the reviewed grayscale rule.
Its original software branch/rasterizers remain unfinished, are rejected by
the adapter and do not count among the four recovered functions. It executes
real recovered queue owners and native APIs rather than accepting stub success.

The real-level static snapshot now dispatches each selected shell through
`bh_draw_resource`, then the recovered model-instance owner. It retains 1,798
polygons, 3,112 triangles, 146 textures and a byte-identical image. The shell
filter, placement loop, descriptor/cache binding, camera and diagnostic lighting
are still explicit adapters. This is resource recursion (depth-based LOD), not
recovered entity traversal, descriptor construction or gameplay.

PC53/nearest/masked remains the admitted numerical profile. Source guards bound
invalid data, recursion and path capacities; original PC24 edge fidelity and
huge-angle trigonometry remain unvalidated. No byte match or original differential
equivalence is claimed. No game or original game code was executed.

The next frontier is the resource queue consumer `004775f0`, descriptor
construction/lifetimes and real entity bindings, subject to boundary/dependency
review. Software rendering and atlas glyph copy remain open. M4 has not passed.

Audit with `python tools/check_sprite_world.py`. Historical model-instance and
parallel-block audits resolve changed shared sources through byte-pinned
snapshots, retaining their exact tested variants and original evidence scope.
