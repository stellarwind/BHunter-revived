# Whole homing projectile step0043f630

`src/bh_homing_projectile.c` recovers the complete2549-byte step owner. The
function is ordinary projectile/entity/geometry/effect behavior, separate RET
body, not CRT/import/compiler scaffolding or an independent shared tail. Its
original boundary is pinned by inventory, contiguous raw instructions and PE
bytes. Provenance is `engine_candidate`; historical engine authorship remains
unestablished. Decompilation required defining that exact function boundary
in the private bhunter-frame analysis database using the retained Java script;
no original executable bytes were changed. The main historical project was
locked and was not unlocked or modified.

The whole owner decrements lifetime using retained1/30 before its float store
comparison, handles positive/zero/negative/unordered expiry, reads actual
event3 target positions, stores the previous target sample, predicts target
motion and applies the original strict-.4 steering gate and gain-15. It
integrates1/30 displacement and calls actual `bh_world_segment_run` with the
original negative radius and self/cache state.

On a segment miss, it traverses the actual cached room's intrusive actor list
using+f4 and event3 queries, checking strict distance squared<9. The proximity
path queues the actual hit sound, creates all three particles and real flash,
releases the projectile, then invokes event2 and optional event0. Solid mesh
hits preserve geometry-versus-actor distinctions, flags4/2 precedence, normal
flip, marker size1.5/quadrant2/+normal*.1, impact particle/flash positions,
event2 impulse-.1 and event0 amount.25(world)/.08(actor). Proximity uses
impulse8 and damage.2. Nonzero original11c suppresses only event0.

The liquid path consumes its one shared CRT draw, creates the actual contact
effect with size6, duration float(.6-rand*(-1.52587890625e-6)), queues its
sound at level1 and pitch1/duration, then retires without normal damage. All
particle constructor random draws, partial allocation failures and original
side-effect order remain visible. Real entities, collision workspaces, sound
queue voices, particle/flash/contact/marker owners supply every definition.

Source context is borrowed through private payload608, with bound generation
at612. Original e8,100..108,11c,124..130 remain actual entity fields. The
existing projectile-lifetime/query binder can coexist; no alternate query,
constructor, vtable or hidden gameplay loop is invented. World response
bindings explicitly associate real geometry objects with their actual pooled
query receiver; missing mappings are errors. NULL denotes an explicitly absent
RET receiver, but marker-producing world hits require a real owning entity.
Room actors/query output, live geometry, contexts and receiver lifetimes remain
stable through callbacks. Arbitrary source mutation/recycling by receivers is
excluded; current-generation checks reject detected retirement. Original
read-after-release payload values are captured before real source release.

Numerical contract is finite moderate state under PC53 nearest/masked FP,
plus explicitly tested NaN lifetime expiry. Zero direction passed to original
normalize-to leaves undefined scratch; source reports numerical error instead
of inventing a contact normal. This is outside the admitted valid fixture
domain. PC24 transitions and broad gameplay fidelity remain open. A constructor
and full player/world assembly are still required for playable projectiles.

`python META/m4/target500/a2/build_check.py` compiles12 real source TUs, links
the admitted renderer/dependency closure, and runs **111 passing checks** in
`build/homing-projectile/bhunter-homing-projectile-check.exe`. The missing
module link control fails the actual bind symbol. Current full/clean parent
integration remains required before admission. Tests exercise real geometry
and actor meshes, actual debris physics receiving contact packets, guidance
and opposite gate, lifetime/NaN expiry, strict proximity equality, self
exclusion, every surface branch, actual marker coordinates/atlas data,
particle/flash field populations, real sound voices,8/5/1 draw populations and
real allocation failure with retained contact/damage/retirement effects.

Integration: add source and `tests/homing_projectile.c`; call
`bh_homing_projectile_checkpoint(unsigned long *)`, scenario `homing-projectile`.
Count only tick0043f630; source binder and world response registry count zero.
No game/original code, gameplay automation, input, UI capture, native playback
or network operation occurred. No shared files were edited.

Final admission variant: see ../validation.json and ../line-ending-review/normalization.json. Focused frozen hashes retain the preceding candidate variant where line endings differ; final full real/clean builds independently validate identical LF source bytes. No byte-match evidence is transferred.
