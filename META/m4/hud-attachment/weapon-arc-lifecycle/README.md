# Complete arc weapon lifecycle family

Classified before decompilation in `CLASSIFICATION.md`: complete ordinary
weapon-index4 table methods `004324a0` (+11c), `00432510` (+120), `00432c60`
(+12c). They are not CRT/imports/compiler helpers or separately counted tails;
historical authorship remains unresolved. Whole bodies are106B/29instructions,
65B/18instructions and900B/280instructions. `export_review.py` verifies every
raw byte, contiguous instruction and RET ending against immutable EXE SHA256
`c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
Metadata, decompilations, disassemblies, readable TSV and hashes are stored
locally. Ghidra's explicit body declarations modify analysis metadata only.

`src/bh_weapon_arc_lifecycle.c/.h` contains all three source owners:

* `bh_weapon_arc_activate` resets only+e8/targetf8/chargefc/handle100, consumes
  exactly one shared CRT rand and requests real loaded sound004cda20 at level.8,
  stored pitch double.9-rand*negative double.000006103515625. Latch+108 clears
  AFTER synchronous real sound allocation; other shared state is preserved.
* `bh_weapon_arc_deactivate` captures handle+100 before clearing target/charge,
  stops the actual nonsentinel queued voice, then clears latch and handle.
* `bh_weapon_arc_track` consumes the actual latch+108, owns complete target
  acquisition and retention, signed charge increment/wrap, cues at16 and51,
  existing-voice update, release/stop paths, and charge reset timing. Inactive
  resets charge; losing an active target preserves charge. All player vectors,
  weapon target/charge/handle and queries remain live across callbacks. The
  method installs no query or callback table, constructs no actors/weapons, and
  calls actual world/sound owners rather than stubs.

Acquisition queries the actual actor list at004dfb28 in successor+f0 order,
using one40-byte packet whose event3 is initialized once. It uses stored float
xyz differences; raw dot is `(aimX*storedX + aimZ*storedZ)+aimY*storedY`, stored
before nearest150 comparison. Norm groups `(storedX²+storedY²)+storedZ²`.
Cone compare is the original C0-only test: finite values require `<double.1`,
but unordered0/0 ALSO passes. Every candidate must pass the real complete
`bh_world_segment_run` with self=player, radius-1 and a direct live alias of
weapon+104 cache. Only actual miss0 acquires; dependency error is not visibility.

Retention uses raw xyz differences while storing x/y: norm is
`(rawX*storedX + rawY*storedY)+rawZ²`; dot is
`(rawZ*aimZ + aimY*storedY)+aimX*storedX`. Cone is stored float before comparing
to double.1; projection150 is accepted. Zero norm performs actual masked0/0 and
retains unordered angle acceptance/sticky invalid. No finite stand-in or epsilon
is inserted. These raw groups/control paths were independently traced by the
attachment agent. Its concrete acquisition unordered finding was fixed and
given a dedicated source regression check before freezing this variant.

State is the actual `BHWeaponCommonState` subrecord over source BHEntity storage,
including shared `BHWeaponHudState`; no copied target/charge or guessed globals.
The context borrows actual sound resources, world query definitions and actor
list. Loaded cues are004cda20/004cdc74/004cdc98/004d1904. Actors' installed event3
methods and all allocation/query lifetimes remain the caller's contract. Source
guards expose invalid bindings, missing callbacks, nonfinite fields, cycles and
dependency failures while preserving prior observable mutations. Moderate finite
PC53 nearest/masked profile is tested; PC24/extreme universal fidelity is open.

Focused real translation-unit compile/link in
`build/weapon-arc-lifecycle/bhunter-weapon-arc-lifecycle.exe` passes **37 checks**;
`focused-result.json` pins exact commands and real dependency-object/source
hashes. Checks cover whole-allocation activation/deactivation subsets, independent
CRT/pitch prediction, actual native PCM voices with no Play, actual allocator
mutation (including post-sound latch clear), cue thresholds16/51 and signed wrap,
real voice update/stop, strict nearest/tie/150 acquisition,150 retention, both
coincident unordered/sticky cases, live query mutations/replacement, mutated
successor and packet event, actual BSP occlusion positive/miss controls,
world-error retention and bounded cycles. Observation callbacks/allocator are
isolated harness boundaries delegating actual owners, not production defaults.

Full source integration/final ledger admission belong to parent evidence. No
game/original code, gameplay/input loop or sound playback executed. No additional
original functions are assigned by this module; the user requested exactly300.
