# Separate pickup constructor00441b80

Read-only review; no implementation or ledger changes. Full contiguous raw340B77-instruction body matches the immutable PE (`boundary.json`). Catalog attribution was unknown before raw review; allocation, resource state, location, registered pickup callbacks and intrusive list insertion establish a genuine engine/application pickup constructor. This is neither CRT/import/compiler machinery nor a shared tail. It has no admitted original-address record.

## Distinct from the existing admitted owner

**`bh_pickup_spawn` is already admitted as whole00441a00**, not a count-zero adapter. Do not relabel or double-count it as00441b80. It implements a variant-aware seven-argument spawn. The missing owner has six original arguments: position,type,remaining,offer,token,fade. It directly selects resources[type], sets variant0 unconditionally, and ALWAYS installs default draw004412f0. It has no resource fallback or special type9 draw selection. Existing spawn's variant0/type9 dispatch selects the special draw, so blindly forwarding all inputs to it is not faithful. A separate new constructor module may reproduce the smaller owner without changing any admitted pickup source/header or their existing variants.

## Exact behavior and closure

1. Call real entity acquire00476f20. Failure returns without resource access, random consumption, room lookup or list mutation. No extra kind, flags, lifetime or scale initialization is present; those remain actual pool-owner behavior. Do not invent pickup-specific kind/flag writes.
2. On success copy resource table base004d19a0[type] to entity+114. Zero resources are not rejected. No alternate table, variant fallback or missing-resource return exists.
3. First actual CRT rand call -> integer-to-x87 -> multiply double004895a0 (hex000000000000e03e =1/131072) -> binary32 phase+f4. Store fade argument to+f8 before phase store. Second rand call -> multiply double00489228 (hex9a9999999999293f =0.0001953125) -> binary32 rotation+fc. Do not preconsume random calls before a successful allocation. There are exactly two calls, in that order.
4. Store type+100, variant0+104, remaining+108, offer+10c. Clamp offer to remaining only when remaining!=0 and remaining<offer, using signed comparisons. Store token+110.
5. Locate real world via admitted0040a8c0 with starting worldNULL, provided position and radius0. Store room+120 and copy position+c..14. Caller supplies actual first-world/count context through the existing source location owner; no fabricated room is permitted.
6. Install admitted tick004415a0 at step+cc, shared RET sentinel refresh+c8 (sourceNULL), admitted query00441570 through real set-query00476e90, and default004412f0 draw at+e8. All callbacks now have real definitions; source-private owner binding at payload+64 is necessary for their actual explicit-context implementation and must outlive the entity. Use actual `bh_pickup_default_step` / rendering binding for the source draw ABI, not a fabricated callback.
7. If a room exists, insert at its pickup list head+12c, initialize room_previous0 and room_next to prior head, update prior head.previous. With no room, original does not initialize the two room-link words; existing source safely clears owned state before publication. Preserve that guard/extension explicitly if adopted, without treating raw recycled bytes as valid pointers.
8. Insert at global pickup head004dfb3c, initialize global_previous0, global_next=oldhead, update oldhead.previous. No callback invocation occurs during insertion. Original function returns void; a source returned entity can be an explicit adapter convenience.

All direct non-CRT definitions are admitted: entity acquire, set-query and world locate. Registered tick/query/default draw are admitted; cleanup004414d0 is admitted and owns unlink/release. CRT rand is excluded from recovery counts. Shared sentinel0047d680 is excluded from independent function counts. No missing gameplay owner blocks this constructor.

## Real source bindings and checks needed before admission

Reuse `BHPickupContext`, actual pool, borrowed resource table, world/room bindings, global head and actual renderer binding. Do not cast guessed global room or resource pointers. Existing cleanup/query/tick retrieve the stored source context, so that binding is required even though original globals did not need it. Context/world/resource/draw owners outlive spawned entities. Any source guards for invalid type/resource range, missing renderer or absent room binding must be explicit and leave pool/RNG/lists untouched when possible. Normal real renderer binding supplies both draw adapters, but the new owner must select default even for type9.

A useful focused fixture should prove every initialized field, two-rand sequence through a seeded actualCRT stream, no consumption on allocation failure, raw resource0 acceptance, signed remaining/offer clamp, default draw for type9, inside/outside room branches, insertion into both nonempty lists and cleanup of head/middle entities through actual admitted cleanup. Bind actual draw owners; execution of the draw callback is not required for the constructor's memory/lifetime checks. No game/UI/input/audio execution is necessary.

The existing bh_pickup.c implementation and its four admitted original owners remain unchanged by this review. No claim of source completion/admission is made for00441b80, and there is still no playable source-built game.
