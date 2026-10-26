# Persistent owner-attached hit markers

Two whole owners are recovered in `src/bh_hit_marker.c`: constructor00407b40
(580 bytes/136 instructions) and installed draw00407a20 (168 bytes/52
instructions). Original immutable body hashes and raw instructions are in
`owners.json` and `original/`. Ordinary entity/list/visual behavior excludes
CRT, compiler helpers, imports and independent shared-tail accounting. Engine
authorship remains unknown; role is `engine_candidate`.

The constructor acquires a real entity before eviction. It retains marker
scalar fields100/104/108, four atlas quadrant selections, normal114, original
strict(-.5,.5) up-vector decision120, position0c, actual draw identitye8,
ownerf4 and owner-list links ec/f0. Owner12c points at newest. Query00407b00 is
the admitted `bh_marker_query`; step and refresh remain the verified RET
sentinel represented by NULL. No timed step00407ad0 is installed or counted.

Context fields represent the original three FIFO globals4df244/248/24c and
borrow the real source pool, queue and image identity004f0084. The original
FIFO cap is200; its global-oldest eviction repairs f8/fc and both owner-list
directions before actual `bh_entity_release`. Newestfc remains unused/stale,
as in the original. Source disposal traverses the known count, never that
unused final link. Context pointer occupies otherwise unused payload624.

The draw owner performs ordered size>0, preserving nonpositive/unordered
suppression, then invokes admitted00478730 `bh_quad_queue` in bucket0 with
the original center/up/normal argument order, equal width/height, angle0,
flags0, modulation1, and exact .5-by-.5 atlas quadrant start/extent pairs.
The actual original world-plane consumer and backend already exist; no fake
renderer was added.

Lifetime contract: owner12c starts NULL or contains this context's real
markers; owners/context/pool outlive every attached marker. Do not directly
release markers or their owners while the FIFO retains them. The count-zero
`bh_hit_marker_dispose` adapter unlinks/releases the same memberships before
owner/pool destruction. Complete startup/world owner assembly is still open.

`python META/m4/target500/a/build_check.py` freshly compiles eight real source
translation units and links the actual retained renderer closure into
`build/hit-marker/bhunter-hit-marker-check.exe`. **1089 checks pass**, with a
missing-marker-module link failure as a negative control. Parent current full
and independent clean integration are still required before admission.

Fixtures test all four atlas quadrants, exact queued geometry/data, callback
installation and event3/20 behavior, strict normal threshold equalities and
NaN fallback, NaN size suppression, queue exhaustion, original FIFO order,
distinct owner lists and reciprocal links,401 repeated recycled-slot evictions,
real pool/query-ring consistency, allocation failure and balanced cleanup.
Every scalar constant/vector was separately read from immutable PE bytes in
`constants.json`. PC53 nearest/masked FP is required; PC24 remains unresolved.

Integration: add `src/bh_hit_marker.c` and `tests/hit_marker.c`, declare/call
`int __cdecl bh_hit_marker_checkpoint(unsigned long *)` in the source harness,
and add one missing-module link control excluding bh_hit_marker. No existing
source, shared headers, ledger, progress or build entry points were edited.
No original code/game, automated gameplay, physical input, UI capture, audio
playback or network execution occurred.

Final admission variant: see ../validation.json and ../line-ending-review/normalization.json. Focused frozen hashes retain the preceding candidate variant where line endings differ; final full real/clean builds independently validate identical LF source bytes. No byte-match evidence is transferred.
