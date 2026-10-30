#include "bh_weapon_common.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef char weapon_payload_offset[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char weapon_player_velocity_offset[offsetof(BHEntity, origin) == 0x48 ? 1 : -1];
typedef char weapon_common_size[sizeof(BHWeaponCommonState) == 0x30 ? 1 : -1];
typedef char weapon_hud_offset[offsetof(BHWeaponCommonState, hud) == 0x10 ? 1 : -1];
typedef char weapon_ammo_offset[offsetof(BHWeaponCommonState, ammo) == 0x24 ? 1 : -1];
typedef char weapon_delay_offset[offsetof(BHWeaponCommonState, delay) == 0x2c ? 1 : -1];
BHWeaponCommonState *__cdecl bh_weapon_common_state(BHEntity *weapon)
{
    return weapon ? (BHWeaponCommonState *)(weapon->payload + 4) : NULL;
}
int __cdecl bh_weapon_delay_tick(BHEntity *weapon)
{
    BHWeaponCommonState *state = bh_weapon_common_state(weapon);
    if (!state)
        return 0;
    if (state->delay)
        state->delay = (long)((unsigned long)state->delay - 1UL);
    return 1;
}
int __cdecl bh_weapon_ready_reset(BHWeaponReadyContext *c, BHEntity *weapon, const BHEntity *player)
{
    BHWeaponCommonState *state = bh_weapon_common_state(weapon);
    unsigned long handle;
    float pitch;
    int value;
    if (!c || !c->sound || !state || !player)
    {
        if (c)
            c->error = 1;
        return 0;
    }
    state->word_e8 = 0;
    state->word_f4 = 0;
    state->word_f0 = 0;
    state->hud.target = NULL;
    state->hud.charge = 0;
    state->word_100 = -1;
    value = rand();
    c->random_calls = 1;
    bh_round_float(&pitch, .9 - (double)value * (-0.000006103515625));
    handle = bh_sound_request(c->sound, c->sound_resource, .9f, pitch, &player->position, &player->origin);
    /* A lower-priority cue may be dropped while earlier selections play. */
    c->error = handle == BH_SOUND_INVALID && c->sound->error != BH_SOUND_PRIORITY ? 2 : 0;
    return !c->error;
}
