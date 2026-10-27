#ifndef BH_HUD_FLAT_H
#define BH_HUD_FLAT_H
#include "bh_hud.h"
#include "bh_hud_status.h"
typedef enum BHHudFlatError
{
    BH_HUD_FLAT_OK,
    BH_HUD_FLAT_ARGUMENT,
    BH_HUD_FLAT_NUMERICAL,
    BH_HUD_FLAT_QUEUE
} BHHudFlatError;
typedef struct BHHudFlatContext
{
    const BHHudContext *hud;
    long viewport_height;
    unsigned long atlas_image, weapon_icons_image;
    unsigned long attempted, queued, dropped;
    BHHudFlatError error;
} BHHudFlatContext;
/* Complete0041a3d0 flat status HUD: two background/fill bars, signed ammunition
   digits and weapon icons, signed seven-position counters, countdown mutation.
   Uses real00478f60 screen UV-rectangle queue owner and live near_z+.01.
   Viewport width is signed and scale is width/640 independent of radar unit.
   Gauge fill gate compares amount against double0.8, or flags bit8 override.
   All values/UV are retained without clamping, including negative counts and
   signed damage widths. Full queue drops are success. Source queue failures
   are recorded while later queue attempts and countdown updates still run.
   Binding/finite guards reject before publication. Borrowed HUD/frame/queue,
   weapon sequence/objects and images outlive consumption; no queue overlap or
   callback reentrancy. PC53 nearest/masked moderate finite contract. Compact
   BHHudStatusState is an explicit binding, not a restored full player layout.
   No weapon/player constructors, software dispatcher or original execution. */
int __cdecl bh_hud_flat_draw(BHHudFlatContext *, BHHudStatusState *);
#endif
