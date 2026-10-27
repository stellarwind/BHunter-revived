#ifndef BH_WEAPON_DISPATCH_H
#define BH_WEAPON_DISPATCH_H
#include "bh_weapon_factory.h"
#include "bh_weapon_empty.h"
#include "bh_weapon_homing_track.h"
#include "bh_weapon_arc_lifecycle.h"
#include "bh_weapon_effect_fire.h"
#include "bh_weapon_pulse.h"
#include "bh_weapon_pulse_launch.h"
#include "bh_weapon_arc_fire.h"
#include "bh_weapon_hud_arc.h"
#include "bh_weapon_hud_target.h"
typedef enum BHWeaponSlot {BH_WEAPON_READY=0x11c,BH_WEAPON_CANCEL=0x120,
 BH_WEAPON_TRIGGER=0x124,BH_WEAPON_FIRE=0x128,BH_WEAPON_TICK=0x12c,
 BH_WEAPON_HUD=0x130} BHWeaponSlot;
typedef struct BHWeaponDispatch {
 BHWeaponFactory *factory;BHEntity *player;
 BHWeaponReadyContext *ready;BHWeaponTargetContext *target;
 BHWeaponArcLifecycleContext *arc;BHWeaponEmptyContext *empty;
 BHWeaponFireContext *fire;BHWeaponPulseContext *pulse;
 BHWeaponEffectFireContext *effects;BHWeaponPulseLaunchContext *pulse_launch;
 BHWeaponArcFireContext *arc_fire;BHArcContext *arc_hud;
 BHWeaponHudTargetContext *target_hud;
 unsigned long calls,identity;int error;
} BHWeaponDispatch;
/* Count-zero source callable closure for all60 entries of constructor00433ea0.
   Registry verifies all six original identities and generation before dispatch;
   identities remain data, never native pointers into the original executable.
   Only verified0047d680 RET is a successful no-op. Every other route calls its
   complete real source owner with typed borrowed contexts. Missing contexts or
   stale/edited registry fail; previous owner effects remain. */
int __cdecl bh_weapon_dispatch(BHWeaponDispatch *,unsigned int,BHWeaponSlot);
#endif
