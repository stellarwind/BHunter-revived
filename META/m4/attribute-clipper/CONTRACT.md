# Attributed perspective polygon clipper 00453c30

Complete independent owner00453c30-00454fb5 contains4998bytes/1656instructions
and no outgoing calls. The two actual rendering producers00455f70/0045db80
call it seven/eight times. Its plain entry, full clipping control flow, global
bounds/scratch references and geometry callers support engine_candidate,
medium confidence. No library authorship, CRT/EH/import or shared-tail
assignment is fabricated. New files use the neutral name attribute_clipper;
the initial review folder's provisional flat-clipper name was corrected.

BHClipVertex is reused only after real-TU compile assertions verify size0x3c,
point0xc, depth0x14, UV0x18/0x1c, screen0x20/0x24, scalar0x28,
next/previous0x2c/0x30 and projected0x38. Although its existing field is named
unknown28/unsigned long, complete FMUL and FSTP instructions read/write that
word as a **float scalar**, not packed integer color. The new source accesses
its representation with memcpy. Scalar meaning (color, intensity, alpha or
another varying) remains unresolved and is not needed to preserve arithmetic.

The original second argument is an integer boolean, not an image or depth.
Nonzero first clips homogeneous point.z against near then far; generated
point.x/y, u/v and scalar28 use ordinary depth-linear interpolation. It then
walks the surviving ring and projects point.x/z and point.y/z only when the
per-vertex projected word is zero, setting it to1. Existing nonzero cache
coordinates are honored. Zero skips both depth clipping and projection,
regardless of the per-vertex cache flag; supplied sx/sy are used as-is.

Both modes continue through left/right screen-X then top/bottom screen-Y
cuts. Side intersections interpolate reciprocalZ, u/Z and v/Z, recovering
perspective-correct depth/UV. They interpolate the other screen coordinate
and scalar28 **affinely** in screen space. In a screen midpoint between Z2/U0
and Z6/U100, generated Z=3 and U=25, while scalar0..100 becomes50. Tests pin
this distinction. Side-generated point.x/y remain untouched scratch bytes;
they are not invented by inverse projection. Side-generated projected=0,
while depth-generated cache becomes1 in the subsequent projection walk.

Each cut walks backward/forward from the outside extreme, allocates a pair,
links before->first->second->after and returns first as new head. Original
min/max ordering and strict outside comparisons preserve inclusive boundaries,
winding and degenerate inside rings. Maximum pointers are retained within
each axis. Near/far/X/Y cuts require at most12scratch records. Frame/local
bytes are never touched; each scratch record receives only original fields.
Generated scalar and all float outputs use explicit binary32 stores. Eight
side intersections preserve the original retained first difference and three
stored float weights for reciprocalZ/U, while V/other-axis/scalar recompute
unspilled weights. All other arithmetic is under callerPC53/nearest/masked.
No instruction match or original numerical differential run is claimed.

BHAttributeClipContext and initializer explicitly bind original globals/bounds
and storage. Original scratch is shared with the other clipping owners;
caller-owned contexts do not grant lifetime across another clipping operation.
Consume output immediately; it borrows input/scratch and cannot be next input.
Valid pointer storage and convexity remain obligations; finite, reciprocal-link,
count and capacity guards are source adapters. ErrorNULL differs from complete
rejectionNULL/errorOK. Earlier completed cuts may remain on a later error.

Focused build `python META/m4/attribute-clipper/build_focused.py` compiles four
real TUs with native SDK header definitions and links real round/FP owners into
`bhunter-attribute-clipper.exe`. All15931checks pass. Independent array clipping
uses edge parameters and reciprocal attributes, avoiding the original
extrema-chain algorithm:480varied polygons cover both boolean modes, winding,
different heads, depth/screen cuts and attributes. A full fixture exercises
all six cuts and all12scratch slots. Separate controls cover perspectiveUV
versus affine scalar, both exact depth boundaries, cache retention/skip flags,
all six complete rejections, payload/lifetime/topology/numerical contracts.
Removing the implementation gives actual LNK2001 unresolved symbols.

`audit.py` verifies all1656original instructions, all exported producer
instructions/fifteen call sites, constants+1/-1, source hashes, focused result
and four immutable reference hashes. No game launches/original code execution.
Shared integration and ledger admission are the parent's separate work.
