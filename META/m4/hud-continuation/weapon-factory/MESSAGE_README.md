# Shared message queue and empty-ammo weapon methods

`export_message_review.py` verifies five complete RET-ended bodies and the
literal text against the immutable PE SHA256
`c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
`message-review.json` records whole body boundaries, bytes, instruction counts,
SHA256, direct calls and the panel clamp constant. The owners are game behavior
with unresolved historical engine authorship, not CRT/import/compiler helpers
or shared tails. No original code was executed.

Shared `00442a30` (292 bytes/103 instructions) has no direct calls. It writes
the eight-slot, 512-byte message ring at `004d0488`, the append-only 8192-byte
text arena at `004ce458..004d0457`, the ring count/start/last index and display
counters. It returns zero on arena exhaustion **after** ring/display mutation.
On success it reads the current panel entity's live `+cc` method identity and,
only for verified `00442950`, clamps a strictly greater-than-3 float at `+e8`
to 3.0. `src/bh_messages.c/.h` owns the source equivalents of these states. The
source panel owner now binds the actual pooled entity and live step identity;
the isolated `message_build.py` harness still supplies an explicit stand-in
binding for this earlier focused test. Missing binding is reported after
append, rather than fabricating a panel callback. Source safety
rejects text longer than 511 characters before the original unbounded ring
copy could overwrite adjacent memory. Normal game strings, including this
weapon warning, are within the verified range. The arena display consumer is
now owned by `src/bh_message_panel.c`, validated separately in
`PANEL_README.md` and `panel-result.json`.

`0042f730` (99 bytes, index0/1), `004309d0` (94 bytes, index2) and
`00432560` (104 bytes, index4) are complete `+124` weapon methods. Each first
mutates authoritative weapon state, then if signed ammo `+10c` equals zero,
queues actual loaded shift sound `004cda10` with .8 level/1.5 pitch and live
player position `+0c`/velocity `+48`. It checks shared warning flag `004dfb24`
**after** sound allocation, calls the message owner once with the exact 107-byte
PE text if clear, then sets the flag to one even if enqueue reports failure.
Index0/1 clears only bit0 of `+e8`; index2 clears all of `+e8`; index4 clears
all of `+e8` and sets latch `+108` to one before the ammo branch. Source
`bh_weapon_empty_trigger` and registry dispatch in `src/bh_weapon_empty.c/.h`
preserve these distinctions and report missing sound/message/binding errors
after retained original side effects. Other `+124` identities are rejected,
not treated as implemented no-ops. This family does not claim projectile or
combat firing behavior.

The same family now includes complete index9 `+124` method `00433dc0` (138
bytes). It skips every effect while player `+240` timer is strictly positive.
Otherwise zero ammo uses the shared sound/warning path; nonzero signed ammo
decrements once and stores 15.0 at player `+240/+248` and 1.0 at `+244`.
Registry dispatch verifies the index9 identity and uses the same authoritative
weapon/player allocations. The timer is a real player field, not a copied HUD
value or an invented gameplay callback.

`message_build.py` compiles these real source translation units, links real
entity/factory/sound definitions into
`build/message-weapon/bhunter-message-weapon.exe` and runs 52 source-built
checks with zero failures. `message-result.json` records the current compiled
source hashes, dependency-object hashes and exact commands. Cases cover ring
order and wrap, arena-exhaustion partial effects, panel clamp/method mismatch,
oversize guard, all four weapon method variants, nonzero/zero ammo, real
DirectSound-backed queued request without playback, the exact warning text,
once-only warning flag, index9 timer/ammo effects, method rejection and stale generation. The panel
identity used in this isolated check is an explicit test binding; the real
pooled panel binding is covered by `tests/message_panel.c`. Parent integration
and per-address admission belong to its own build/ledger evidence.
