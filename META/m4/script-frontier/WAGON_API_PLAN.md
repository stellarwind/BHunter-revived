# Proposed actual source API and component ownership

Five-owner implementation has not begun. Proposed new `bh_rail_wagon.c/.h`,
`tests/rail_wagon.c`, scenario `rail-wagon`, checkpoint
`bh_rail_wagon_checkpoint(const char *archive,unsigned long *count)`.

Whole behavior APIs:

- `BHEntity *bh_rail_wagon_spawn(BHRailWagonContext*,position,angles)`004470c0.
- `void bh_rail_wagon_draw(BHEntity*)`00446160.
- `void bh_rail_wagon_tick(BHEntity*)`004462f0.
- `void bh_rail_wagon_follow(BHEntity*)`00446b10.
- `void bh_rail_wagon_query(BHEntity*,void *packet)`00446e00.

The last four are actual fixed source entity callbacks with persistent context
binding, not caller-provided runtime substitutes. A private owner pointer must
be stored in an explicitly asserted source-owned region outside all original
wagon fields, following existing entity modules. No original code address,
guessed global/vtable or temporary copied context is installed.

BHRailWagonContext borrows BHRailPaths, BHEntityPool, first BHWorldGeometry/ring
count, actual BHPickupRoom array(count) for shared `actors` heads, canonical
BHDrawQueue, actual render resource handle/descriptor identity, BHSoundQueue and
unsignedlong sound resource ID. The resource callbacks are engine-provided
source definitions: draw invokes `bh_draw_submit_resource(queue,0,model_handle,
pose,angles,1)`; sound uses actual request/update/stop. The constructor's world
queries use `bh_world_locate` with the actual first/ring/hint bindings.

Preserve shared actor prefix offsets. Existing pickup callbacks read actor next
from `entity->payload+16`(original+f4) and class from original+fc. A wagon state
must preserve next/previous at+f4/+f8 and class9 at+fc rather than moving those
fields into an incompatible new layout. New code may declare and assert the
observed state beginning+e8, with draw pointer at+e8, two retained/preserved words
at+ec/+f0, actor next/previous, class/subtype, sound instance/cooldown/resource,
secondary-1handle, interpolation numerator/denominator/speed/spacing, retained
word+124, segment pointer+128, leader pointer+12c, direction+130 and state
fields+134/+138. It may use explicit source components for Euler/basis/world
ownership while retaining all actually shared prefix/links used by admitted
definitions. Do not cast the unrelated `BHSceneLevel` reader to a world entity.

Original resource load identities are proved in startup owner004474c0:
RCWAGON.BSP through admitted00458140 to004d0478, and sound\\metro.wav through
admitted0047b950 to004d1a64. Current immutable archive can provide those actual
assets to focused source tests using BHModelContext/BHTextureContext and the
whole sound loader. A clean fixture may use owned bounded model/WAV data through
those actual source loaders; do not fake a model vtable or DirectSound buffer.
Model lookup at command execution uses actual BHDrawResource descriptors, while
command-only checks can prove loaded model identity and pose without drawing.

Useful full-family integration order: establish stable linked rail chain and
real world bounds/rooms; create actual model/sound owners and resource IDs;
initialize canonical queue and entity pool; spawn two actual wagons; dispatch
real draw/step/query callbacks; verify command and deferred voice state,
coupling/follower transition, pose query, world migration and deletion cleanup;
then dispose entities before rail/world/model/sound owners. No actual Play,
visible renderer, game loop, physical input or UI capture is needed.

Safety guards should reject unlinked/open/dead rail pointers before walking
fixed10/21 steps, invalid room membership, zero/invalid divisor state and
foreign/recycled leader entities instead of dereferencing stale source storage.
These should be stated as source contracts, not silent changes to valid closed
rail behavior. Original movement branches, interpolation spill locations,
negative/zero comparison behavior and angle-wrap fmod decisions still need
line-by-line raw audit during implementation. The read-only closure review is
not evidence that these five have already been implemented or validated.
