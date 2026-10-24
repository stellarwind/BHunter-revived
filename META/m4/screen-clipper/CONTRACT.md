# Uniform-depth screen polygon clipper 00453300

Complete owner00453300-00453c2a is 2347 bytes/777 instructions with no outgoing
calls. Its plain entry, complete control flow, global bounds/scratch use and
three geometry callers distinguish it from CRT/import/EH/shared-tail code.
Role remains engine_candidate; original authorship/library source is unknown.

The initial assignment called it a colored clipper. Complete producer review
corrected that interpretation before implementation: 00454fc0/00455410 and
00455960 construct screen-space sprite quads with UV attributes. This owner
takes a 60-byte vertex ring and a *single* float depth, rejects that depth
outside [near,far], then clips screen X against left/right and screen Y against
top/bottom. It does not transform/project vertices or interpolate per-vertex
depth. The accurate new API is `bh_screen_clip_polygon`; no six-plane general
clipper or missing sprite producer is claimed as implemented.

Reusing BHClipVertex is verified in the real TU with compile-time checks:
size0x3c; point.z0x14; u/v0x18/0x1c; sx/sy0x20/0x24; next/previous0x2c/0x30;
projected0x38. Native DirectX headers are inherited through bh_clipper.h and
used as real SDK definitions, not competing local types. All other vertex
fields are irrelevant to this owner and retain their bytes.

The algorithm finds min/max X in ring order, applies strict outside comparisons
so boundaries are included, cuts left then right, finds Y extrema on the result
and cuts top then bottom. A cut walks backward/forward from the outside extreme
until it reaches each surviving chain endpoint, allocates two scratch records,
and splices before -> first -> second -> after. Its new head is first, matching
original return choice. The second cut within each axis retains the original
maximum pointer. Convex topology is a caller contract. Winding and degenerate
inside rings are preserved; boundary duplicates are not silently removed.

Each cut sets depth, u, v, screen coordinates, projected=0 and four links only.
UV and the other screen axis use affine weighted sums
((bound-b)*attribute_a+(a-bound)*attribute_b)/(a-b), with no reciprocal-Z.
Explicit `bh_round_float` stores reproduce the binary32 output boundaries;
intermediate weights remain PC53 doubles, consistent with unspilled original
x87 arithmetic. No perspective correction is borrowed from owner00452140.
Existing input node payloads (including depth/cache) are untouched except
surviving links. Four cuts require at most eight scratch records. The original
shared global scratch lifetime ends at the next clipping operation; source
contexts provide explicit storage but callers must consume output immediately
and must not rely on persistence across clipping calls in either family.

The config initializer and finite/topology/capacity guards are explicit adapters.
Valid pointer storage and convexity remain caller obligations. Guards reject
nonfinite values actually consumed; an unused input point.z may contain NaN
without changing the uniform-depth decision. NULL/errorOK is rejection;
error codes distinguish source guard failures. Initial topology/numerical
validation occurs before splicing, but an error after a prior completed cut
does not roll that cut back. Scratch output cannot be reused as a later input.

`python META/m4/screen-clipper/build_focused.py` compiles four actual TUs and
links `bhunter-screen-clipper.exe` with real round/FP dependencies. All11038
checks pass, including an independent array Sutherland-Hodgman oracle over320
varied convex polygons, both windings/rotating heads, affine UV field checks,
all six bound decisions, all eight scratch slots, inclusive depth/boundary
equality, scratch payload preservation, ring/lifetime/depth/cache contracts,
and malformed/nonfinite guard controls. Removing the implementation causes
actual LNK2001 failures. No original code executes and no game launches occur.
`audit.py` verifies all777 instructions and all exported caller instructions
against the immutable EXE, source hashes, target result and all four reference
hashes. Shared integration/ledger admission is the parent's separate work.
