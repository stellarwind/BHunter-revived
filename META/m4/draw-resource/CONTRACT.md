# 0045adf0 resource draw dispatch

Boundary: `0045adf0–0045b1b4`, 965 bytes / 281 decoded instructions in the
saved `META/m4/sprite-world/original/0045adf0.tsv` and `.c.txt`. The four-way
jump table, single outside call from `004775f0`, and self-call inside tag 1
classify it as an engine-candidate renderer resource dispatcher, not an entity
traversal owner. It is no CRT, import, EH helper, or shared tail. Engine versus
game provenance remains unresolved. The jump table belongs to this body.

Original x86 descriptor word `+0` is the tag. Tag 0 passes the descriptor
address directly to `0045aa60` (model draw); its radius is at `+4`. The source
binds this existing owner through a borrowed `BHModelInstanceContext` pointer.
Tag 1 has radius `+4`, signed count `+8`, five float threshold slots starting
`+0c`, and selected child pointer at `+20 + 4*i`. The fixed child base limits
the valid threshold count to five. It compares the depth against near minus
radius while the computed x87 value remains live after `FST float`; far plus
radius and thresholds consume the stored float. The first threshold `>=` depth
selects one child; falling past the last makes no draw. All six double pose
arguments and the final float scalar are forwarded unchanged to the child.

Tag 2 passes image pointer `+18`, width/height/angle floats `+8/+c/+10`,
low byte of flags `+14`, float-cast position, and final scalar to `00455960`.
Tag 3 has local endpoint `+8/+c/+10`, start/end widths `+14/+18`, common
flags `+1c`, image `+20`. It copies the identity transform, rotates locally
Y/X/Z with float-cast angle arguments, transforms the endpoint, and adds it to
the double origin before final float stores. The camera projected transform
maps both world points to two entries of the original `0x54` path pool. Entries
carry flags at `+44`, scalar at `+48`, width at `+4c`, and float index at
`+50`; old tail, first new entry, and second new entry form one linked path.
It then calls `004569b0`, the oriented sprite owner.

`BHDrawResource` is an explicit native descriptor, not a serialized x86
layout. All descriptor, image, model, camera, sprite context, path and emitter
bindings are borrowed and must remain valid through the synchronous call.
The source adds finite-argument, fixed-count, path-capacity, missing-binding
and recursion-depth guards. Unknown tags, including negative signed bit
patterns, take the unsigned jump-table default and return without drawing.
The original function has no error return. The source
returns success/failure for integration and carries diagnostic dispatch/cull
counters. It does not claim entity list traversal, descriptor file parsing, or
original binary execution. PC53/nearest/masked is the established numeric
profile; 24-bit runtime precision and large-angle trigonometry remain open.

Dependency definitions: `bh_model_instance_draw`, `bh_sprite_billboard_draw`,
`bh_sprite_oriented_draw`, `bh_mat34_rotate_y/x/z`, and
`bh_mat34_transform_point`. Model context and sprite contexts are owned by
their caller. The oriented path pool owns no image; the resource borrows its
image until the synchronous emitter returns. Validation/admission status is
recorded separately in `META/function-status.json` by the integration owner.
