# Whole effect sound cleanup00403850

Complete31-byte cdecl effect cleanup calls the actual sound-stop owner with the
entity+128 handle, then releases the real source pool entity even when that
handle is stale or invalid. Distinct ordinary engine-candidate callback, no
CRT/import/EH/shared tail and no byte-match claim. Count-zero source binding at
private payload+604 replaces original pool/sound globals. Constructor403870 is
not restored or counted.

Focused compile/link/run passes19 checks twice with actual queued stream voices,
selected/foreign voice preservation, real stale/invalid handles and actual pool
release. Missing new module fails actual LNK2001. Current render dependency's
menu-light consumer is freshly compiled as a real source dependency; it adds no
owner to this family. No stub, native Play, original/game code or input occurs.
Parent full real and independent clean source integration remains required.

Integration: src/bh_effect_cleanup.c, tests/effect_cleanup.c;
int bh_effect_cleanup_checkpoint(unsigned long *checks), scenario effect-cleanup.
Exact complete immutable-PE-verified boundary is in owners.json and original/.
Current source/header/test hashes are in frozen-sources.json.

Final admission variant: see ../validation.json and ../line-ending-review/normalization.json. Focused frozen hashes retain the preceding candidate variant where line endings differ; final full real/clean builds independently validate identical LF source bytes. No byte-match evidence is transferred.
