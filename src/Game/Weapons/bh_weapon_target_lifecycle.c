#include "bh_weapon_target_lifecycle.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stddef.h>
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static int fail(BHWeaponTargetContext *ctx, BHWeaponTargetError e)
{
    if (ctx)
        ctx->error = e;
    return 0;
}
static int finite_vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
BHVec3 *bh_weapon_target_player_aim(BHEntity *entity)
{
    return entity ? &entity->player.aim_direction : NULL;
}
static void clear(BHWeaponCommonState *weapon_state)
{
    weapon_state->word_e8 = weapon_state->word_f4 = weapon_state->word_f0 = 0;
    weapon_state->hud.target = NULL;
    weapon_state->hud.charge = 0;
}
static void stop(BHWeaponTargetContext *ctx, unsigned long h)
{
    if (h != BH_SOUND_INVALID)
    {
        ++ctx->stops;
        if (bh_sound_stop(ctx->sound, h) < 0)
            ctx->error = BH_WEAPON_TARGET_SOUND;
    }
}
int bh_weapon_target_ready(BHWeaponTargetContext *ctx, BHEntity *weapon_entity, const BHEntity *entity)
{
    BHWeaponCommonState *weapon_state = bh_weapon_common_state(weapon_entity);
    int value;
    float pitch;
    unsigned long h;
    if (!ctx || !ctx->sound || !weapon_state || !entity)
        return fail(ctx, BH_WEAPON_TARGET_ARGUMENT);
    ctx->error = BH_WEAPON_TARGET_OK;
    clear(weapon_state);
    weapon_state->word_100 = -1;
    value = rand();
    ++ctx->random_calls;
    pitch = stored(.9 - (double)value * (-.000006103515625));
    ++ctx->sound_requests;
    h = bh_sound_request(ctx->sound, ctx->ready_sound, .8f, pitch, &entity->position, &entity->origin);
    if (h == BH_SOUND_INVALID && ctx->sound->error != BH_SOUND_PRIORITY)
        ctx->error = BH_WEAPON_TARGET_SOUND;
    return ctx->error == BH_WEAPON_TARGET_OK;
}
int bh_weapon_target_cancel(BHWeaponTargetContext *ctx, BHEntity *weapon_entity)
{
    BHWeaponCommonState *weapon_state = bh_weapon_common_state(weapon_entity);
    if (!ctx || !ctx->sound || !weapon_state)
        return fail(ctx, BH_WEAPON_TARGET_ARGUMENT);
    ctx->error = BH_WEAPON_TARGET_OK;
    clear(weapon_state);
    stop(ctx, (unsigned long)weapon_state->word_100);
    weapon_state->word_100 = -1;
    return ctx->error == BH_WEAPON_TARGET_OK;
}
static int query(BHWeaponTargetContext *ctx, BHEntity *entity, BHEntityPositionPacket *p)
{
    if (!entity || !entity->query)
        return fail(ctx, BH_WEAPON_TARGET_CALLBACK);
    ++ctx->queries;
    entity->query(entity, p);
    return finite_vector(&p->position) ? 1 : fail(ctx, BH_WEAPON_TARGET_NUMERICAL);
}
int bh_weapon_target_update(BHWeaponTargetContext *ctx, BHEntity *weapon_entity, const BHEntity *entity)
{
    BHWeaponCommonState *weapon_state = bh_weapon_common_state(weapon_entity);
    const BHVec3 *aim;
    BHEntity *e, *next;
    BHVec3 d;
    union {
        BHEntityPositionPacket packet;
        unsigned long words[10];
    } packet;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    unsigned long visited = 0, h, charge;
    float best = 150, dot, angle, value;
    double x, y, z, projection, length;
    int result;
    if (!ctx || !ctx->sound || !weapon_state || !entity)
        return fail(ctx, BH_WEAPON_TARGET_ARGUMENT);
    ctx->error = BH_WEAPON_TARGET_OK;
    memcpy(&value, &weapon_state->word_f0, 4);
    if (value > 0)
    {
        value = stored((double)value - 1.0 / 30.0);
        memcpy(&weapon_state->word_f0, &value, 4);
    }
    memcpy(&value, &weapon_state->word_f4, 4);
    if (value > 0)
    {
        value = stored((double)value - 1.0 / 30.0);
        memcpy(&weapon_state->word_f4, &value, 4);
    }
    packet.packet.event = 3;
    if (!weapon_state->ammo)
    {
        h = (unsigned long)weapon_state->word_100;
        weapon_state->hud.target = NULL;
        stop(ctx, h);
        weapon_state->word_100 = -1;
        return ctx->error == BH_WEAPON_TARGET_OK;
    }
    if (!weapon_state->hud.target)
    {
        e = ctx->actors_owner ? *ctx->actors_owner : ctx->actors;
        while (e)
        {
            if (++visited > ctx->max_actors)
                return fail(ctx, BH_WEAPON_TARGET_LIMIT);
            if (!query(ctx, e, &packet.packet))
                return 0;
            aim = bh_weapon_target_player_aim((BHEntity *)entity);
            if (!finite_vector(&entity->position) || !finite_vector(aim))
                return fail(ctx, BH_WEAPON_TARGET_NUMERICAL);
            d.x = stored((double)packet.packet.position.x - entity->position.x);
            d.y = stored((double)packet.packet.position.y - entity->position.y);
            d.z = stored((double)packet.packet.position.z - entity->position.z);
            projection = ((double)aim->y * d.y + (double)aim->x * d.x) + (double)aim->z * d.z;
            dot = stored(projection);
            if (projection >= 0 && dot < best)
            {
                length = sqrt(((double)d.x * d.x + (double)d.y * d.y) + (double)d.z * d.z);
                if (!(1.0 - (double)dot / length >= .02))
                {
                    if (!ctx->world)
                        return fail(ctx, BH_WEAPON_TARGET_WORLD);
                    self.kind = BH_WORLD_HIT_ACTOR;
                    self.object.actor = (BHEntity *)entity;
                    memset(&hit, 0, sizeof(hit));
                    ++ctx->traces;
                    result = bh_world_segment_run(ctx->world, &self, &weapon_state->cached_world, &entity->position,
                                                  &packet.packet.position, -1, &hit);
                    if (result < 0)
                        ctx->error = BH_WEAPON_TARGET_WORLD;
                    if (!result)
                    {
                        weapon_state->hud.target = e;
                        best = dot;
                    }
                }
            }
            memcpy(&next, &e->actor.global_next, 4);
            e = next;
        }
        weapon_state->hud.charge = 0;
        return ctx->error == BH_WEAPON_TARGET_OK;
    }
    charge = (unsigned long)weapon_state->hud.charge + 1UL;
    weapon_state->hud.charge = (long)charge;
    if ((long)charge > 16 && (long)charge < 48)
    {
        if (charge & 2)
        {
            ++ctx->sound_requests;
            if (bh_sound_request(ctx->sound, ctx->beep_sound, .5f, 1, NULL, NULL) == BH_SOUND_INVALID)
                ctx->error = BH_WEAPON_TARGET_SOUND;
        }
    }
    else if ((long)charge == 48)
    {
        ++ctx->sound_requests;
        h = bh_sound_request(ctx->sound, ctx->lock_sound, .5f, 1.7f, NULL, NULL);
        weapon_state->word_100 = (long)h;
        if (h == BH_SOUND_INVALID)
            ctx->error = BH_WEAPON_TARGET_SOUND;
    }
    if (!query(ctx, weapon_state->hud.target, &packet.packet))
        return 0;
    aim = bh_weapon_target_player_aim((BHEntity *)entity);
    if (!finite_vector(&entity->position) || !finite_vector(aim))
        return fail(ctx, BH_WEAPON_TARGET_NUMERICAL);
    x = (double)packet.packet.position.x - entity->position.x;
    y = (double)packet.packet.position.y - entity->position.y;
    z = (double)packet.packet.position.z - entity->position.z;
    d.x = stored(x);
    d.y = stored(y);
    length = sqrt((x * d.x + y * d.y) + z * z);
    projection = ((double)aim->z * z + (double)aim->y * d.y) + (double)aim->x * d.x;
    angle = stored(1.0 - projection / length);
    if (projection >= 0 && !(projection > 150.0) && !((double)angle > .075) &&
        (weapon_state->hud.charge >= 48 || !((double)angle > .02)))
        return ctx->error == BH_WEAPON_TARGET_OK;
    h = (unsigned long)weapon_state->word_100;
    weapon_state->hud.target = NULL;
    stop(ctx, h);
    weapon_state->word_100 = -1;
    return ctx->error == BH_WEAPON_TARGET_OK;
}
