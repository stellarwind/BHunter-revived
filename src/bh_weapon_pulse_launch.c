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
int __cdecl bh_weapon_pulse_launch(BHWeaponPulseLaunchContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
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
    memcpy(&up, (unsigned char *)entity + 0x24, 12);
    memcpy(&forward, (unsigned char *)entity + 0x280, 12);
    for (i = 0; i < 3; i++)
    {
        ((float *)&muzzle)[i] = stored((double)((float *)&entity->position)[i] + ((float *)&up)[i]);
        ((float *)&velocity)[i] = stored((double)((float *)&forward)[i] * 15);
    }
    if (f->peers > 1 && ctx->projectile->sync_identity)
        *ctx->projectile->sync_identity = ((unsigned long)f->local_id | 0x4000UL) << 16 | f->sender;
    memcpy(&world, (unsigned char *)entity + 0xc4, 4);
    bh_pulse_projectile_create(ctx->projectile, world, entity, &muzzle, &entity->origin, &velocity);
    if (f->peers > 1)
        bh_weapon_fire_packet(f, 1, 0, 8, NULL);
    memcpy(&recoil, (unsigned char *)entity + 0x2a4, 12);
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-6));
    memcpy((unsigned char *)entity + 0x2a4, &recoil, 12);
    ++f->shots;
    return f->error == 0;
}
