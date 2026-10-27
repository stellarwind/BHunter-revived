#ifndef BH_WEAPON_EMPTY_H
#define BH_WEAPON_EMPTY_H
#include "bh_weapon_factory.h"
#include "bh_messages.h"
typedef enum BHWeaponEmptyError
{
    BH_WEAPON_EMPTY_OK,
    BH_WEAPON_EMPTY_ARGUMENT,
    BH_WEAPON_EMPTY_METHOD,
    BH_WEAPON_EMPTY_SOUND,
    BH_WEAPON_EMPTY_MESSAGE,
    BH_WEAPON_EMPTY_STALE
} BHWeaponEmptyError;
typedef struct BHWeaponEmptyContext
{
    BHSoundQueue *sound;
    unsigned long shift_sound; /* actual loaded 004cda10 */
    BHMessageState *messages;
    long *warning_seen; /* live original 004dfb24 owner */
    unsigned long requests, warnings;
    BHWeaponEmptyError error;
} BHWeaponEmptyContext;
/* Complete +124 empty-ammo methods 0042f730 (indices0/1), 004309d0
   (index2), 00432560 (index4). Each first mutates real weapon fields, then
   only when signed ammo is zero requests actual queued sound .8/1.5 using
   live player pose/velocity; one shared warning is appended, then flag set1.
   Original ignores sound/message returns; source reports failures after the
   same prior side effects. No firing behavior is claimed. */
int __cdecl bh_weapon_empty_trigger(BHWeaponEmptyContext *, unsigned long method_identity, BHEntity *weapon,
                                    const BHEntity *player);
/* Complete index9 +124 00433dc0. Positive player+240 timer suppresses action.
   Otherwise empty ammo uses the same sound/warning path; nonzero signed ammo
   decrements once and sets player+240/+248=15, +244=1. The player is mutable
   authoritative entity storage. */
int __cdecl bh_weapon_index9_trigger(BHWeaponEmptyContext *, BHEntity *weapon, BHEntity *player);
/* Resolve one live registry allocation and dispatch its verified +124 method.
   All other identities return METHOD, with no silent no-op. */
int __cdecl bh_weapon_empty_dispatch(BHWeaponEmptyContext *, BHWeaponRegistry *, unsigned int selection,
                                     BHEntity *player);
extern const char bh_weapon_empty_warning[];
#endif
