#ifndef BH_WEAPON_TARGET_LIFECYCLE_H
#define BH_WEAPON_TARGET_LIFECYCLE_H
#include "bh_weapon_common.h"
#include "bh_world_segment.h"
typedef enum BHWeaponTargetError {BH_WEAPON_TARGET_OK,BH_WEAPON_TARGET_ARGUMENT,
 BH_WEAPON_TARGET_CALLBACK,BH_WEAPON_TARGET_LIMIT,BH_WEAPON_TARGET_NUMERICAL,
 BH_WEAPON_TARGET_WORLD,BH_WEAPON_TARGET_SOUND} BHWeaponTargetError;
/* Borrow real actor+f0 chain, collision owner and loaded sounds004cda20,
  004cdb88,004cdb8c. Source bounds malformed chains; no receiver is installed.
   Weapon state aliases common+e8, HUD+f8/fc, ammo+10c, room cache+104.
   Player position+0c/velocity+48 and aim+280 remain live through callbacks. */
typedef struct BHWeaponTargetContext {
 BHSoundQueue *sound;BHWorldSegmentContext *world;BHEntity *actors;
 /* Original004dfb28 is read at acquisition, after earlier entity callbacks. */
 BHEntity **actors_owner;
 unsigned long ready_sound,beep_sound,lock_sound,max_actors;
 unsigned long queries,traces,sound_requests,stops,random_calls;
 BHWeaponTargetError error;
} BHWeaponTargetContext;
int __cdecl bh_weapon_target_ready(BHWeaponTargetContext *,BHEntity *weapon,const BHEntity *player); /*004310c0*/
int __cdecl bh_weapon_target_cancel(BHWeaponTargetContext *,BHEntity *weapon); /*00431130*/
int __cdecl bh_weapon_target_update(BHWeaponTargetContext *,BHEntity *weapon,const BHEntity *player); /*00431180*/
/* Observed-field adapter only; not player or weapon constructor recovery. */
BHVec3 *__cdecl bh_weapon_target_player_aim(BHEntity *player);
#endif
