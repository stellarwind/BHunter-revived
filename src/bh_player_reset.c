#include "bh_player_reset.h"

static_assert(sizeof(BHEntity) >= 0x2bb, "reset player table fits");
static_assert(sizeof(void *) == 4, "reset weapon state offset");

static int player_live(BHEntityPool *pool, BHEntity *player)
{
    return bh_entity_is_active(pool, player);
}
static void stop_sound(BHPlayerReset *reset, unsigned long handle)
{
    if (handle != BH_SOUND_INVALID)
    {
        ++reset->attempted_stops;
        if (!bh_sound_stop(reset->sound, handle) && reset->sound->error != BH_SOUND_STALE)
            ++reset->failed_stops;
    }
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
int bh_player_reset(BHPlayerReset *reset)
{
    BHEntity *player, *entity;
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
        if (player->player.weapons[i] != entity)
            return 0;
    }
    player->player.disabled_frames = 0;
    player->player.damage = 0;
    stop_sound(reset, player->player.ground_sound);
    player->player.ground_sound = BH_SOUND_INVALID;
    stop_sound(reset, player->player.shield_sound);
    player->player.shield_sound = BH_SOUND_INVALID;
    stop_sound(reset, player->player.death_sound);
    player->player.death_sound = BH_SOUND_INVALID;
    player->player.damage_flash = 0;
    player->player.instability = 1;
    player->player.wear = 0;
    player->player.frame_phase = 0;
    player->player.cash_flash = 0;
    player->player.fine_flash = 0;
    player->player.cash = 10000;
    player->player.fine = 0;
    player->player.bounty_pending = 0;
    *reset->frame_count = 0;
    player->player.shield_pitch_time = 0;
    player->player.shield_fade = 0;
    player->player.shield_time = 0;
    for (i = 0; i < 10; i++)
    {
        /* Target/arc charge voices belong to the weapon, not the player's
           four sound fields. Reset before ready methods overwrite the handle. */
        stop_sound(reset, (unsigned long)states[i]->common.word_100);
        states[i]->common.word_100 = -1;
        states[i]->common.word_e8 = 0;
        states[i]->common.word_f0 = 0;
        states[i]->common.word_f4 = 0;
        states[i]->common.hud.target = NULL;
        states[i]->common.hud.charge = 0;
        states[i]->common.word_108 = 0;
        states[i]->common.available_ec = 0;
        states[i]->common.ammo = 0;
    }
    player->player.selected_weapon[0] = 0;
    player->player.selected_weapon[1] = 9;
    grant(states[0], 300);
    grant(states[9], 1);
    reset->error = reset->failed_stops ? BH_PLAYER_RESET_SOUND : BH_PLAYER_RESET_OK;
    return reset->error == BH_PLAYER_RESET_OK;
}
