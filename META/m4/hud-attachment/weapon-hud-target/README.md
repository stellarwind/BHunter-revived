# Selected weapon target HUD 004314f0

This complete game-specific owner spans 004314f0..0043180e: 799 bytes and
235 instructions. It is the selected HUD callback for weapon table index5.
The player scene callback invokes weapon+130 with weapon/player arguments;
this owner uses only the weapon argument. Its direct dependencies are the
already restored point projection 004558d0 and UV rectangle queue 00478f60,
plus the target's actual installed event3 query callback at target+d0. No
query implementation or weapon lifetime behavior is invented by this owner.

Classification preceded recovery: the callback table's DATA reference at
0048fadc identifies a game HUD method, and its ordinary prologue, full body,
internal branches and final RET have a reviewed coherent boundary. It has no
CRT/import/compiler/EH/shared-tail signature. Ghidra had instructions at the
entry but no containing function. Its shorthand `function create` failed;
the explicit FunctionManager/AddressSet annotation in
`../hud-world/DefineReticleBoundary.java` established this exact reviewed body
without changing any executable bytes or overlapping another function. The
preceding read-only inspection script recorded no containing function.
`export_review.py` now reproduces the full export and verifies every raw byte
against the pinned immutable PE. `raw-review.json` pins the complete body and
three referenced floating constants. This is not source byte-match evidence.

## Actual table and next-frontier facts

`dispatch_review.py` pins the actual 240-byte six-callback/ten-weapon table
0048fa60, its ten ammo limits and the complete weapon factory's raw body hash.
Factory00433ea0 installs the second word of each record at weapon+130. The
player constructor00421d20 creates exactly indices0..9 and defaults selectors
to0/9. Eight selected-HUD entries point to the same verified common RET at
0047d680; only index4/00432ff0 and index5/004314f0 perform drawing. Repeating
the same RET in eight table entries does not add eight recovered game owners.
Other five callback columns, weapon factory/player construction, firing and
projectile/query lifetimes remain separate restoration obligations.

The bounded network-label review also verifies all 1,036 bytes/274 instructions
of existing join packet consumer00427920 as reference data. It stores a remote
slot in both table004dfad8 and actor+114, sets kind8 at actor+fc, initializes
score+11c and name+11e, and links the same actor into marker list004dfb28.
Thus marker label rows and the score atlas share the actual remote slot
identity. `remote-label-review.json` records raw anchors. No source admission
is claimed for that consumer.

## Source behavior and bindings

`src/bh_weapon_hud_target.c` / `.h` recover the complete target reticle.
`src/bh_weapon_hud_state.h` shares explicit target+f8/signed charge+fc bindings
with the other selected weapon HUD method. These are observed borrowed field
bindings, not an invented compact original weapon allocation. The query may
mutate charge, replace target or change live renderer state; its original
40-byte packet extent is retained. The unit is signed viewport-width/160 and
is captured **before** querying. Charge is read after actual projection.

After successful projection, four diamond-oriented corner images always
publish to bucket3. Signed charge <48 uses the original wrapped
`(80-charge)*unit`, signed truncating division by4 for radial offset and a
4-unit corner size. Its central bar blinks on mask4. Charge >=48 always draws
a central ready bar and alternates 8/12-unit radius and 4/6-unit size by mask4.
Bar dimensions, raw atlas coordinates, four raw angle words and live near+.01
depths are preserved. A null target or near-plane projection cull returns
without drawings; full queues drop individual commands while remaining draws
are attempted. Source queue errors also retain subsequent attempts and query
effects. Inputs/context/queue must not overlap, and borrowed target/query/
queue/image lifetimes survive the original synchronous/queued ownership.

## Validation scope

`tests/weapon_hud_target.c` uses the real projection and rectangle queue owners.
Its explicitly isolated observable query fixture supplies a position, writes
the last word of the full packet and can mutate charge/target/viewport/near
state to verify ordering; it is not installed by production source or counted
as a recovered target method. Tests cover all threshold/blink combinations,
negative charge, signed wrap at INT_MIN, raw UV/angle payloads, signed viewport
boundaries/extremes, live-depth and target-query ordering, null/cull/equality
paths, full/partial/invalid queues and source guards.

The focused real-TU build/link/run passes **123 checks, zero failures** in
`build/weapon-hud-target/bhunter-weapon-hud-target.exe`. Reproduce with
`focused_build.py`; `focused-result.json` records exact commands, own-source
hashes and hashes of every real m0 dependency object used by that focused
target. This is distinct from the final full/clean build: enclosing integration
and admission evidence belongs to the block's audit and function ledger.
The numerical contract is the restoration's
PC53 nearest/masked moderate finite profile. PC24, exceptional floating cases,
full weapon/player creation and the complete scene caller remain open. No
game process or original code execution is used by this family.
