# Independent world status HUD cross-review

Reviewed `src/bh_hud_world.c` against the complete00418f00 raw body supplied in
`../hud-world/original/`:5318bytes/1447instructions. Read-only review; no source,
test or global ledger edits and no original/game execution. The separate HUD
agent owns implementation and validation evidence for this function.

All position constants were checked against the exact PE constant table,
including primary/secondary digit horizontal positions, label offsets, and the
deliberate distinction between secondary digit vertical
-0.06250000000000001 and icon vertical-0.0625. Flag gates, signed ammo division,
leading/internal-zero handling, always-present units, seven-position counter
loop, float horizontal updates, pre-decrement bit8 blinking and post-decrement
pending reload16 agree with the complete raw control flow.

Detailed first gauge fill x87 trace00419058..00419183 confirms:

- retained remainder=1-amount feeds UV extent*.6953125;
- stored float remainder is reloaded for placement and height;
- placement remains double: .159-(1-stored_remainder*.5)*.13;
- world point is computed from fresh center-axisX*horizontal+axisY*placement,
  retaining intermediates until the final float point stores;
- height is stored_remainder*.13 and V-start.00390625-amount*(-.6953125).

No further finite-domain semantic defect was found. This is a static contract
review, not standalone admission or a claim about PC24/exceptional behavior.
Flat HUD0041a3d0 differs: it retains remainder rather than reloading it for fill
placement. That distinction and raw-half-dimension corrections are implemented
and tested separately in the flat owner's own evidence.
