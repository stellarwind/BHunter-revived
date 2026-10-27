#include "bh_player_reset.h"
#include <string.h>

typedef char reset_player_table_fits[sizeof(BHEntity) >= 0x2bb ? 1 : -1];
typedef char reset_weapon_state_offset[sizeof(void *) == 4 ? 1 : -1];

static void put_word(BHEntity *player, unsigned int offset, unsigned short value)
{
    memcpy((unsigned char *)player + offset, &value, sizeof(value));
}
static void put_long(BHEntity *player, unsigned int offset, unsigned long value)
{
    memcpy((unsigned char *)player + offset, &value, sizeof(value));
}
static unsigned long get_long(BHEntity *player, unsigned int offset)
{
    unsigned long value;
    memcpy(&value, (unsigned char *)player + offset, sizeof(value));
    return value;
}
static int player_live(BHEntityPool *pool, BHEntity *player)
{
    BHEntity *cursor;
    unsigned long count;
    if (!pool || !player)
        return 0;
    cursor = pool->active;
    count = 0;
    while (cursor && count++ < pool->used)
    {
        if (cursor == player)
            return 1;
        cursor = cursor->next;
    }
    return 0;
}
static void stop_field(BHPlayerReset *reset, BHEntity *player, unsigned int offset)
{
    unsigned long handle = get_long(player, offset);
    if (handle != BH_SOUND_INVALID)
    {
        ++reset->attempted_stops;
        if (!bh_sound_stop(reset->sound, handle))
            ++reset->failed_stops;
    }
    put_long(player, offset, BH_SOUND_INVALID);
}
static void grant(BHWeaponObjectState *state, long count)
{
    long ceiling;
    state->common.available_ec = 1;
    state->common.ammo = count;
    ceiling = state->common.ammo_limit;
    if (ceiling < count)
        state->common.ammo = ceiling;
}
int __cdecl bh_player_reset(BHPlayerReset *reset)
{
    BHEntity *player, *entity, *weapons[10];
    BHWeaponObjectState *states[10];
    unsigned int i;
    if (!reset)
        return 0;
    reset->attempted_stops = 0;
    reset->failed_stops = 0;
    reset->error = BH_PLAYER_RESET_ARGUMENT;
    if (!reset->current_player || !reset->weapons || !reset->sound || !reset->frame_count)
        return 0;
    player = *reset->current_player; /* original 004dfab8 is loaded only once */
    reset->error = BH_PLAYER_RESET_BINDING;
    if (!player_live(reset->weapons->pool, player))
        return 0;
    for (i = 0; i < 10; i++)
    {
        if (!bh_weapon_registry_resolve(reset->weapons, i, &entity, &states[i]))
            return 0;
        memcpy(&weapons[i], (unsigned char *)player + 0x258 + i * 4, 4);
        if (weapons[i] != entity)
            return 0;
    }
    put_word(player, 0x2b4, 0);
    put_long(player, 0x23c, 0);
    stop_field(reset, player, 0x228);
    stop_field(reset, player, 0x22c);
    stop_field(reset, player, 0x224);
    put_long(player, 0x254, 0);
    put_long(player, 0x250, 0x3f800000UL);
    put_long(player, 0x24c, 0);
    ((unsigned char *)player)[0x2b8] = 0;
    ((unsigned char *)player)[0x2b9] = 0;
    ((unsigned char *)player)[0x2ba] = 0;
    put_long(player, 0x230, 10000);
    put_long(player, 0x234, 0);
    put_long(player, 0x238, 0);
    *reset->frame_count = 0;
    put_long(player, 0x248, 0);
    put_long(player, 0x244, 0);
    put_long(player, 0x240, 0);
    for (i = 0; i < 10; i++)
    {
        states[i]->common.available_ec = 0;
        states[i]->common.ammo = 0;
    }
    ((unsigned char *)player)[0x2b6] = 0;
    ((unsigned char *)player)[0x2b7] = 9;
    grant(states[0], 300);
    grant(states[9], 1);
    reset->error = reset->failed_stops ? BH_PLAYER_RESET_SOUND : BH_PLAYER_RESET_OK;
    return reset->error == BH_PLAYER_RESET_OK;
}
