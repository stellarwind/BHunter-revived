# Weapon factory binding and the index7/8 timer owner

`00433ea0` is the complete 203-byte, 46-instruction, RET-ended weapon factory
body, verified by the immutable PE export in
`../../hud-attachment/next-player-review/00433ea0.disasm.json` and
`../../hud-attachment/weapon-hud-target/dispatch-table.json`. It is a game
weapon initializer, not CRT, compiler support, import or shared tail. Its
historical proprietary-engine provenance remains unresolved. Direct calls are
the admitted entity pool acquire `00476f20` and query registration `00476e90`.
Its data dependencies are ten ammo limits at `0048fa38` and sixty verified
method identities at `0048fa60`. Installed query `00433e50` has an admitted
source owner `bh_weapon_target_removed` for event17; actual sound stop uses the
real `BHSoundQueue` owner. The player owns the ten entries; this module does
not claim the complete player constructor `00421d20`.

`src/bh_weapon_factory.c/.h` initializes every field and all six method
identity words that `00433ea0` writes in the actual pooled BHEntity allocation.
It registers a source query callback that calls the real event17 removal owner.
The callback ABI has no user pointer, so one explicit active factory binds the
sound and registry context. `bh_weapon_factory_close` rejects live query owners.
The typed registry borrows the ten actual allocations and records their pool
generation and constructor index. Resolution walks the active pool, checks the
generation, verifies all six original method identities in authoritative entity
storage, and returns live HUD/ammo/charge views of that same allocation. A
released address reused by another index cannot resolve as the old weapon or
dispatch its stale query. The source registry is an adapter, and the factory
is **not a complete callable reconstruction of all table methods**. Unrecovered
method addresses stay identities as data; source dispatch must report a binding
error rather than call an original address or silently succeed. The common
`0047d680` RET identity is an explicit sentinel for eight selected-HUD slots,
not eight recovered callbacks. The currently recoverable individual identities
are listed by `bh_weapon_method_available` only; that predicate does not invoke
them or claim the unrecovered trigger and firing closures.

Before decompilation, `00433820` and `00433b00` were classified as index8 and
index7 game table methods at `+12c`. `timer-review.json` and
`export_timer_review.py` verify both complete 42-byte/10-instruction bodies
against the immutable EXE (same SHA256
`7a8c14bc4dfc9cf6929ac3a1ee1d8b21add450ae6b074a24d65ced03faf27245`),
their whole RET boundary, no direct calls and the exact double 1/30 constant
at `004891f8`. These are distinct table identities with identical bodies;
`src/bh_weapon_timer.c/.h` provides one shared callable source implementation.
It subtracts the original double step from strictly positive float `weapon+f0`
and stores float; zero, negative and unordered raw bits remain intact. The
implementation borrows the real entity/common-state owner and requires the
existing masked PC53/nearest profile. It does not guess or alter any weapon
vtable, target, ammo or firing behavior.
`bh_weapon_factory_tick` resolves the live registry entry and routes both
distinct identities to that shared source owner. It also routes the existing
complete index0/1 delay and index3 dual-timer methods, while rejecting all
other +12c entries with an explicit method error.

`focused_build.py` compiles the actual factory and timer translation units with
the pinned VC5 SP3 compiler, links them with current real `build/m0` source
objects and stock VC5 LINK, then runs `tests/weapon_factory.c` as the standalone
source target. `focused-result.json` records hashes, exact commands and the
result: 97 checks, zero failures, no original code and no gameplay execution.
The checks cover all ten indices/limits/identities, actual pool/query ownership,
event17 target removal, invalid index, duplicate live construction, release and
same-address reuse across different indices, explicit missing-method status,
and timer edge behavior including crossing below zero and preserved NaN bits.
Parent integration build and ledger admission are separate evidence; this
focused result alone does not assert game integration or a complete factory
implementation. Trigger methods `0042f730` and `004309d0` and their shared
message owner `00442a30` were recovered later in this same block; see
`MESSAGE_README.md` and `PANEL_README.md`. Other unresolved table methods retain
their own explicit source frontier. Original PE/BIN/MDF/MDS were read only.
