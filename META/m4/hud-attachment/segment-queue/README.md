# Complete segment queue consumer family

`00455e40` is the complete 147-byte/44-instruction ribbon-pool append owner;
`004777c0` is the complete 104-byte/43-instruction segment command consumer.
Both are rendering behavior, not CRT/imports/compiler helpers or independent
shared tails. The existing `00478390` producer installs `004777c0`; production
source had the producer but lacked this consumer. No new producer recovery is
claimed. Historical engine authorship remains unresolved.

Fresh full exports and immutable PE byte checks are in
`../next-player-review/00455e40.*`, `004777c0.*` and `raw-review.json`;
`../frontier_review.py` reproduces them. The immutable EXE SHA256 is
`c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
Source is `src/bh_segment_queue.c/.h`. The append is its own complete body,
not an alias of `00455ee0`: longitudinal UV is the stored signed pool count
minus one, converted to binary32. It increments used, clears/links adjacent
entries, calls the actual projected matrix transform `004012e0`, writes width,
flags and modulation, and preserves corner scratch. The consumer appends both
endpoints before the real `004569b0` near clip/ribbon draw, then loads the current
command next link. Lookup resolves a borrowed real image rather than executing
original callback pointers. Original capacity overflow/nonfinite domains are
diagnosed by source guards; successful native draw owns pool reset.

`tests/segment_queue.c` passes **72 checks** in the named focused executable
`build/segment-queue/bhunter-segment-queue.exe`. Checks include exact implicit
UV/links/scratch, a nontrivial actual camera matrix, all eight flag combinations
through real ribbon geometry, both near crossings and full clipping, appending
to a preexisting pool, pool capacity/lookup/emit failure effects, and post-emit
next-link reload. Observation callbacks are isolated harness sinks/resolvers;
they replace neither the transform nor the ribbon/clip owners. Source and real
dependency-object hashes plus exact commands are in `focused-result.json`.

The extended actual native fixture `tests/weapon_hud_native.c` also submits an
independent taper/modulation segment through the existing producer and both
new owners, real attribute clip/native emitter/texture owners and offscreen
Direct3D. Fresh checks pass **94 clean / 181 actual archive**. It distinguishes
blank, solid-red geometry, curved weapon path and actual decoded `sray1.bmp`
shape; images are `build/weapon-hud-native/weapon-hud-native-*-segment.bmp`.
Actual segment bitmap was visually inspected. The prior 88/169 weapon-only
fixture is a historical source variant, not validation transferred to this one.

No game/original code or gameplay/input loop was executed. Parent owns current
full integration admission and global ledger; focused evidence alone is a
candidate until the current linked target and affected scenarios pass.
