#include "bh_weapon_pulse_launch.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
int bh_weapon_pulse_launch(BHWeaponPulseLaunchContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    BHWeaponFireContext *weapon_fire_context;
    BHVec3 up, forward, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    float timer;
    unsigned long i;
    if (!ctx || !ctx->fire || !ctx->projectile || !weapon_entity || !entity || !bh_math_environment_supported())
        return 0;
    weapon_fire_context = ctx->fire;
    if (!weapon_fire_context->sound)
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    weapon_fire_context->error = 0;
    memcpy(&timer, &weapon_state->word_f0, 4);
    if (!(timer <= 0))
        return 1;
    if (!weapon_state->ammo)
        return bh_weapon_fire_empty(weapon_fire_context, entity);
    weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
    timer = 1.5f;
    memcpy(&weapon_state->word_f0, &timer, 4);
    bh_sound_request(weapon_fire_context->sound, ctx->sound, 1, .8f, &entity->position, &entity->origin);
    up = entity->basis[1];
    forward = entity->player.aim_direction;
    for (i = 0; i < 3; i++)
    {
        ((float *)&muzzle)[i] = stored((double)((float *)&entity->position)[i] + ((float *)&up)[i]);
        ((float *)&velocity)[i] = stored((double)((float *)&forward)[i] * 15);
    }
    if (weapon_fire_context->peers > 1 && ctx->projectile->sync_identity)
        *ctx->projectile->sync_identity = ((unsigned long)weapon_fire_context->local_id | 0x4000UL) << 16 | weapon_fire_context->sender;
    world = entity->world;
    bh_pulse_projectile_create(ctx->projectile, world, entity, &muzzle, &entity->origin, &velocity);
    if (weapon_fire_context->peers > 1)
        bh_weapon_fire_packet(weapon_fire_context, 1, 0, 8, NULL);
    recoil = entity->player.recoil_velocity;
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-6));
    entity->player.recoil_velocity = recoil;
    ++weapon_fire_context->shots;
    return weapon_fire_context->error == 0;
}
