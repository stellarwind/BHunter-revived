# Full-image world-plane quad and queued consumer

The parent HUD/room-connector family emits real oriented commands through
004785f0. Its consumer is the complete78-byte/37-instruction **00477710**;
00477720 is inside that body and must not be counted independently. It calls
the complete2141-byte/547-instruction00456d30 full-image arbitrary-plane quad.
Both are engine candidates with unresolved historical authorship; ordinary
geometry/queue ownership, not import/CRT/compiler/shared-tail boundaries.
`export_review.py` verifies every instruction byte against the immutable PE,
and `owners.json` records whole-body SHA256. No original code is executed.

The new real `src/bh_plane_quad.c` TU owns both symbols. The quad borrows the
existing explicit `BHSpriteQuadContext` camera, clip, directional/point lights,
base color and synchronous emitter, and calls actual recovered vector math,
matrix transform, polygon clip and world-light owners. This boundary differs
from00457590 by its whole-image UV domain. Its source body is separate, leaving
the admitted arbitrary-UV variant unchanged; independent whole-body review
verified the equivalent geometry/lighting dataflow and differing UV setup.
No claim of source/raw byte matching is made for either variant.

Observed contract: negate normal, reject strictly positive camera side unless
flags bit8, normalize normal/up, Gram-Schmidt, cross basis, optional in-plane
rotation, half-width/height scaling, four world corners, clip/project and emit.
Bit1/2 flip U/V, bit4 transposes; backface reverses ring while retaining first
corner. Zero/parallel bases retain original degenerate geometry. Modulation is
the real15th argument, missed by Ghidra's signature. No-light shade truncates
modulation*-255 before unsigned32 base-color arithmetic. Point and directional
paths preserve normal orientation, double modulation and two separate integer
conversions per directional channel before clamp. Caller owns moderate finite
PC53/nearest/masked environment; huge/exceptional/PC24 behavior is unvalidated.

Raw stores require A.x/y's origin-right intermediate to spill to float; A.z and
B/C retain the sums, while D.xyz reload stored origin+right before adding up.
Rotation retains sin for one temporary and cos for rightX, with stored values
for other components; Z basis remains retained until scale. These were checked
against this owner's complete raw instructions, not assumed from00457590.
An independent reviewer confirmed these sites and UV/lighting/control behavior.

Consumer maps actual BH_DRAW_ORIENTED payload up/normal into the original
reordered arguments, truncates flags to one byte, forwards width/height/angle
and modulation, calls full-image draw, then reads the current next link.
An image lookup is an explicit source identity binding, not an original image
pointer/vtable. Lookup/draw errors publish next and report source failure;
invalid command kind rejects. All borrowed objects survive queue consumption.

`build_check.py` compiles real source/dependency TUs, links named
`bhunter-plane-quad-check.exe` with real definitions and runs the source test.
`standalone-build.json`:690checks,0failures, covering all flag combinations,
front/back winding, Gram-Schmidt and rotation, side/near/far clipping, real
point/directional light contributions and channel conversion, queue payload
mapping, missing modulation argument, post-draw next-link read, errors, and six
static cancellation-sensitive PC53 corner cases. The generator uses this
owner's verified FST dataflow and no game execution; the golden cases detect
all12 corner component spill/retention simplifications. These are static
analytical fixtures, not runtime differential or byte-match evidence. Native
composition/full integration is recorded separately by the parent block.
