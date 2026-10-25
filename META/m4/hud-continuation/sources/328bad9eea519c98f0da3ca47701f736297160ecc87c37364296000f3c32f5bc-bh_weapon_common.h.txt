#ifndef BH_WEAPON_COMMON_H
#define BH_WEAPON_COMMON_H
#include "bh_weapon_hud_state.h"
#include "bh_sound_queue.h"

/* Actual observed weapon+e8..117 subrecord. Unknown words retain offset names;
   this is not a weapon constructor or a complete weapon allocation claim.
   The HUD state is an actual shared subobject at original+f8, not a copy.
   Ammo+10c, limit+110 and delay+114 share this same borrowed entity storage. */
typedef struct BHWeaponCommonState {
    unsigned long word_e8,available_ec,word_f0,word_f4;
    BHWeaponHudState hud;
    long word_100;
    unsigned long word_104,word_108;
    long ammo,ammo_limit,delay;
} BHWeaponCommonState;
/* Source field adapter over the real BHEntity allocation. No allocation,
   ownership extension, callbacks, initialization or constructor recovery. */
BHWeaponCommonState *__cdecl bh_weapon_common_state(BHEntity *);
/* Complete004309b0. Every nonzero signed delay decrements with x86 wrap,
   including negative values and LONG_MIN; zero retains every byte. */
int __cdecl bh_weapon_delay_tick(BHEntity *);
typedef struct BHWeaponReadyContext {
    BHSoundQueue *sound;
    unsigned long sound_resource; /* actual loaded004cda10 */
    unsigned long random_calls;
    int error;
} BHWeaponReadyContext;
/* Complete00431050, shared table method+11c. Zeros only five observed words,
   sets+100=-1, consumes exactly one CRT rand and requests the real queued sound
   at player+0c with velocity+48. Actual sound owner supplies level/pitch mapping;
   this does not play sound. Original discarded sound request result: a binding/
   backend failure is reported after preserving reset and random side effects.
   Borrowed player/weapon/sound lifetime spans the call. PC53 nearest/masked. */
int __cdecl bh_weapon_ready_reset(BHWeaponReadyContext *,BHEntity *weapon,
    const BHEntity *player);
#endif
