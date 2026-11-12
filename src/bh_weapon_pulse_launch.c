#include "bh_weapon_pulse_launch.h"
#include "bh_round.h"
#include "bh_fp.h"
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
    BHWeaponFireContext *f;
    BHVec3 up, forward, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    float timer;
    unsigned long i;
    if (!ctx || !ctx->fire || !ctx->projectile || !weapon_entity || !entity || !bh_math_environment_supported())
        return 0;
    f = ctx->fire;
    if (!f->sound)
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    f->error = 0;
    memcpy(&timer, &weapon_state->word_f0, 4);
    if (!(timer <= 0))
        return 1;
    if (!weapon_state->ammo)
        return bh_weapon_fire_empty(f, entity);
    weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
    timer = 1.5f;
    memcpy(&weapon_state->word_f0, &timer, 4);
    bh_sound_request(f->sound, ctx->sound, 1, .8f, &entity->position, &entity->origin);
    up = entity->basis[1];
    forward = entity->player.aim_direction;
    for (i = 0; i < 3; i++)
    {
        ((float *)&muzzle)[i] = stored((double)((float *)&entity->position)[i] + ((float *)&up)[i]);
        ((float *)&velocity)[i] = stored((double)((float *)&forward)[i] * 15);
    }
    if (f->peers > 1 && ctx->projectile->sync_identity)
        *ctx->projectile->sync_identity = ((unsigned long)f->local_id | 0x4000UL) << 16 | f->sender;
    world = entity->world;
    bh_pulse_projectile_create(ctx->projectile, world, entity, &muzzle, &entity->origin, &velocity);
    if (f->peers > 1)
        bh_weapon_fire_packet(f, 1, 0, 8, NULL);
    recoil = entity->player.recoil_velocity;
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-6));
    entity->player.recoil_velocity = recoil;
    ++f->shots;
    return f->error == 0;
}
