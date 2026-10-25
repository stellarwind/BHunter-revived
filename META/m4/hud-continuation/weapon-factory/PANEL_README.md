# Message panel ownership and linked source check

`export_panel_review.py` compares eight complete RET-ended bodies, 2002 bytes
total, with the immutable PE SHA256
`c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
`panel-review.json` records boundaries, instruction counts, direct calls, body
hashes and the exact double layout constants. These are game message-panel and
text behavior, neither CRT/import/compiler support nor independently counted
shared tails. Historical proprietary-engine authorship remains unresolved.

`src/bh_message_panel.c/.h` owns the four whole step methods `00442bf0`
(startup), `00442b60` (entry), `00442950` (typing), `004428b0` (exit), the
event-4 query method `00442cd0`, refresh `00442440`, and the plain text
producer `00478b60` and its distinct plain consumer `00477e80`. The plain
producer stores verified callback identity `00477e80`; unlike positioned text
`00477ea0`, that consumer calls atlas text without resetting the page cursor.
The panel-specific FIFO adapter dispatches plain, positioned and clear records
to their real source consumers. The sidecar binds the source owners to a live pooled
entity's real refresh/step/query callback slots and preserves each original
method identity as explicit data. It gives the existing `00442a30` message
owner the authoritative panel entity and active method identity. The source
constructor is only the panel allocation slice of the much larger scene
startup `004474c0` (9584 bytes); no complete constructor claim is made.
`bh_message_state_init` mirrors the original static arena pointer and
`last_index=-1` before the first message.

The refresh owner submits to the admitted screen-quad and UV-rectangle queue
owners. Source image handles, draw queue, screen width and near depth are
explicit borrowed bindings for the corresponding original globals. It draws
the TV and ring at the verified positions/UVs, decrements the ring countdown,
conditionally appends the deferred notice to the 8192-byte arena, clamps the
typing timer to .5, and retires the ring index. Unknown ring icon bytes outside
`'1'..'9'` and `'A'..'C'` are rejected or skipped as a safe adapter bound;
original out-of-range image table accesses are not emulated. Source also
rejects invalid cursor/arena bounds rather than dereferencing arbitrary
memory. The tested page is a borrowed record identity for queueing, while
actual atlas/image loading is outside this family.

`panel_build.py` compiles the real panel and message translation units with
VC5 SP3, links them against current real `build/m0` source definitions, and
executes `tests/message_panel.c`: **35 checks, zero failures**. The test uses
an actual entity pool, draw queue, text queue and DirectSound-backed sound
queue without playback. It checks the four phase transitions, plain text
producer, current and old ring rendering, numeric draw records, NaN visibility
branch, 64-tick expiry/deferred append and timer clamp, event-4 release and
stale generation. `panel-result.json` contains exact commands and source and
dependency hashes for this focused linked variant. Parent integration build
and per-address ledger admission are separate evidence. No game or original
code was executed.

`panel_native_build.py` separately compiles `tests/message_panel_native.c`
against current real source definitions and runs 53 checks with zero failures.
It mounts immutable `GAME_DATA/Bhunter.bin` as **data only**, decodes all twelve
original `TV1..TVC.PCX` images, `TAGS.BMP`, `TVFONT.PCX` and the three TV WAV
resources through source loaders, and binds the real page image, font, sound,
draw queue and entity pool to the panel. It advances startup to typing, drains
the real text queue into a native DirectDraw atlas page, consumes panel refresh
records through recovered quad/rectangle owners and a Direct3D2 offscreen
device, and saves blank and typed BMPs. Two consecutive glyphs accumulate
advance on the same native page line; a queued newline returns to the left and
moves down a line without a character sound. The native page surface hash
changes after these three queued records; readback changes by 21 pixels in
bounds `x=68..74,y=78..81`, inside the drawn atlas-page rectangle. These are fixed
source owner calls with no input/game loop or audio playback. Exact source,
object and archive hashes, commands, BMP hashes and pixel delta are in
`panel-native-result.json`.
The standalone link map has distinct live symbols for panel render, refresh,
advance, create, plain text producer and message-state initialization; no
original address is invoked as code.
