# Whole pulse weapon firing0042f7a0

Complete3659-byte ordinary firing owner in `src/bh_weapon_pulse.c`. Separate
whole RET body with raw PE-verified instruction/boundary data; no CRT/compiler
support/import/shared-tail counting. Historical engine/game provenance remains
unknown (`engine_candidate`). Historical decompile is retained alongside raw
instructions; `constants.json` records every referenced numeric PE literal.

The complete owner handles alternating phase with unsigned32 wrap, empty ammo
early return, signed-ammo decrement, actual player basis muzzle position,
shot sound, conditional actual flare, real smoke particle, shared player recoil
fields, actual player event2 call, randomized normalized firing direction,
initial trail, actual cached world/actor query with radius3, every surface
branch, remaining trails, exact type8 network packet and cooldown/phase state.

The normal impact creates all original particles, flash, two spark batches,
optional world marker(quadrant3,size.7), randomized smoke and conditional real
debris, then dispatches contact/damage packets through the actual borrowed
receiver. Liquid preserves its actual rays owner, duration/size7/level7/6,
pitch1/duration and offset. Actual real effect constructors retain their own
shared CRT random consumption and partial allocation behavior. The network
branch uses real source packet framing/checksum and transport: type8, sender,
five payload bytes(localID low byte,4,2,0,255),13 transmitted bytes and16 padded
bytes. Negative local ID skips transmission but still sets delay8. Odd-phase
calls increment phase only; zero ammo leaves phase unchanged.

Context binds actual source pool, world/actor segment owner, geometry response
entities, sound voices, particle/contact/flash/marker/debris owners, shared
resource IDs and network transport. Original player direction280/recoil2a4,
weapon phasee8/ammo10c/cache104/delay114 remain actual shared entity storage.
All borrowed query/context/geometry/list lifetimes remain stable; arbitrary
callback mutation/recycling is excluded. No invented query, no-op weapon
method, fake player vtable, game loader or backend was introduced.

**Gameplay limit:** whole original player query0041d7c0/constructor/tick and
live startup bindings remain missing. The owner invokes the actual caller's
query and does not claim that the player receiver semantics or live firing
scenario are restored. Fixtures use an actual source factory index1 weapon
record and an actual source debris physics receiver to validate the full
firing owner/event2 ordering and shared words. That fixture is a typed source
integration case, not a claim that debris query substitutes the player query
in a shipping game. Factory method dispatch remains independently unfinished.

PC53 nearest/masked finite moderate arithmetic is required; original retained
double final-trail distance is preserved without adding a float spill. PC24
transitions, native playback and actual peer gameplay remain unresolved.

`python META/m4/target500/a4/build_check.py` freshly compiles18 real source
TUs, links actual admitted dependencies, executes **71 passing checks**, and
confirms an omitted-module unresolved firing symbol. Parent current full and
independent asset-free integration remain required before admission. Fixtures
test real factory identity, ammo/phase/wrap, exact18/19/49-or58 draw populations,
muzzle/smoke/recoil, real source physics packets, conditional flare, cached
segment geometry and actors, solid/flag2/liquid paths, real marker/particle/
flash/spark/debris ownership, exact packet bytes/XOR receiver acceptance,
cooldown, negative local ID and NULL-peer S_FALSE with zero native Send calls.

Integration: add source and `tests/weapon_pulse.c`, entry
`bh_weapon_pulse_checkpoint(unsigned long *)`, scenario `weapon-pulse`.
Count only original0042f7a0; context, packet storage and fixture components add
zero. No shared files were edited. No original/game code, automated gameplay,
physical input, UI capture, native audio playback or network operation ran.

Final admission variant: see ../validation.json and ../line-ending-review/normalization.json. Focused frozen hashes retain the preceding candidate variant where line endings differ; final full real/clean builds independently validate identical LF source bytes. No byte-match evidence is transferred.
