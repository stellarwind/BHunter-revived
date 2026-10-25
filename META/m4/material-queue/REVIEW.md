# Image policy command family

Canonical LF refresh: standalone-prior-candidate.json and
source-hashes-prior-candidate.json preserve the previous focused candidate's
evidence. Current standalone.json is a fresh compile/link/run of frozen current
source, with100 checks/zero failures; source-hashes.json pins current bytes,
including the current shared draw-queue header. No prior instruction-match
evidence is transferred. Global real/clean builds remain separate parent evidence.

2026-10-25. Static review and source restoration; zero game executions.

Before decompilation, retained original boundaries/call edges classify these
entries as render engine candidates with unresolved proprietary/library
provenance: 00478060/00477fa0/0044f510 (queued image fields),
00478730/00477760/00457590 (queued textured quadrilateral), and
00478930/00477f20/0045cf30 (queued two-word render state). Each is a separate
contiguous RET-ended body, not an import, verified CRT, compiler helper or
shared tail. Original queue callback assignments establish consumers; call
edges alone omit these callback relationships. `export_static.py` exports
complete current Ghidra listings and checks every byte against the pinned PE.

## Selected complete source closure

`src/bh_material_queue.c` restores 00478060 and 00477fa0 with existing real
0044f510 `bh_texture_image_set_words` in `src/bh_texture_policy.c`. The producer
copies an image identity and seven untouched words to command +04..+20;
floating bit patterns receive no conversions, clamps or arithmetic. The
original consumer calls the setter then reads +50 next. Original bucket
mapping is 0..8 -> 1,2,3,4,5,0,6,7,8; all other values map to header zero.
The original full queue silently drops the update. Source reports zero while
preserving that behavior. Ordered header-zero updates precede numbered draw
buckets, matching the actual player render call with bucket 5.

The public name 'material' describes the queued image policy family only.
The setter itself preserves raw words. Existing recovered clipper00452140
establishes their observable consumer: +54 enables projected UV, and six
binary32 terms at +58..6c compute affine U/V from world X/Z after inverse
camera transformation. This is not a claim that the player's particular
animated texture effect has been fully explained.

Source identities resolve through an explicit borrowed `BHDrawImageLookup`.
Images and binding storage must remain live through synchronous consumption;
lookup must not mutate the command, and next output must not alias it.
Unknown handles fail without dereferencing an arbitrary pointer, publishing
the command next link. Missing binding/kind/output and malformed queue guards
are source adapter behavior, not original malformed-pointer behavior.
The consumer invokes real owning source logic; it does not call original code
or a test substitute for the setter. Focused fixtures bind actual source
BHTextureImage instances; these are explicit memory fixtures, not restored
game startup or actual archive/native graphics evidence.

`check_standalone.py` compiles all eight real TUs, links named source-only
`build/material-queue/material_queue.exe`, and executes its relevant checks.
Its JSON preserves exact commands/results. All100 checks pass, including
composition through real00452140 clipper: queue installs selected image policy,
four vertices obtain exactly expected projected UVs, and a second queued disable
preserves authored UVs. No renderer callback is needed to observe this effect.
Parent integration/checkpoint
evidence remains necessary for admission. No instruction-match claim.

## Other reviewed candidates

00478730 installs callback00477760. It copies 19 payload words including
the nonsequential param15/16 placement (+3c/+38); consumer reorders coordinates
and narrows +3c flags to byte before calling 2181-byte00457590. This larger
owner transforms/clips textured quad data and calls00451140,00452140 and
0046e390 plus recovered math and CRT conversion. It is not a closed producer
assignment without those real consumers and numerical contracts.

00478930 always inserts at header8;00477f20 calls0045cf30, which stores
globals004edec0/004edec4. Real consumers0045cf50 and0045d690 read this state;
0045cf50 also updates it. Restoring only a two-word setter would leave the
observable state transition owner unfinished. This family was not implemented.

00478f60 full-image/UV rectangle is already restored by
`bh_pickup_rect_queue`, `bh_pickup_rect_execute`, `bh_pickup_rect_draw` in
`src/bh_pickup_draw.c`; do not count it as newly recovered here.

## Parent integration

Append `BH_DRAW_MATERIAL_WORDS` to BHDrawKind, preserving existing values.
Add src/bh_material_queue.c and tests/material_queue.c to normal build sources.
Declare/call `bh_material_queue_checkpoint(unsigned long *)` from checkpoint
dispatch under `material-queue`. The real source dependency closure is queue
allocation/begin/dispose and image-word setter; no new runtime library beyond
normal system/CRT bindings. Full scene owner remains independently missing.
