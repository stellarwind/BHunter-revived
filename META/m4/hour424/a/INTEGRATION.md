# Parent-owned renderer integration

No admitted source TU/header is edited by the family agent. Before admission,
parent preserves the prior variant and performs these small explicit changes:

* In `src/bh_render_execute.h`, include `bh_flash_flare.h`; add borrowed
  `BHFlareSubmit *flare` to `BHRenderExecute`; expand `kinds[17]` to `[18]`.
* In `src/bh_render_execute.c` draw_one switch, add actual whole dispatch:
  `case BH_DRAW_FLARE: return e->flare ? bh_flare_execute(e->flare,(BHDrawCommand*)c,next) : -3;`
* Its source kind guard changes from `c->kind>=17` to `>=18`. Path point
  rejection remains. `BH_DRAW_FLARE` is defined17 in the new family header,
  so existing enum/source bytes in `bh_draw_queue.h` need no change.
* Recompile/relink real and clean integration; refresh active source hashes
  for the admitted renderer implementation variant and every affected owner
  source variant without transferring byte-match evidence.

Existing callers zero-initialize executor state. A queue containing a flare
requires its real borrowed flare binding, just as other kinds require their
actual draw contexts. Missing binding remains the established explicit error.
