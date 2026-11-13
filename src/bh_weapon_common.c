#include "bh_weapon_common.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static_assert(offsetof(BHWeaponCommonState, cached_world) == 0x1c, "weapon cached world offset");

static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");
static_assert(offsetof(BHEntity, origin) == 0x48, "BHEntity.origin offset");
static_assert(sizeof(BHWeaponCommonState) == 0x30, "BHWeaponCommonState size");
static_assert(offsetof(BHWeaponCommonState, hud) == 0x10, "BHWeaponCommonState.hud offset");
static_assert(offsetof(BHWeaponCommonState, ammo) == 0x24, "BHWeaponCommonState.ammo offset");
static_assert(offsetof(BHWeaponCommonState, delay) == 0x2c, "BHWeaponCommonState.delay offset");
BHWeaponCommonState *bh_weapon_common_state(BHEntity *weapon)
{
    return weapon ? (BHWeaponCommonState *)(weapon->payload + 4) : NULL;
}
int bh_weapon_delay_tick(BHEntity *weapon)
{
    BHWeaponCommonState *state = bh_weapon_common_state(weapon);
    if (!state)
        return 0;
    if (state->delay)
        state->delay = (long)((unsigned long)state->delay - 1UL);
    return 1;
}
int bh_weapon_ready_reset(BHWeaponReadyContext *ctx, BHEntity *weapon, const BHEntity *player)
{
    BHWeaponCommonState *state = bh_weapon_common_state(weapon);
    unsigned long handle;
    float pitch;
    int value;
    if (!ctx || !ctx->sound || !state || !player)
    {
        if (ctx)
            ctx->error = 1;
        return 0;
    }
    state->word_e8 = 0;
    state->word_f4 = 0;
    state->word_f0 = 0;
    state->hud.target = NULL;
    state->hud.charge = 0;
    state->word_100 = -1;
    value = rand();
    ctx->random_calls = 1;
    bh_round_float(&pitch, .9 - (double)value * (-0.000006103515625));
    handle = bh_sound_request(ctx->sound, ctx->sound_resource, .9f, pitch, &player->position, &player->origin);
    /* A lower-priority cue may be dropped while earlier selections play. */
    ctx->error = handle == BH_SOUND_INVALID && ctx->sound->error != BH_SOUND_PRIORITY ? 2 : 0;
    return !ctx->error;
}
