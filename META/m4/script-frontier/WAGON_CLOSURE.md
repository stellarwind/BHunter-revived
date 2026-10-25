# Whole rail-wagon runtime closure: five candidate owners

Read-only source planning, no implementation or source admission. The complete
raw constructor and four callback bodies are retained and PE-verified; see
`raw-review.json`. These are ordinary application entity/rail behavior, not CRT,
imports, compiler helpers or independent shared tails. No VM00443ff0 callback is
installed or required by this family. The data family is separately implemented
and must be admitted before assigning this consumer.

|Owner|Bytes/instructions|Role|
|---|---|---|
|004470c0|1014/284|Locate world and nearest qualifying segment; acquire/configure wagon, announce event19 and install real query.|
|00446160|395/125|Interpolate current segment pose, submit actual render resource, maintain deferred moving sound.|
|004462f0|2073/574|Independent wagon step, segment transitions, world-room relinking, basis/angle updates.|
|00446b10|746/219|Attached/following wagon step selected by coupling query.|
|00446e00|660/207|Position query, detach/delete and wagon coupling query.|

Total4888bytes/1409instructions. Callback00446b10 is not installed by the
constructor directly:00446e00 installs it on either participant during coupling
(raw00446ee1/00446f17). Omitting that owner would be a partial family. All five
catalog bodies end inRET; the query's jump tables begin00447094 and are data,
not part of its660-byte executable owner.

At the read-only scout checkpoint Ghidra had a function for constructor004470c0 but not the four callbacks.
Fresh decompile errors are retained rather than fabricated text. Future source
implementation may establish the four already reviewed boundaries in the
analysis database before fresh decompilation; this read-only scout made no
database change. No full-body pseudocode is claimed for those callbacks yet.

##External definitions and state

All direct non-CRT callees are restored: vector cross/normalize/normalize-to,
direction transform, local/world rotations, world locate0040a8c0, entity
acquire/set-query/release/query-dispatch, render resource submission00478170,
sound request0047bd40/update0047c050/stop0047c140. Actual source APIs and layouts
must be reused, not original function addresses or copied dummy interfaces.
The only apparently unknown direct math helper0047e3ca is **__CIfmod**, not atan2
or a game owner. `fmod-crt.json` compares its complete10-byte intrinsic wrapper
against the actual symbol offset in pinned VC5 SP3 LIBC, with actual COFF
relocation masks, verified precision-wrapper positive control and altered-byte
negative control. Reuse CRT fmod; count no additional engine function.

Required source context borrows the admitted stable BHRailPaths allocation,
actual entity pool, world geometry ring and per-room actor-list heads, resource
handle for actual RCWAGON render model, live sound resource/queue and renderer
submission context. Original globals004d0478 (render handle) and004d1a64 (sound
resource) must become actual source identities loaded by admitted owners.
No native audio Play or displayed rendering is needed for focused checks;
inactive listener keeps sound requests/updates source-deferred.

The wagon lives in the room **actor** ring (head originalroom+128), distinct from
the pickup ring(originalroom+12c). `BHPickupRoom.actors` is an established actual
source binding for that shared head; do not invent a parallel room-head global
when composing with real actors. Entity+f4/f8 are wagon next/previous actor links.
Explicit wagon state may retain all original scalar/vector behavior and source-
private stable context ownership; do not reinterpret unrelated BHScene reader
storage as the whole original entity.

Constructor inputs are borrowed position and Euler rotation vectors. It locates
the real world first, scans segments with projection fraction in[0,1], and picks
strictly lower normalized perpendicular distance (first tie survives). Failed
world/segment/allocation returns without installing a partial fake entity.
It sets flags|0x100, actor class9, copies orientation/position, prepends to the
room actor ring and initializes interpolation numerator+114, denominator+118=1,
speed+11c=30, selected segment+128, direction flag+130 and self reference+12c.
It sets sound instance+104=-1, sound resource+10c, sound cooldown+108=0 and a
second -1 field+110. Exact other initial fields require the retained full raw
body, not this abbreviated planning list. Euler rotations compose actual
admitted rotations before basis-vector transforms. Constructor announces event19
through actual query dispatch before installing its own query.

Draw00446160 interpolates pose according to direction and ratio(numerator/
denominator), submits actual model with alpha1, sets a two-tick sound cooldown,
derives velocity and pitch from live segment/speed data, and requests a sound
only when current instance==-1; otherwise updates that real instance. Sound
queue ownership and instance lifetime must survive until stopped/deleted.

Query jump table maps event3 to computed position, event4 andevent20 to room
unlink/soundstop/entityrelease, event19 to coupling, others to no-op return.
Coupling compares direction, walks ten segments in one direction then at most
twenty-one back to find the other wagon, replaces the current leader when new signed spacing is greater or equal (equal spacing overwrites),
updates parent/reference+12c and spacing+120 and installs actual follower step.
It is not permissible to replace event19 or follower behavior with a callback
placeholder. Step code also transitions current world actor-ring membership;
exact FP spills, unordered comparisons, fixed timestep and fmod angle wrapping
still need complete source-level audit at implementation.

##Useful checks and lifetime limits

Use real admitted rail geometry, world bounds and entity dispatch in an owned
isolated fixture. Nearest-segment/tie/projection/direction controls, open/closed
chain edge guards, one independent step and each transition direction, real
model command production, sound-request/update/stop lifetime without playback,
two actual wagons coupling into follower step, room migration, event3 pose and
event4/20 unlink/release form a meaningful whole-family test. A real archive
rail loop may be reused as data without game execution. Do not install test
queries as production defaults or use fabricated VM callbacks.

Rail data may not be relinked/reordered/disposed while live wagons borrow its
segment pointers. Pool, room heads, world components, renderer and sound owners
must outlive every wagon callback. Native hardware/gameplay and universal
PC24/extreme-angle fidelity remain outside the supported bounded source checks.

##Other constructor candidates

Most other VM-direct constructors are not small closed assignments:00402e50
installs five missing effect callbacks,0043bec0 installs seven missing behavior
callbacks and its catalog boundary even omits aRET at0043c200, and debris
constructors0043d2c0/0043ddc0 need missing draw/destruction trees plus additional
model behavior. Preserve those as frontiers, never assign/count their wrapper
alone with guessed callbacks.00441b80 is the separate bounded basic-pickup
constructor; see PICKUP_BASIC_CONTRACT.md before reusing existing broader spawn.
