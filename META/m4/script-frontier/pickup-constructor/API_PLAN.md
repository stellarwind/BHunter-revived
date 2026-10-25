# Basic pickup constructor implementation plan

Read-only preparation after wagon freeze; no implementation, ledger or admitted source changes. Await parent authorization after wait-two admission. Boundary and raw proof remain CONTRACT.md and boundary.json: whole00441b80,340 bytes/77 instructions; ordinary application pickup construction. Existing bh_pickup_spawn owns00441a00 and must remain unchanged.

## New module and API

New src/bh_pickup_basic.c/.h, tests/pickup_basic.c; module bh_pickup_basic; scenario pickup-basic; checkpoint int bh_pickup_basic_checkpoint(unsigned long *count). Public source API:

    BHEntity *__cdecl bh_pickup_basic_spawn(BHPickupContext *context,
        const BHVec3 *position,long type,long remaining,long offer,
        long token,float fade);

The added context and returned entity are explicit source conveniences; the original six arguments exclude context and original return is void. No archive argument or native device is needed to test this memory/resource-identity constructor. All real direct/callback dependencies are already defined: bh_entity_acquire/set_query, bh_world_locate, bh_pickup_tick/query/cleanup, and bh_pickup_default_step with bh_pickup_render_bind. No VM closure, playback or original-address adapter is needed.

## Source binding and supported-state guards

Borrow actual BHPickupContext unchanged: real pool, first world/count, room bindings, resource table/count, global head, renderer binding. New code asserts entity payload+64==0x124 and 32-bit pointer width, then memcpy stores the persistent context pointer there. The actual admitted bh_pickup_context and cleanup/tick/query must retrieve it; no temporary owner or private alternate getter. BHPickupState is obtained through its actual admitted getter. Store the default callback at payload+4 using memcpy, independent of type9; require the real default binding established by bh_pickup_render_bind. Never delegate to existing variant-aware spawn because its fallback and type9 dispatch differ.

Pre-allocation guards can reject absent context/pool/position/resource table/world bindings, type outside0..9/table extent, absent room-array storage and missing default draw. These guards must not touch pool, lists or RNG. A located world without a matching source room binding is an explicit source-binding failure: release the acquired entity, document that the original assumed its global room structure, and preserve the already-consumed two RNG calls. Do not reject resource0, silently fall back, inspect a second variant, or add original kind/flags/lifetime/scale assignments. State clear after acquire is an explicit safety extension so no-room recycled links are NULL; raw only writes those room-link words when room exists.

## Exact store/side-effect order

Acquire first; on failure return before resource indexing or CRT rand. Store resources[type] even0. First rand is converted with double1/131072, fade stored before phase store; second rand converted with double0.0001953125 into rotation. Use bh_round_float for required binary32 stores. Store type, variant0, remaining, signed-clamped offer (only remaining!=0 and remaining<offer), token. Locate from NULL with radius0, set room and position, actual tick, NULL refresh sentinel, actual query via set-query, and actual default draw. Insert at room pickup head if present, repairing predecessor, then at global head repairing predecessor. No callback is invoked in this insertion sequence.

## Focused fixture plan

Use the actual entity pool, closed one-world AABB/navigation binding and BHPickupRoom, plus a real canonical draw queue. Construct BHPickupDrawContext/BHEntityResourceDrawContext and install both admitted adapters with actual bh_pickup_render_bind; no draw_probe/runtime callback. Callback execution is unnecessary for this constructor and would add unrelated model/atlas dependencies. Assert default callback identity for both type2 and type9, with special callback distinctly bound to actual bh_pickup_special_step. Resource table entries remain caller-provided identities; include raw0 acceptance, avoiding any claim that an arbitrary numeric marker is a loaded model.

Seed actual CRT srand; precompute first/second/third rand, reseed, construct and assert phase/rotation exact binary32 reference plus rand()==third. Exercise signed clamp with positive remaining<offer, remaining0, negative remaining and negative offer. Verify fade/token/type/variant/resource, exact position, room/global heads and previous/next links, callback identities, owner pointer returned by actual getter, and pool-owned origin/lifetime/scale/kind untouched by the constructor. Test successful allocation from a deliberately recycled nonzero payload, documenting cleared no-room link safety.

Create at least three inside-room pickups to test nonempty insertion and actual admitted cleanup of middle/head/tail in both lists. Create outside-world pickup and verify no room list link while global list remains correct. Event20 through actual query must remove query registration and schedule actual cleanup; invoke actual entity stepping to complete release. A forced phase0/fade0.1 pickup with no actors can exercise actual tick timeout cleanup without test gameplay callbacks or sound.

Allocation failure control: a test-only allocator owns real malloc/free storage but can fail the actual pool growth allocation after initial pool setup. Fill the initial pool with a real entity, fail the next growth, and assert constructor returnsNULL, actual pool used/capacity/list heads unchanged, and actual CRT stream next value unconsumed. This is isolated allocation-failure instrumentation, not a replacement gameplay dependency or fake entity acquire. Release the filler and dispose actual pool. Precondition-failure controls similarly check RNG/pool unchanged. Add expected-negative link without bh_pickup_basic.obj proving the new actual symbol is required.

Compile the real new TU/test with pinned VC5/SP3, link actual current production definitions, run focused clean checks, record exact source/dependency hashes and missing-module control. All entities are cleaned before queue/context disposal. No UI/game/original code, physical input, native playback or archive mount. Parent owns full real+clean integration and admission; no existing pickup source/header edits are planned.
