#ifndef BH_PLAYER_RESET_H
#define BH_PLAYER_RESET_H
#include "bh_weapon_factory.h"

typedef enum BHPlayerResetError
{
    BH_PLAYER_RESET_OK,
    BH_PLAYER_RESET_ARGUMENT,
    BH_PLAYER_RESET_BINDING,
    BH_PLAYER_RESET_SOUND
} BHPlayerResetError;
typedef struct BHPlayerReset
{
    BHEntity **current_player; /* original 004dfab8, captured once */
    BHWeaponRegistry *weapons; /* ten real factory-owned allocations */
    BHSoundQueue *sound;       /* actual 0047c140 source owner */
    long *frame_count;         /* original 004df6b4 */
    unsigned long attempted_stops, failed_stops;
    BHPlayerResetError error;
} BHPlayerReset;
/* Complete 00421bc0. Validate live player/ten exact weapon identities first,
   then preserve the original sound-stop and field-write order. A failed sound
   stop never aborts later writes; return 0 reports it after full reset. Signed
   ammo ceilings retain negative limits rather than inventing a zero floor. */
int __cdecl bh_player_reset(BHPlayerReset *);
#endif
