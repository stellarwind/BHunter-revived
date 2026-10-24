# Original damage and destruction trigger — 2026-10-24

This read-only block asks whether replaying the previously captured wall collision is a justified destruction test. No game launch occurred. Complete player event owner 0041d7c0–0041e664 is classified before export; it is installed by the actual player constructor through 00476e90. Ownership remains a gameplay candidate, not an admitted source implementation.

## Producer, receiver and separate accumulators

Reviewed physics 004124d0 builds event 2 with contact position, direction/impulse and a severity word at packet +0x20. It invokes entity +0xd0 only when the relevant response is nonzero. Therefore a collision-detector return of 1 alone does not prove that this damage event was delivered.

Player receiver 0041d7c0 case 2 applies physical response and checks severity*0.5 > 0.05 before the relevant sound/damage branch. The damage gate additionally needs player+0x2b4 == 0 and player+0x240 == 0. With player+0x250 < 1.5, it adds severity*0.2 to the transient accumulator at player+0x23c (the assembly subtracts a product with double -0.2). It clamps values strictly greater than 1.0 to exactly 1.0. A positive protection field clears the transient accumulator on this path. Event 0 is a separate direct-damage path that adds its supplied amount under protection/cooldown gates; it is not the same packet layout as event 2.

The player step tests the exact binary32 value 1.0 in player+0x23c while its cooldown flag is clear. That starts a damage episode, sets player+0x2b4, and adds 0.2 to the longer-lived damage field at player+0x24c. At the latter field's 1.0 threshold, the original starts destruction counter 004df6b4. The transient field decays by the original double 0.995 each step, with the subsequent float store; cooldown clears only through the original recovery branch (including transient <0.5 and permanent damage <1.0). These transient and longer-lived fields must not be conflated with ending/control state 004dfabc.

A single maximum-severity event 2 contributes at most 0.2 to the transient field on the ordinary reviewed gate, before its decay. Thus repeatedly launching the old single-contact route is not evidence of a repeatable destruction trigger. Multiple contacts, their severity, timing, protection, cooldown and state must be measured; a simple contact count does not imply destruction. No synthetic damage model or modified health is used to stand in for original behavior.

## Next original capture requirements

Capture actual player event 0 or 2 with its real producer, complete appropriate packet, transient/permanent damage and protection/cooldown values before and after the receiver. Observe the original step reaching destruction counter 1 and its 75/95/200 checkpoints, resulting menu/respawn, and subsequent original selection/spawn before claiming retry. Use an actual combat/impact input scenario that can satisfy these gates. Stop an unsuccessful bounded attempt and retain its negative result instead of repeating identical routes or injecting a damage packet.

## Evidence and limits

`damage-contract.json` pins complete receiver/constructor/physics/player-step exports and eight constants. `python tools/check_m3_damage_contract.py` verifies 48 targeted instruction records and all eight constants directly against immutable EXE bytes. This static verification is not runtime destruction/retry validation. Existing positive collision trace dfa3b4981226 proves one contact result; feature trace 5fc01fdfc4f6 samples destruction counter zero. Neither is reclassified as failure evidence.

The disc file inventory contains no loose game manual/readme beyond copyright and installer material; redist help/readmes concern dependencies. Installer CAB contents were not unpacked in this block, so absence of a bundled manual is not established. No computer-use skill, visual identification, source-game admission or original reference patch occurred. M3 remains active.
