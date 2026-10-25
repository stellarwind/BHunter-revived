#ifndef BH_WEAPON_HUD_TARGET_H
#define BH_WEAPON_HUD_TARGET_H
#include "bh_weapon_hud_state.h"
#include "bh_pickup_draw.h"

typedef enum BHWeaponHudTargetError {
    BH_WEAPON_HUD_TARGET_OK,BH_WEAPON_HUD_TARGET_ARGUMENT,
    BH_WEAPON_HUD_TARGET_CALLBACK,BH_WEAPON_HUD_TARGET_NUMERICAL,
    BH_WEAPON_HUD_TARGET_QUEUE
} BHWeaponHudTargetError;
typedef struct BHWeaponHudTargetContext {
    BHPickupDrawContext *projection;
    long viewport_width;
    unsigned long atlas_image,queries,projected,attempted,queued,dropped;
    BHWeaponHudTargetError error;
} BHWeaponHudTargetContext;

/* Complete004314f0, the ten-weapon table's index5 selected HUD callback.
   Null target and projection culls succeed without draws. Event3 queries use
   the original40-byte packet extent; callbacks fill xyz synchronously. Unit
   width/160 is captured before query; signed charge is read after projection.
   Four rotated corners always draw after successful projection. Charge<48
   blinks its central bar on bit4; charge>=48 always draws the ready bar.
   All raw signed wrap/division, angles, UVs and near+.01 depths are retained.
   Queue exhaustion drops individual draws; source queue errors preserve later
   attempts. Context/queue/input must not overlap. PC53 nearest/masked moderate
   finite contract; no target query implementation is supplied by this owner. */
int __cdecl bh_weapon_hud_target_draw(BHWeaponHudTargetContext *,const BHWeaponHudState *);
#endif
