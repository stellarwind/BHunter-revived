# Whole00441b80 basic pickup constructor, read-only contract

340bytes/77instructions, complete rawRET through00441cd3, fresh decompiler and
PE verification retained. Ordinary game entity/resource behavior, not CRT or
shared tail; not yet admitted. Its six original arguments are position,type,
remaining,offer,token,fade. It acquires an actual entity, selects resource[type],
consumes exactly two CRT rand calls, sets phase/fade/rotation/type, forces
variant+104=0, clamps offer down to remaining only when remaining!=0 and less,
locates world, copies position, installs admitted pickup tick/query and ALWAYS
default draw004412f0, then prepends room-pickup and global-pickup lists.

All native/runtime dependencies and installed callbacks are already restored.
The one-byte RET callback is represented by NULL under the admitted entity
contract and is not an additional owner. Root owns implementation after prior
families close; no admitted source is modified in this scout.

The existing `bh_pickup_spawn` is already admitted00441a00, not an uncounted
generic adapter. It supports a variant and selects a special draw for variant0
type9, whereas00441b80 always uses the default. Do not admit it unchanged or
wrap a stack copy of its context: lifetime and special-draw policy would differ.
Existing actual source-private context binding at entity+124 (payload+64) may
be written by a separate new module with an offsetof/size assertion and verified
through real `bh_pickup_context`/cleanup. This is established source ownership,
not a claim about an original global or invented vtable. Borrow the persistent
BHPickupContext and its actual resource/room/global-head state; retain all
existing pickup source/header variants unchanged unless separately coordinated.

Positive checks should discriminate type9 default draw from the special draw,
two RNG calls and ordering, remaining/offer clamp, room/global reciprocal links,
no allocation/no RNG on real pool failure, source-default/no-op refresh,
actual query/tick/draw and real cleanup/unlink. Resource fixtures must bind real
draw owners rather than fake callbacks. No renderer execution or native audio
play is required to verify its complete lifecycle in an isolated source pool.
