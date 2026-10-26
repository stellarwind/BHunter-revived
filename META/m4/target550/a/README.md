# Whole player event receiver

`src/bh_player_query.c` restores whole0041d7c0, including every branch in its
17-entry switch and the late internal pickup tail at0041e64e. The complete
3749-byte/1110-instruction body is checked against the immutable EXE by
`capture.py`; raw SHA256 and exact range are in `owners.json`. The switch table,
ten refill quantities and numerical/message constants are separately captured
in `constants.json`. No owned data or shared-tail functions are counted.

Role is game: constructor00421d20 installs this callback at0042208e, and the
body owns actual Hunter fine warnings, cash/outlaw state, protection/damage,
pickup purchases and the ten primary/secondary weapons. Calls to verified CRT
rand/ftol and separately admitted engine/math/platform behavior are dependencies,
not additional recovered owners. Current Ghidra project lacks function metadata
for this body; historical complete M3 pseudocode/assembly is retained together
with a fresh raw instruction capture. No Ghidra mutation was needed.

The source receives explicit borrowed bindings for the actual original globals.
It operates on original player offsets, checks each weapon against the live
source factory registry and original player+258 pointer table, invokes real
ready/cancel owners and allocates real pooled delayed sound cues. Those cues
install the existing whole00437a30 `bh_mission_sound_tick`, with the same source
private lifetime-context binding at payload400. Only original104/180 are written
by this allocation, preserving unrelated recycled payload. No fake vtable,
unrecovered method or constant-success callback is introduced. Verified
0047d680 RET methods/steps are represented by no operation/NULL as elsewhere.

`bh_player_query_bind` is a count-zero source callback adapter with one explicit
live context. Caller must unregister before context/player/pool destruction.
Every original-global equivalent remains live across synchronous callbacks;
the actor successor is loaded after each actual event15 query, and the original
byte counters and signed32 wrapping arithmetic are retained.

`python META/m4/target550/a/build_check.py` freshly compiles both new real
translation units, snapshots the exact existing `build/m0` admitted objects,
and links `build/player-query/bhunter-player-query-check.exe` with real native
libraries. **124 checks pass**, and omitting the new owner fails linking.
Exact commands, source/header hashes, retained-object hashes and test output
are in `standalone-build.json`. This focused closure requires a parent fresh
full build and independent clean integration before admission.

Fixtures use actual entity/weapon/message/sound/math/lifetime/codec/transport
owners. They cover every event class, both pose outputs, byte wrapping, damping,
30k/50k crossings, rights restoration, repair clipping/debit, all ten ammo caps
and complete weapon grants with real ready/cancel effects, delayed cue expiry,
deterministic RNG/damage attribution/protection, radial impulse/radius equality,
contact force/torque/damping/damage, severity suppression, real framed packet
send through the NULL-peer transport, unsupported-event immutability, query-ring
integrity and balanced cleanup. There are no fabricated test implementations.

Limits: these are reconstructed functional source fixtures, not original runtime
differential evidence or instruction matching. PC53 nearest/masked and finite
moderate physical values are required. PC24, exceptional numerical behavior,
audible hardware playback, real peers and player constructor/tick remain open.
Pickup indices must be0..9,20,21 and quantity/price/money nonnegative. A fine
payment requires positive price: the original zero-price branch can divide by
zero, and source rejects that malformed packet before mutation. Callbacks may
fail after prior original effects; no rollback is invented. Source context
errors report missing live bindings or sound/message failures explicitly.

Parent integration: add `src/bh_player_query.c` and `tests/player_query.c`, call
`int __cdecl bh_player_query_checkpoint(unsigned long *)`, and add an omission
link control excluding `bh_player_query`. No shared source/header/build entry,
ledger, checkpoint or progress files were edited by this implementation.
No game/original code, automated gameplay, physical input, UI capture, native
playback, session creation or network transmission was executed.
