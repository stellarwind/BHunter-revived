# UI binding verification, 2026-10-27

This pass audits the current application adapter against retained original
resource references. Run `python tools/check_ui_bindings.py`: 77 resource
contracts, 14 derived/font/pointer/viewport bindings and three in-memory rejection
controls. The controls reject the old shield-as-radar mapping, an absent score
page, and a late override that swaps the bullet flare back to firebolt.
`contracts.json` names each source field, original identity, asset and hashed
original evidence. This is a binding audit, not a pixel-equivalence claim.

| Consumer | Previous source binding | Correct original binding |
|---|---|---|
| Radar background | shield | radar `004f00c0` |
| Radar dots | shield | dots `004f54dc` |
| Radar target bar | red | tags `004f00b4` |
| World and flat HUD gauges/digits | radar | clipbrd `004f54e4` |
| Player status/name label | dots | shared font page image `004cdb9c` |
| Weapon target brackets | weapons | tags `004f00b4` |
| Target price label | radar | clipbrd `004f54e4` |
| Pickup numeric label | unbound | clipbrd `004f54e4` |
| Pickup glow | unbound | yellow flare `004f00b0` |
| Score draw image/text context | unbound | shared font image/page and text queue |
| Pickup label scale input | pixel depth 16 | viewport width 640, original `004e2998` |
| Ballistic flare / trail | firebolt / white flare | white flare / firebolt |
| Homing flare / trail | plasma / green flare | green flare / plasma |

The remaining reviewed menu sounds, logos, player overlays/crosshairs, radar
connectors, dialogue images/sounds, HUD weapon icons, arc image and target lock
sounds agree with their recorded identities. All twelve dialogue portrait
bindings are checked against the original contiguous table. Bootstrap constructs
the current 18 menus and 118 rows with real resources; row labels and button
models remain the recovered bootstrap data and owning menu implementations.

The source screenshot shows the CD warning, fully typed, with no history entry.
The original screenshot shows an earlier dialogue state and a history icon row.
Panel position depends on history state. No global UI enlargement or portrait
repositioning was applied to imitate different states. The scene/camera positions
also differ, so these images alone do not establish a ship projection mismatch.

Bullets use the restored ribbon/flare producers. Original ballistic flare size2,
strength50 and ribbon width0.3 are preserved; homing sizes are preserved too.
The correction exchanges the actual assets, not their original dimensions.
The static source trail now produces 695 nonblack pixels versus 428 with the
wrong flare texture. This establishes a visible path, not mission size fidelity.

The shield sound has a separate **explicit source lifetime fix**. The recovered
timer path can restart a loop after its one-second stop threshold and leave a
live handle when the timer reaches zero. The source now stops that owned voice
at expiry and invalidates the handle. This additional cleanup is not claimed
as original instruction equivalence. A synthetic linked source fixture verifies
a live voice before expiry, retirement at expiry and no restart on the next tick;
it performs no native playback, physical input or game execution.

Full real-asset integration: 5,530,029 checks, zero failures. Independent clean
integration: 5,024,074, zero failures. The 299-TU game and bounded static startup
pass, including beam/boost/logo/portrait plus new radar and complete flat HUD
render fixtures (3524 radar and 8348 HUD nonblack pixels). Startup collects no
input and dispatches no entity steps/refreshes or application frames. No original
code runs. Historical source variants are retained; zero owners added, M4 incomplete.

Human confirmation of appearance, bullet size and audible shield expiry remains
pending. Collision/bullet-load performance remains open. Score resource/page
bindings are repaired, but network score event integration and actual peer UI
remain unverified. Native hardware, timing and matching-state pixel fidelity are
not established by this pass.
