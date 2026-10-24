# Unlit BSP model traversal and face production

Complete source candidates cover 00458310-00458421 (96 instructions),
00458430-00458460 (20) and 00458470-004593fe (1184). They are ordinary
independent engine_candidate owners, not CRT/EH/imports/shared tails. Full
caller0045aa60 review proves current-model global004edeb0 points to **model+8**;
therefore internal+0x3c vertices/+0x4c faces/+0x54 nodes/+0x5c leaves/+0x60
images map exactly to admitted BHModel absolute offsets0x44/0x54/0x5c/0x64/0x68.
No competing model layout or guessed global pointer was introduced.

Unlit traversal uses the six **per-model** planes and eye from BHViewCamera.
For every active mask bit it first computes minimum AABB support, choosing
maximum coordinate for negative normal components and minimum otherwise.
Minimum signed distance>=0 rejects, including an exactly tangent box. After
all rejection tests, maximum support<0 clears that bit using original
AND0xfffe/0xfffd/0xfffb/0xfff7/0xffef/0xffdf. These exact masks discard bits16+
when they execute; unknown bits otherwise survive. Full sign-tree instruction
review preserves two sum orders: minimum nx<0/nz>=0, or maximum nx>=0/nz<0,
uses Z+(X+Y); all other cases use X+(Y+Z). Cancellation fixtures with2^60 and a
unit residual prove the different sums change visibility at offset0.5.

Node split side is (nz*eye.z+ny*eye.y)+nx*eye.x+plane. Negative side traverses
front_node first, emits back_leaf chain, then tail-traverses back_node.
Nonnegative side traverses back_node, emits front_leaf chain, then tail-
traverses front_node. The source preserves this painter order with an explicit
bounded task stack rather than replacing BSP recursion with linear face order.
No extra independent face backface test is invented. Original callers push
mask&3 to face/list routines, but complete face instructions never consume that
unused argument; the new context API drops it without changing behavior.

Face production marks its frame **before** checking the image pointer or
(texture_mask & image.flags). Zero texture mask draws nothing; zero image
flags remain filtered even under0xffffffff. Each edge assigns raw UV to its
vertex. A changed raw32bit vertex epoch marks it, clears projected and invokes
the real homogeneous point transform using view.projected. The edge list is
closed into a doubly linked ring and passed to real00452140 starting from the
**last** edge vertex. That source clipper includes projected-UV image mode and
the real general-inverse dependency. A surviving ring goes immediately to the
explicit emit callback with the real image and original uniform color word.

Epochs are integer bits, not numeric float values: original CMP/MOV instructions
read/write dwords at vertex+0x34 and face+8, and wrapper0045ad09 increments the
global with integer INC. Existing BHClipVertex.frame is float-declared because
earlier clippers do not consume it; this implementation accesses its raw token
with memcpy. Tests distinguish0x01000000,0x01000001 and0xffffffff without
float conversion. The separate typed working cache invalidates initial epochs
to0, requires nonzero frame tokens and must reset before token reuse. It copies
the60-byte raw vertex records but immediately clears their serialized links,
never interprets serialized pointer words, and leaves source model arrays
unchanged. Callback rings borrow that cache and clipper scratch; callback must
consume synchronously and may not retain/reenter/mutate the context.

Cache ownership, explicit model/view/clip contexts, native callback binding,
bounded stack and guard/failure propagation are adapters. Validated acyclic
model/node/edge/leaf topology is required; admitted loader already validates it.
The callback represents the dispatch boundary, **not** a claim that complete
0046e390 or software rasterizers are recovered. Native-only root composition
must link the real queue adapter and execute the actual scene before admission.

`python META/m4/model-draw/build_focused.py` compiles seven real TUs and links
`bhunter-model-draw.exe`. All171666 checks pass using an explicitly isolated
synchronous capture callback. Tests include independent eight-corner visibility
and recursive BSP order over80 configurations/all64 masks, actual matrix and
clipper definitions, shared vertex/frame/UV caching, duplicate faces, pre-filter
frame marking, NULL images, correct last-vertex ring head, complete clip
rejection, real projected-UV/general-inverse behavior, callback failure, typed
cache disposal, exact tangent/high-mask and cancellation controls, and full
raw32bit epochs. Removing the owner causes seven real LNK2001 errors.

`audit.py` verifies all1300 owner instructions, full wrapper instructions,
model+8/epoch/mask anchors, source hashes, focused result and four immutable
references. No original code or game launches. `review.json` deliberately keeps
these candidates WIP pending parent real native composition and current full
integration evidence; isolated callback capture is not used to admit a backend.
