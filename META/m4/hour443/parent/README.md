# Physics effect block: count-zero whole-owner dependency

This source binding supplies the effects block inside whole004124d0; it is not
an independent original function and adds no recovered-owner count. Original
3916-byte body fingerprints and exact instruction anchors/constants are retained
in raw-review.json. Parent and family_c independently reviewed the block.

BHPhysicsEffects borrows actual contact effect, sound queue and transport owners.
Resource IDs, sender, peer count and current-world index are caller bindings.
It executes real impact/spark allocation and callbacks, sound requests, position
quantization, envelope/checksum and the real sender. It does not tick sound or
create a provider/session. A NULL native peer retains S_FALSE with zero Send calls.
The caller must invoke this block before whole physics event2 delivery.

Special surface bit2 takes priority over4, creates the real16-unit quad or12-unit
rays, and queues its sound only after successful allocation. No special packet
or spark path executes. Normal binary32 level L in[0,1] queues sound only above
.5: volume=(L-.25)*.7 and stored binary32 pitch=.9-L*(-.2). For peer_count>1,
current-world/contact-point position is appended before four bytes: sound ID,
trunc(L*128),trunc(stored_pitch*64),trunc(L*3); quiet uses ff,00,40,last byte.
NULL world suppresses that packet only. The packet is finalized and synchronously
sent before sparks; existing actual writer/sender owners supply these operations.

Spark count is trunc(L*3). Each attempt first consumes one outer CRT random value
for lifetime=float(rand/131072-L*(-.0625)-(-.5)), speed1, then calls the real spark
constructor. A successful constructor consumes eight more draws; allocation
failure consumes none of those eight. Diagnostic failures retain prior effects
and continue later original requests. The packet bytes and counters are source
observability extensions, not original globals or a fake network transport.

The real-TU source fixture passes66 checks and a missing-module LNK2001 control.
It checks independently specified packet bytes/checksum, exact .5 threshold,
contact-point/zero-velocity sound state,27successful versus3failed random draws,
outer lifetime values, special material priority and allocation-failure sound
gating. All owners are real; no dummy emitter, effect callback or COM object is
provided. New code is limited to moderate finite input in the existing PC53
nearest/masked profile; PC24, NaN payloads and native successful networking remain
unvalidated. Whole physics and full real/clean integration are required before
admitting this count-zero dependency. No game/original code, input/UI capture,
new Play calls or native networking occurs in these checks.
