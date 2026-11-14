#include "bh_weapon_effect_fire.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static int valid(BHWeaponEffectFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    return ctx && ctx->fire && ctx->projectile && ctx->fire->sound && weapon_entity && entity && bh_math_environment_supported();
}
static void sync(BHWeaponEffectFireContext *ctx)
{
    BHWeaponFireContext *f = ctx->fire;
    if (f->peers > 1 && ctx->projectile->pending_identity)
        *ctx->projectile->pending_identity = ((unsigned long)f->local_id | 0x4000UL) << 16 | f->sender;
}
static int alternate(BHWeaponEffectFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity, int split)
{
    BHWeaponCommonState *weapon_state = bh_weapon_common_state(weapon_entity);
    BHWeaponFireContext *f = ctx->fire;
    BHVec3 right, up, side, forward, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    BHEntity *target;
    unsigned long i, phase;
    float timer;
    double dot, gain;
    phase = weapon_state->word_e8;
    memcpy(&timer, phase ? &weapon_state->word_f0 : &weapon_state->word_f4, 4);
    f->error = 0;
    if (timer > 0)
        return 1;
    if (!weapon_state->ammo)
        return bh_weapon_fire_empty(f, entity);
    target = weapon_state->hud.charge >= 48 ? weapon_state->hud.target : NULL;
    weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
    weapon_state->word_e8 = phase ? 0 : 1;
    timer = split ? .3f : 1;
    memcpy(phase ? &weapon_state->word_f0 : &weapon_state->word_f4, &timer, 4);
    right = entity->basis[0];
    up = entity->basis[1];
    side = entity->basis[2];
    forward = entity->player.aim_direction;
    gain = phase ? 1 : -1;
    for (i = 0; i < 3; i++)
    {
        double coordinate;
        if (!phase)
            coordinate =
                (double)((float *)&entity->position)[i] + ((double)((float *)&up)[i] * .5 - ((float *)&right)[i]);
        else if (split)
            coordinate =
                ((double)((float *)&entity->position)[i] + (double)((float *)&up)[i] * .5) + ((float *)&right)[i];
        else
            coordinate =
                ((double)((float *)&entity->position)[i] + ((float *)&right)[i]) + (double)((float *)&up)[i] * .5;
        ((float *)&muzzle)[i] = stored(coordinate);
    }
    world = entity->world;
    sync(ctx);
    if (split)
    {
        for (i = 0; i < 3; i++)
            ((float *)&velocity)[i] =
                stored((double)((float *)&entity->origin)[i] - (double)((float *)&forward)[i] * (-60));
        dot = ((double)side.y * velocity.y + (double)side.z * velocity.z) + (double)side.x * velocity.x;
        if (dot < 10)
        {
            dot = 10 - dot;
            for (i = 0; i < 3; i++)
                ((float *)&velocity)[i] = stored((double)((float *)&side)[i] * dot + ((float *)&velocity)[i]);
        }
        bh_effect_split_create(ctx->projectile, world, entity, &muzzle, &velocity, 3, 10, ctx->split_model, 1);
        if (f->peers > 1)
            bh_weapon_fire_packet(f, 1, 0, 6, NULL);
    }
    else
    {
        bh_effect_orbit_create(ctx->projectile, world, entity, &muzzle, &entity->origin, &right, &up, &forward, 8,
                               target);
        if (f->peers > 1)
            bh_weapon_fire_packet(f, 1, 0, 3, target);
    }
    recoil = entity->player.recoil_velocity;
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] =
            stored(((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (split ? -3 : -4)) -
                   (double)((float *)&right)[i] * (phase ? -.5 : .5));
    entity->player.recoil_velocity = recoil;
    ++f->shots;
    return f->error == 0;
}
int bh_weapon_effect_orbit_fire(BHWeaponEffectFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    if (!valid(ctx, weapon_entity, entity) || !ctx->projectile->orbit)
        return 0;
    return alternate(ctx, weapon_entity, entity, 0);
}
int bh_weapon_effect_split_fire(BHWeaponEffectFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    if (!valid(ctx, weapon_entity, entity) || !ctx->projectile->split)
        return 0;
    return alternate(ctx, weapon_entity, entity, 1);
}
int bh_weapon_effect_accelerated_fire(BHWeaponEffectFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    BHWeaponFireContext *f;
    BHVec3 up, forward, muzzle, acc, recoil;
    BHWorldGeometry *world;
    float timer;
    unsigned long i;
    if (!valid(ctx, weapon_entity, entity) || !ctx->projectile->accelerated)
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    f = ctx->fire;
    f->error = 0;
    memcpy(&timer, &weapon_state->word_f0, 4);
    if (!(timer <= 0))
        return 1;
    if (!weapon_state->ammo)
        return bh_weapon_fire_empty(f, entity);
    weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
    timer = 2;
    memcpy(&weapon_state->word_f0, &timer, 4);
    bh_sound_request(f->sound, ctx->shot_sound, 1, .6f, &entity->position, &entity->origin);
    bh_sound_request(f->sound, ctx->shot_sound, 1, 1.3f, &entity->position, &entity->origin);
    up = entity->basis[1];
    forward = entity->player.aim_direction;
    for (i = 0; i < 3; i++)
    {
        ((float *)&muzzle)[i] = stored((double)((float *)&entity->position)[i] + ((float *)&up)[i]);
        ((float *)&acc)[i] = stored((double)((float *)&forward)[i] * 20);
    }
    acc.y = stored((double)forward.y * 20 - (-2));
    sync(ctx);
    world = entity->world;
    bh_effect_accelerated_create(ctx->projectile, world, entity, &muzzle, &entity->origin, &acc);
    if (f->peers > 1)
        bh_weapon_fire_packet(f, 1, 0, 7, NULL);
    recoil = entity->player.recoil_velocity;
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-10));
    entity->player.recoil_velocity = recoil;
    ++f->shots;
    return f->error == 0;
}
