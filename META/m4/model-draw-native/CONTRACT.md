# Actual native composition control for model traversal

`tests/model_draw_native.c` exercises both recovered unlit and lit BSP owners
through the **real** native ring emitter, recovered clipping/texture queue/
residency/mode barrier owners, original material/default state and native RGB
Direct3D2 offscreen device. No fake COM interfaces, vtables or success stubs are
used. Game/original executable code is never loaded or executed.

The explicit typed model fixture has one node, one leaf, one face and three
vertices atZ4. Its +Z normal and plane-4 place eye0 on the negative side,
selecting back_leaf0; front_leaf and both child indices are-1. Edge order is the
CCW triangle surviving original CW culling after positive camera-Y projection.
The per-model view uses actual60-degree horizontal FOV, near1 and identity
orientation. Source AABB traversal and face assembly feed real00452140, then
`bh_native_primitive_emit`; native TLdepth/rhw are independently checked as0.25.
The target opts into BHSceneView.original_depth=1, zeroZ clear and actual
original GREATEREQUAL/default material render state. A real staged white texture
has flags1, actual surface/palette, residency and source-owned queue arenas.

Five separately cleared native frames prove distinct behavior:

- Unlit raw blue produces exact interior RGB0000ff.
- Directional +Z red light produces RGBff0000.
- Changing only directional color to green produces RGB00ff00.
- A true prepared point light at(0,0,8), scale/intensity1, normal+Z and AABB
  center(0,0,4) produces RGB0000ff, matching independent unit diffuse geometry.
- Texture mask0 marks the face but publishes no queue and leaves every pixel
  black. It does not mean draw all textures.

Independent pinhole math places projected triangle endpoints near18.14/45.86
on the64x64 target. Interior(32,28)/(32,32), exterior(4,4)/(55,55), exact uniform
RGB values and300..500 nonblack footprint bound verify readback instead of
accepting queue/callback counters. Pixel audit scans every bitmap pixel: the
four drawn frames contain only black and their expected RGB; the filtered frame
is entirely black. Red/green image changes are a positive lighting control using
identical geometry. Actual pixels and successful native flush/readback calls
are required for the scenario to pass.

`python META/m4/model-draw-native/build_focused.py` snapshots current m0 source
objects with SHA256 identities, compiles the real unlit/lit TUs and this new
test TU, and links `bhunter-model-draw-native.exe`. The orphan preexisting
bh_waypoint.obj has no current source and is explicitly excluded. It passes all
71 checks. Removing unlit, lit or native emitter independently produces actual
LNK2001 errors, demonstrating all three real definitions are required. The
successful link map and five bitmap artifacts are retained here.

`audit.py` verifies source/object/bitmap hashes, native dependency symbols,
three failed missing-owner controls, independent all-pixel controls and all
four immutable reference hashes. This closes the isolated-capture backend gap
for the model traversal variants; parent current full integration and its
separate real-archive scene evidence still govern final ledger admission.
Complete0046e390 software dispatch/rasterizers remain unfinished and are not
made complete by this explicitly native-only composition adapter.
