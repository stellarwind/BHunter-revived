# Tutorial target: creation, position and kill evidence

This is a read-only original-code review, with **zero game launches** and no source implementation admission. Runtime spawn, hits, kill and mission completion remain unvalidated. `target-contract.json` and `python tools/check_m3_target_contract.py` pin ten complete function bodies, 48 selected instructions, seven numerical constants and three dispatch-data spans to the preserved EXE. These checks protect the reviewed evidence; they do not execute or independently prove its behavior.

## Creation and identity

The existing raw `TEST.LEV` placement 4827 uses `LAUNCH2.BSP` and slots `[2011,1,2013,6,5,10,0,0]`. `LAUNCH2.TXT` waits for StartKey, then launches StartAmount entities. The script's launch opcode at `004450f9` resolves placement `+1c8` when nonzero, scales bounty by 1000, converts integer speed to float, and calls `0043bca0` at `00445148`. Expected slow-target arguments are resolved placement owner, type 6, bounty 5000, speed 10.0, death event 2013. These are expectations from data/code, not runtime captures.

`0043bca0–0043beb4` generates a two-`rand` seed and forwards six arguments to `0043a920`. Its networking branches apply to types 0/3; type 6 takes the direct construction path. The launcher has no reliable entity return contract: do not treat its EAX as a created vehicle.

`0043a920–0043b2aa` calls the real entity allocator `00476f20` at `0043a978`; EAX at `0043a97d` is the allocated entity, including the possible null result. The constructor stores the launch owner at `+1a4/+1a8/+c4`, links the entity into that owner's `+128` child chain using `+f4/+f8`, and stores death event at `+11c`, type at `+fc`, bounty at `+100`, speed at `+188`, and random state at `+1e8`. Type 6 also joins the `004dfb28` list using `+ec/+f0`. The type dispatch decrements the type before its six-entry jump table; type 6 selects `0043b14e`.

At the common type-6 finish, `+cc` is `004398c0` and the callback registered through `00476e90` is `0043a1d0` at `+d0`. The constructor also sets `+e8` to `00434920`, which computes interpolated render position and submits the model. Verified CRT `rand` and `ftol` remain excluded from recovery assignments. Other reviewed owners remain engine candidates; engine provenance is unresolved.

## Position and movement

The player layout cannot be reused to identify this target's position. Constructor `+0c/+3c` are initialized from the shared vector at `00495e30`, while curve vectors beginning `+120`, `+12c`, `+144`, `+150` start at the launch-owner bounds centre. Position-query event 3 computes the curve at phase `+180 / +184` and adds displacement `+15c/+160/+164`. Its dispatcher selects `0043a385`. The render callback computes the same family of interpolated positions and passes a position pointer as the third argument of `00478170` at call site `00434b0e`; ESI identifies the entity there. Renderer sampling is conditional on that callback actually running and is not a guaranteed simulation sample.

Default update `004398c0` first calls `00437ae0`, then subtracts double `-1/30` from phase `+180`, increasing it. If phase reaches duration `+184`, it calls `00438ff0` and retains the overshoot. The latter advances the path and computes segment duration from path length, speed `+188` and multiplier `+198`. A speed argument of 10 is not independently measured constant world velocity. A future trace must capture original evaluated positions or sufficient raw curve state; reading only `+0c` could falsely report a stationary target.

## Damage and destruction

Event 0 of `0043a1d0` starts at `0043a6e9`: packet DWORD 1 is float damage and DWORD 2 is attacker. For type 6, damage from any attacker other than actual player global `004dfab8` becomes zero. Player damage is multiplied by double 0.8; values greater than double 0.45 become float `0.44999998807907104`. The callback accumulates it at `+194`, switches update to `00439910`, and records the attacker at `+1ac` if the accumulated result reaches 1.0. It also adjusts the movement multiplier and segment timing. A callback dispatch by itself is not evidence of accepted damage.

`00439910` decays positive accumulated damage by double 0.998 twice per update for type 6. It tests the resulting damage against 1.0 before the destruction branch. Exact shot count therefore depends on actual damage and spacing; the per-event cap alone does not establish a three-shot kill.

The destruction branch unregisters the event receiver, stops owned sound, emits effects/debris/reward behavior, broadcasts event `0x11` naming the removed entity, and then, if `+11c` is nonzero, broadcasts event `0x0e` with that stored ID. At `0043a0e5` the packet sender is still the entity written at `0043a0a4`, and the event ID was loaded from `+11c` at `0043a0b9`. It then unlinks both chains and releases the entity at `0043a177`. Event 2013 attribution must be checked against the captured target pointer, not merely observed as an arbitrary mission event.

## Next runtime acceptance

Prepare all observations before the route begins so receiver ordering cannot hide a spawn occurring before the instructor consumer returns:

1. Capture the launch opcode's actual arguments and resolved placement owner for 4827. Capture allocator result inside the constructor and verify finished type, owner links, callbacks, speed and death ID on that same entity.
2. Capture several original evaluated positions or raw curve states across updates. Verify movement and actual entity lifetime. Do not infer movement from the speed field or use the player position layout.
3. Follow that entity's event-0 entry/return and update callback; capture attacker, input damage, accumulated damage and callback transition. Native weapon input and original collision/projectile handling must produce the event.
4. For kill acceptance, capture its destruction-path event-2013 packet and original mission-consumer return, then unlink/release. This proves the first target kill only; later pickup, faster target and episode completion remain separate outcomes.

No position, health, event, mission state, original callback or API success injection is authorized by this review. The existing `80df3c4686e1` handoff trace ends at the mission consumer return and does not establish whether the launcher receiver had already run. No additional launch was made to resolve that ordering here.
