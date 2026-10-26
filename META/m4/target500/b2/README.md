# Whole VM-installed damage and radar queries

Three whole original query callbacks compile in src/bh_vm_queries.c and link
into build/target500-vm-queries/vm-queries.exe with actual source dependencies.
Focused54-check source fixtures pass twice (no archive dependency). An omitted
new-module link control fails with actual LNK2001. Parent full and clean current
integration is still required. No byte-match claim or source-game admission.

0040fe80:362B damage query, direct/contact/blast/owner detach.00410020:90B
player-only direct damage and owner detach.0043c220:413B radar entity query,
position/full state, removal-ID/owner/deferred sound/full disposal. Both omitted
catalogue queries are ordinary application/engine behavior proven registered by
actual VM pointer stores; historical authorship is unresolved. No CRT/import,
EH/helper or shared tail is counted. Separate owned jump tables38/42B add zero
functions. Each complete code span and table is verified against immutable PE.
The final43c3bc RET is independently decoded as rawC3 where Ghidra lacked that
one instruction; no code or original binary was patched.

Damage state preserves original entity+148/+14c; real BHWorldGeometry supplies
bounds, and explicit player identity controls the player-only variant. PE
literal4894f0 is **-10.0f**, so the original subtraction increases that stored
field for positive packet amounts. It is preserved literally rather than
reinterpreting an inferred health meaning. Radial scale489528 is **-.0008**;
retained midpoint/delta float spills and original mixed extended/float squares
are transcribed from full instruction flow. Blast ignores zero/outside/NaN
distance and leaves last direct source unchanged.

Radar binding installs actual query and stores original prev/next links at
ec/f0, removal ID11c and watched owner1b8. Event14 matches only nonzero ID from
packet+8; event20 unlinks/releases unconditionally; matching event17 unlinks,
releases, reacquires and binds actual bh_mission_sound_tick with1.5 seconds and
actual sound resource. Event16 writes position and two zero vectors. Count-zero
binding helpers own source lifetime, not a fabricated original constructor.

Fixtures execute every switch route and every ignored positive damage event,
source attribution, exact coefficients, radial zero/radius/outside/unordered
cases, player gate, intrusive middle/tail/head removals, IDs, foreign/matching
owner and actual delayed sound request. Native Play is never called.

The requested follow-on constructors are not admitted.43bca0 calls unadmitted
whole43a920;43bec0 installs missing366d0/36cd0,374b0/375b0,376e0,363f0 and calls
36fa0. Their returned decompilations are retained as dependencies, not source
completion. A constructor with dummy callbacks would violate restoration rules.

Integration: src/bh_vm_queries.c, tests/vm_queries.c; invoke
int bh_vm_queries_checkpoint(unsigned long *checks), scenario vm-queries.
Sources frozen in frozen-sources.json. No game/original code, automated gameplay,
input/UI, native audio playback or network operations were executed.

Final admission variant: see ../validation.json and ../line-ending-review/normalization.json. Focused frozen hashes retain the preceding candidate variant where line endings differ; final full real/clean builds independently validate identical LF source bytes. No byte-match evidence is transferred.
