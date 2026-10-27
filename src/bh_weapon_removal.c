#include "bh_weapon_removal.h"
#include <stddef.h>
typedef char removed_packet_size[sizeof(BHWeaponRemovedPacket) == 8 ? 1 : -1];
typedef char removed_entity_offset[offsetof(BHWeaponRemovedPacket, entity) == 4 ? 1 : -1];
int __cdecl bh_weapon_target_removed(BHSoundQueue *sound, BHEntity *weapon, const BHWeaponRemovedPacket *packet)
{
    BHWeaponCommonState *state;
    long handle;
    if (!weapon || !packet)
        return 0;
    if (packet->event != 17)
        return 1;
    state = bh_weapon_common_state(weapon);
    if (packet->entity != state->hud.target)
        return 1;
    handle = state->word_100;
    state->hud.target = NULL;
    if (handle != -1)
    {
        if (!sound)
        {
            state->word_100 = -1;
            return 0;
        }
        bh_sound_stop(sound, (unsigned long)handle);
    }
    state->word_100 = -1;
    return 1;
}
