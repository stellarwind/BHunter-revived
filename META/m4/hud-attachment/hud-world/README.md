# Complete world status HUD 00418f00

This owner spans 00418f00..0041a3c5: 5,318 bytes, 1,447 instructions. It is
game-specific HUD behavior with a high-confidence boundary: its sole external
caller is player scene callback 0041c860 at 0041d207, selected when the embedded
camera mode at player+e8 is zero. There is no callback installation, EH/unwind
scaffolding, independent shared tail or import thunk in this owner.

`export_review.py` reproduces the full Ghidra export and validates every
instruction byte against the pinned immutable EXE. `raw-review.json` pins the
whole raw body and all 33 referenced floating constants. The three outgoing
function identities are the already admitted complete quad producer 00478730,
verified CRT `__CIpow` 0047df70 and verified CRT `__ftol` 0047de20. The CRT
helpers are reused and excluded from gameplay recovery assignments/counts.
The intrinsic wrapper consumes ST1 as base and ST0 as exponent; the owner
loads 10 and 6 respectively. Source calls actual CRT `pow(10.0,6.0)` followed
by the original signed truncating cast, preserving seven decimal positions.

## Source and data ownership

`src/bh_hud_world.c` / `.h` recover the whole behavior with real quad queue
definitions. The context borrows the setup's actual frame and world center,
queue and two image handles. The atlas is original 004f54e4; weapon icon sheet
is 004f0054. Every draw uses bucket3, angle0 and flags0. Ammo/icon modulation
is binary32 .8; gauges/counters use1. No original function is counted for the
new source helpers or shared status binding.

`src/bh_hud_status.h` supplies compact observed-field bindings shared with the
complete flat status owner, without claiming an invented compact original
player layout. The inputs are gauges +24c/+23c, selected bytes +2b6/+2b7,
flags +2b8, counters +230/+234, pending +238 and mutable countdown bytes
+2b9/+2ba. The borrowed weapon pointer sequence is player+258; selected bytes
zero extend and index that actual sequence. A supplied count validates its
accessible length rather than assuming a guessed ten-entry or 256-entry
original allocation. The selected weapon prefix has a checked signed amount
field at +10c. No weapon constructor, dispatch callback, projectile behavior
or allocation/lifetime implementation is fabricated by this read-only view.

The full owner publishes both gauge backgrounds, optionally fills each gauge,
publishes selected weapon ammunition digits and icons, publishes both number
panels and seven-position counters, then mutates both countdowns. Gauge fill
requires a nonnegative amount and either amount < the original **binary64 .8**
or player flags bit8. The override does not clamp fills above1. The stored
binary32 remainder controls fill placement and height; the retained remainder
controls UV extent. Countdown bit8 is read before decrement. Both nonzero
unsigned bytes decrement; countdown_b reloads16 when it becomes/remains zero
and pending is nonzero. Full queues drop individual draws but do not suppress
any later publications or these state changes. A source queue error similarly
retains later owner work and diagnoses failure after the mutations.

Signed ammo division and the original three-digit rules are retained, including
negative values and a hundreds quotient greater than9. Counters retain only
seven positions, so signed32-bit extremes produce a large first atlas glyph
index followed by six decimal remainder glyphs. This is deliberately not an
unrestricted decimal formatter. The source preserves the original raw atlas
coordinates rather than silently clamping them. All products/subtractions in
the signed decimal division paths remain in range, including INT_MIN.

## Validation scope

`tests/hud_world.c` links the actual owner and real quad producer. Tests cover
a complete independently specified 17-command sequence, both complete meter
gates, selected icons, internal and leading zeros, every unsigned countdown
value with/without pending reload, full queue and topology failure behavior,
signed ammo/counter extrema, a supplied 256-pointer selection table, exact
selected object layout and source argument/nonfinite guards. A fixed raw-word
meter fixture distinguishes the FST-stored remainder from its retained x87
counterpart in placement/height versus atlas extent.

The first linked test variant exposed one test expectation error: the last
bucket3 command resumes existing header5, not end-of-queue. The real queue
topology was correct; the final test checks that existing continuation. Setup
and status modules have separate source/integration evidence, with final
admission and the actual final target results owned by the enclosing block's
ledger and audit. No source byte-match claim transfers from reference exports.

The contract remains the restoration's PC53 nearest/masked moderate finite
profile. PC24, exceptional/huge floating behavior, complete player/weapon
construction and the full scene caller remain open. No original code is
mapped or executed, no game is launched and no gameplay is automated.
