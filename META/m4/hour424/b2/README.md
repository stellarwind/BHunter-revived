# Complete projectile query and deferred sound cleanup

Whole00440030 (31 bytes,11 instructions,end0044004f) and00440810 (205 bytes,
60 instructions,end004408dd) restore one closed projectile lifetime family.
Query event20 installs the actual cleanup address at00440837, unregisters
the query through actual00476e90 and clears refresh to the verified inactive
RET identity (source NULL). The subsequent whole cleanup always calls real
sound Stop with original118 handle then actual entity release. Its ignored
Stop result never blocks release, including -1/stale handles. Neither owner
changes retained gameplay payload or forcibly invalidates that stored handle.

Query event17 independently clears12c and130 references when either equals
packet+4, so both equal references clear. Event18 first compares packet+4
against identity11c. A match writes packet+4=-1 before examining kind+8;
nonkind3 makes no entity changes. Kind3 applies the exact original ordered
countdown clamp (binary64.2 threshold, literal binary32.2 store), copies all
three position words from packet+10 and writes identity1. Quiet NaN countdown
retains its class and is not clamped. Other events and mismatched IDs are
complete no-effect branches. No actual network operation occurs here.

These are ordinary proprietary engine/game behavior owners, with historical
authorship unresolved. Reviewed full entry/return boundaries exclude CRT,
compiler helpers and imports. The tempting98-byte candidate00437770 was
rejected: it enters using live ESI and returns through someone else's saved
EDI/ESI/EBP/EBX and24-byte stack frame, so it is an interior cleanup tail,
not an independently callable gameplay owner. Its raw stream remains review
evidence and adds no count.

Explicit count-zero source binding borrows actual pool/sound context,
initializes only verified fields, preserves prior step/refresh, installs
the real complete query, and stores private context/generation at payload
468/472. Existing ownership/lifetime/profile contracts remain required.
Source rejects dead/foreign/recycled current entities before side effects.
No source constructor, projectile tick, visual callback, vtable or global is
invented. Event20 itself creates the actual deferred cleanup transition;
the test invokes it through the complete real entity step dispatcher.

`python META/m4/hour424/b2/build_check.py` compiles actual new and existing
source translation units with pinned VC5 SP3 and stock LINK, links named
`build/projectile-lifetime/bhunter-projectile-lifetime-check.exe`, passes37
checks and requires genuine missing-module link failure. Actual source
streaming resource/voice allocations prove Stop affects only the selected
handle; full payload/position retention, stale-handle failure, real deferred
dispatcher teardown, packet acknowledgements, kind gates, exact clamp cases,
NaN class, independent dual-reference removal and ignored events are checked.
No dummy callback, fake sound backend, original-backed adapter or original
code is supplied.

Parent current real/clean integration is required for admission. Whole
physics004124d0 was triaged and deliberately not partially implemented; its
typed mixed-candidate/global-state orchestration remains documented in
PHYSICS_REVIEW.md. No game, automated gameplay, input, UI capture, audioPlay
or network execution occurred. Full projectile construction/movement/rendering,
PC24, full VM/world startup and a playable source game remain open; M3 is
skipped and M4 remains incomplete.
