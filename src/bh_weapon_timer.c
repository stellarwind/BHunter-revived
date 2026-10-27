#include "bh_weapon_timer.h"
#include "bh_round.h"
#include <string.h>
int __cdecl bh_weapon_single_timer_tick(BHEntity *weapon)
{
    BHWeaponCommonState *state = bh_weapon_common_state(weapon);
    float value;
    if (!state)
        return 0;
    memcpy(&value, &state->word_f0, 4);
    if (value > 0)
    {
        bh_round_float(&value, (double)value - 0.033333333333333333);
        memcpy(&state->word_f0, &value, 4);
    }
    return 1;
}
