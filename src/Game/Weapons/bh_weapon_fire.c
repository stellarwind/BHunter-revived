#include "bh_weapon_fire.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
#include <stdlib.h>
static const char empty_text[] =
    "5Hello Mister,\r\n\r\nseems like you're\r\nfresh out of ammo\r\non that one!\r\n\r\nCome visit one of my\r\nshops for a refill.\r\n\r\n";
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static int valid(BHWeaponFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    return ctx && weapon_entity && entity && ctx->sound && bh_math_environment_supported();
}
static unsigned char target_id(BHWeaponFireContext *ctx, BHEntity *target)
{
    unsigned long i;
    unsigned char id = 255;
    if (!target)
        return id;
    if (target == ctx->local)
        return (unsigned char)ctx->local_id;
    for (i = 0; i < 8; i++)
        if (ctx->remote[i] == target)
            id = (unsigned char)i;
    return id;
}
void bh_weapon_fire_packet(BHWeaponFireContext *ctx, unsigned char a, unsigned char b, unsigned char d,
                           BHEntity *target)
{
    BHPacketWriter writer;
    unsigned char payload[5];
    if (!ctx->transport || ctx->local_id < 0)
        return;
    payload[0] = (unsigned char)ctx->local_id;
    payload[1] = a;
    payload[2] = b;
    payload[3] = d;
    payload[4] = target_id(ctx, target);
    if (bh_packet_begin(&writer, ctx->packet, sizeof(ctx->packet), 8, ctx->sender) &&
        bh_packet_append(&writer, payload, 5) && bh_packet_finish(&writer, &ctx->packet_bytes))
    {
        bh_network_send(ctx->transport, 0, ctx->packet, ctx->packet_bytes, 0);
        ++ctx->packets;
    }
    else
        ctx->error = 2;
}
int bh_weapon_fire_empty(BHWeaponFireContext *ctx, const BHEntity *entity)
{
    if (!ctx->messages || !ctx->empty_warning)
    {
        ctx->error = 2;
        return 0;
    }
    bh_sound_request(ctx->sound, ctx->empty_sound, .8f, 1.5f, &entity->position, &entity->origin);
    ++ctx->empty_cues;
    if (!*ctx->empty_warning)
    {
        bh_message_enqueue(ctx->messages, empty_text);
        *ctx->empty_warning = 1;
    }
    return 1;
}
int bh_weapon_fire_ballistic(BHWeaponFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    BHVec3 up, side, forward, right, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    unsigned long phase, i;
    int r;
    double sign, center, launch_x;
    if (!valid(ctx, weapon_entity, entity) || !ctx->projectiles)
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    phase = weapon_state->word_e8;
    ctx->error = 0;
    if (!(phase & 1))
    {
        if (!weapon_state->ammo)
            return 1;
        weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
        up = entity->basis[1];
        side = entity->basis[2];
        right = entity->basis[0];
        forward = entity->player.aim_direction;
        /* 0043066c/8b spill without popping: the side offset uses retained centers.
    0043071b/25 retain X as double across rand; Y/Z reload their float spills. */
        for (i = 0; i < 3; i++)
        {
            center = ((double)((float *)&entity->position)[i] - (double)((float *)&up)[i] * (-.5)) -
                     (double)((float *)&side)[i] * (-1.5);
            ((float *)&muzzle)[i] =
                stored((phase & 2) ? (double)((float *)&right)[i] + center : center - (double)((float *)&right)[i]);
        }
        launch_x = (double)entity->origin.x - (double)forward.x * (-150);
        velocity.y = stored((double)entity->origin.y - (double)forward.y * (-150));
        velocity.z = stored((double)entity->origin.z - (double)forward.z * (-150));
        r = rand();
        velocity.x = stored(launch_x - ((double)r - 16384) * (-1.0 / 8192));
        for (i = 1; i < 3; i++)
        {
            r = rand();
            ((float *)&velocity)[i] = stored((double)((float *)&velocity)[i] - ((double)r - 16384) * (-1.0 / 8192));
        }
        bh_sound_request(ctx->sound, ctx->ballistic_sound, .75f, 1.5f, &muzzle, &entity->origin);
        world = entity->world;
        bh_projectile_create_ballistic(ctx->projectiles, world, entity, &muzzle, &velocity, 2);
        if (ctx->peers > 1 && !weapon_state->delay)
        {
            bh_weapon_fire_packet(ctx, 4, 2, 1, NULL);
            weapon_state->delay = 8;
        }
        recoil = entity->player.recoil_velocity;
        for (i = 0; i < 3; i++)
            ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * (-3));
        entity->player.recoil_velocity = recoil;
        sign = (phase & 2) ? -.5 : .5;
        for (i = 0; i < 3; i++)
            ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&right)[i] * sign);
        entity->player.recoil_velocity = recoil;
        ++ctx->shots;
    }
    weapon_state->word_e8 = phase + 1;
    return ctx->error == 0;
}
int bh_weapon_fire_homing(BHWeaponFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    BHVec3 up, side, forward, muzzle, velocity, recoil;
    BHWorldGeometry *world;
    unsigned long phase, i;
    int r;
    if (!valid(ctx, weapon_entity, entity) || !ctx->projectiles)
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    phase = weapon_state->word_e8;
    ctx->error = 0;
    if (!(phase & 3))
    {
        if (!weapon_state->ammo)
            return 1;
        weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
        bh_sound_request(ctx->sound, ctx->homing_sound, .75f, 2, &entity->position, &entity->origin);
        up = entity->basis[1];
        side = entity->basis[2];
        forward = entity->player.aim_direction;
        for (i = 0; i < 3; i++)
        {
            ((float *)&muzzle)[i] =
                stored(((double)((float *)&entity->position)[i] - (double)((float *)&up)[i] * (-.5)) -
                       (double)((float *)&side)[i] * (-1.5));
            ((float *)&velocity)[i] =
                stored((double)((float *)&entity->origin)[i] - (double)((float *)&forward)[i] * (-120));
        }
        for (i = 0; i < 3; i++)
        {
            r = rand();
            ((float *)&velocity)[i] = stored((double)((float *)&velocity)[i] - ((double)r - 16384) * (-1.0 / 8192));
        }
        world = entity->world;
        bh_projectile_create_homing(ctx->projectiles, world, entity, &muzzle, &velocity, 2, weapon_state->hud.target);
        recoil = entity->player.recoil_velocity;
        for (i = 0; i < 3; i++)
            ((float *)&recoil)[i] = stored((double)((float *)&recoil)[i] - (double)((float *)&forward)[i] * 4);
        entity->player.recoil_velocity = recoil;
        if (ctx->peers > 1 && !weapon_state->delay)
        {
            bh_weapon_fire_packet(ctx, 2, 4, 2, weapon_state->hud.target);
            weapon_state->delay = 8;
        }
        ++ctx->shots;
    }
    weapon_state->word_e8 = phase + 1;
    return ctx->error == 0;
}
int bh_weapon_area_start(BHWeaponFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    if (!valid(ctx, weapon_entity, entity))
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    ctx->error = 0;
    if (weapon_state->word_e8)
        return 1;
    if (!weapon_state->ammo)
        return bh_weapon_fire_empty(ctx, entity);
    weapon_state->ammo = (long)((unsigned long)weapon_state->ammo - 1);
    weapon_state->word_e8 = 5;
    if (ctx->peers > 1)
        bh_weapon_fire_packet(ctx, 5, 1, 4, NULL);
    return ctx->error == 0;
}
int bh_weapon_area_tick(BHWeaponFireContext *ctx, BHEntity *weapon_entity, BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    BHVec3 forward, recoil;
    BHWorldGeometry *world;
    unsigned long phase, i;
    float strength;
    long distance;
    double gain;
    if (!valid(ctx, weapon_entity, entity) || !ctx->area)
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    phase = weapon_state->word_e8;
    ctx->error = 0;
    if (!phase)
        return 1;
    if (phase == 5)
    {
        bh_sound_request(ctx->sound, ctx->area_sound, 1, 1, &entity->position, &entity->origin);
        bh_sound_request(ctx->sound, ctx->area_sound, 1, .5f, &entity->position, &entity->origin);
    }
    forward = entity->player.aim_direction;
    distance = (long)(3UL - phase);
    strength = stored(1 - (double)(distance < 0 ? -(double)distance : distance) * .25);
    world = entity->world;
    bh_area_projectile_create(ctx->area, world, &entity->position, &forward, entity, strength);
    gain = ((phase & 1) - .5) * 6;
    recoil = entity->player.recoil_velocity;
    for (i = 0; i < 3; i++)
        ((float *)&recoil)[i] = stored((double)((float *)&forward)[i] * gain + ((float *)&recoil)[i]);
    entity->player.recoil_velocity = recoil;
    weapon_state->word_e8 = phase - 1;
    ++ctx->bursts;
    return 1;
}
