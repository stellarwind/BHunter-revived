# Recursive room radar connectors:00417f30 /00417f70

Both boundaries were classified before decompilation as complete ordinary
engine-candidate owners, with unresolved historical authorship. Neither is an
import, CRT/library match, compiler helper or independently counted shared tail.
`owners.json`, complete instruction JSON/TSV and `export_review.py` pin every
body byte to immutable reference SHA256 c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1.
Reset is61B/21instructions and render3890B/1140instructions. No original code
was loaded, mapped or executed. This is functional source, not a byte match.

## Actual ownership and source layout

The historical frontier name “attachment” denotes world-room portal topology,
not a newly allocated attachment/model hierarchy. The original entity's bounds
at+1b0/+1bc, query flags+114, six60-byte portals at+1cc, neighbor at portal+38
(entity+204) and shared traversal mark+fc map to the already restored
`BHWorldGeometry`/`BHPortal` components. `BHAttachmentNode` is an alias; the only
new owning state is `BHWorldGeometry.radar_visited`, explicitly a source field.
The source world record is not claimed to reproduce the full original ABI.
Compile-time checks retain real portal60B/neighbor56B and navigation-first
layout; casting neighbor component pointers to owning world nodes is therefore
valid under this borrowed-world contract. No fake globals/vtables or opaque
original pointers are introduced.

Portal plane is a signed integer. Renderer reads overlap[0..3], original
portal+18..24, rather than aperture or scratch. Directions0/4 select planeX and
overlap midpointZ;1/5 select both overlap midpoints;2/6 select midpointX/planeZ.
Invalid directions3/>6 are source guard errors, not original recovered behavior.

`BHAttachmentContext` binds actual draw queue, the mutable HUD-produced radar
frame, player position/yaw and viewport width. Image handles are borrowed:
default004f00b8(graph), bit200004f006c(graph2), bit400004f0070(graph3),
bit800004f0074(graph4), with200 then400 then800 precedence. Frame maps radar
origin004df1b8, axisX004df218, axisY004df228, normal004df1a8, flat004d1a9c,
screen radius004decec and integer center004ce2a8/004ce444. HUD owns these
placements; renderer does not guess startup state or allocate images.

## Behavior and numerical contract

Reset returns behind an already-clear parent, clears before visiting all six
links in slot order, and never changes links/resources. Cycles/shared nodes
are valid. Render marks before segment emission and also suppresses revisits.
Exactly two live neighbors produce one portal-to-portal segment, followed by
first recursion and second tail recursion. Other degrees produce room-center
to portal connectors in slot order. An outside center is clipped in place and
reused by later slots. Clipping interpolates endpoint radial distances to400;
it is not a segment-circle intersection. Thus two outside endpoints cull even
when the geometric segment crosses the circle. Strict post-clip radius<400
controls visits; fade zero overwrites second endpoint radius400. In the
two-neighbor case, fade-zero still permits the first visit.

Height is signed distance from playerY to roomY interval times.02. Absolute
height controls clamped fade, but signed height controls width; rooms above the
player widen and rooms below narrow. Width is.01 in world mode and signed
viewport_width*.0078125 in flat mode. World coordinate scale.00025, flat scale
.0025*screen_radius, endpoint radius fade1-(rA+rB)*.00125. Angles are
atan2(deltaX,deltaZ)-yaw; no normalization/wrapping is inserted. Exact constants
are in `constants.json`.

PC53/nearest/masked moderate finite behavior is retained with explicit stores:
endpoint radius uses retained delta times stored float; clipping firstX uses
retained double division weights while finalZ reloads stored float weights;
midpoint radius retains midpointZ and one midpointX factor; world projection
retains double rotated offsets; flatX uses stored radial offset while flatY
retains it. Audit independently traced these x87 sites. PC24, huge arguments,
NaN/exception semantics and original software rasterization remain unvalidated.
Source guards report invalid bindings/nonfinite conversions/topology; prior
queue entries and visited marks remain on failure, allowing caller reset.

## Linked validation

`build_check.py` compiles the real `src/bh_attachment.c` translation unit and
real draw-queue/screen-quad/screen-clip/fp/round dependencies, links
`bhunter-attachment-check.exe`, then executes `tests/attachment.c` checkpoint.
`standalone-build.json` records commands, exact source hashes and actual result:
109checks,0failures. This includes six directions, all image priorities,
degree0/1/2/6, cycles/shared nodes/reset short circuit, strict radial equality,
inside/outside clipping and mutated center reuse, signed height/width,
first-versus-second fade behavior, full-queue traversal and source errors.

Five generated precision cases in `tests/attachment_cases.h` check exact queue
words against a static instruction-derived PC53 arithmetic oracle. The three
deterministically searched cases have explicit counterexamples for rounded-only
clipping, endpoint and midpoint simplifications. Two fixed cases cover both
off-axis clip directions and yaw. `static-cases.json` retains intermediate
values and incorrect-simplification results. This is static analytical evidence,
not an original-runtime differential claim. Global integration/native composition
is tracked by the parent HUD/attachment evidence; this file does not invent its
results or transfer evidence to another source variant.
