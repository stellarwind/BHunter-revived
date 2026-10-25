#ifndef BH_HUD_WORLD_H
#define BH_HUD_WORLD_H
#include "bh_hud.h"
#include "bh_hud_status.h"
typedef BHHudStatusState BHHudWorldState;

typedef enum BHHudWorldError {
    BH_HUD_WORLD_OK,BH_HUD_WORLD_ARGUMENT,BH_HUD_WORLD_NUMERICAL,BH_HUD_WORLD_QUEUE
} BHHudWorldError;
typedef struct BHHudWorldContext {
    const BHHudContext *hud;
    unsigned long atlas_image,weapon_icons_image;
    unsigned long attempted,queued,dropped;
    BHHudWorldError error;
} BHHudWorldContext;

/* Complete00418f00: two gauge backgrounds/fills, selected ammo digits/icons,
   two signed seven-position decimal counters and all countdown mutations.
   Gauges/counters/ammo are not clamped. Countdown bit8 suppresses its number
   before decrement; pending reloads countdown_b to16 after decrement-to-zero.
   Full-queue drops and source queue errors still execute later publications
   and countdown updates. Source binding/numerical guards reject before any
   publication or mutation. PC53 nearest/masked moderate finite contract;
   source bindings do not implement player/weapon constructors or callbacks. */
int __cdecl bh_hud_world_draw(BHHudWorldContext *,BHHudWorldState *);
#endif
