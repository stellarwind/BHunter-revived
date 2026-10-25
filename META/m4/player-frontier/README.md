# Final bounded player/input family

Two complete original behaviors are source-built in the separate
`bh_player_input_events.c` translation unit:

| Original | Source symbol | Role |
|---|---|---|
| 004215c0–00421605 | `bh_player_input_broadcast` | game |
| 004757e0–00475801 | `bh_player_input_bind_kind4` | engine candidate |

Classification preceded decompilation: complete raw bodies and callers were
reviewed first. These are distinct game-list dispatch and input-tag owners,
not imports, CRT/compiler helpers or shared tails. Complete instruction exports
were verified against the pinned original EXE. Engine provenance for the generic
input writer remains unresolved.

The broadcaster receives the former global 004dfb28 list explicitly. It selects
members whose game class at +fc equals 1, writes event 15 into the first word of
the original ten-word stack packet and calls the actual installed +d0 query.
Remaining packet words retain their original unspecified state. It reads the
+f0 successor after the callback, preserving observable list mutation. Source
bounds diagnose cycles and missing callbacks without undoing prior effects.
No callback is supplied or installed by production source.

The input writer sets only the indexed binding's kind byte to 4 and raises count
to `index+1` when needed. It preserves flags, keys, names and all other opaque
bytes. The kind's broader device semantics are not guessed by its source name.

`python META/m4/player-frontier/build_evidence.py` compiles the new translation
unit, focused tests and the actual existing input-list owner, links real CRT
definitions and executes 267 checks in `player-input-events.exe`. Checks cover
every valid tag index, full-byte write footprints, retained maximum count, actual
allocation, class filtering, callback order and event reset, successor mutation,
missing callbacks, cyclic lists and empty/zero-limit behavior. Observable fixture
callbacks are isolated in tests and read only the defined event word.

00421610 was classified and statically reviewed but remains `not_started`.
It selects the lowest distance divided by a signed target weight, combining an
eligible current player with class-2 event-3 queried members. A dedicated next
block must preserve zero/negative weight division, mixed x87 stores and strict
comparison before storing a new score. No source completion or admission is
claimed for that function merely because its decompilation was inspected.

This family does not modify the earlier input implementation variant. Named
build/map evidence and exact source snapshots are retained here; the parent
restoration block records normal harness integration and the canonical ledger.
No original code, startup or gameplay was executed.
