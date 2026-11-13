#include "bh_weapon_homing_track.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <math.h>
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static int query(BHWeaponTargetContext *ctx, BHEntity *entity, BHEntityPositionPacket *p)
{
    if (!entity || !entity->query)
    {
        ctx->error = BH_WEAPON_TARGET_CALLBACK;
        return 0;
    }
    ++ctx->queries;
    entity->query(entity, p);
    return 1;
}
int bh_weapon_homing_track(BHWeaponTargetContext *ctx, BHEntity *weapon_entity, const BHEntity *entity)
{
    BHWeaponCommonState *weapon_state;
    BHVec3 aim, d;
    BHEntityPositionPacket packet;
    BHEntity *target;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    float cooldown, dot, angle, best = 150;
    double x, y, z, projection, length;
    unsigned long n = 0;
    int result;
    if (!ctx || !weapon_entity || !entity || !bh_math_environment_supported())
        return 0;
    weapon_state = bh_weapon_common_state(weapon_entity);
    ctx->error = BH_WEAPON_TARGET_OK;
    if (weapon_state->delay)
        weapon_state->delay = (long)((unsigned long)weapon_state->delay - 1);
    memcpy(&cooldown, &weapon_state->word_f0, 4);
    if (cooldown > 0)
    {
        cooldown = stored((double)cooldown - 1.0 / 30);
        memcpy(&weapon_state->word_f0, &cooldown, 4);
    }
    memcpy(&cooldown, &weapon_state->word_f4, 4);
    if (cooldown > 0)
    {
        cooldown = stored((double)cooldown - 1.0 / 30);
        memcpy(&weapon_state->word_f4, &cooldown, 4);
    }
    packet.event = 3;
    target = weapon_state->hud.target;
    if (target)
    {
        weapon_state->hud.charge = (long)((unsigned long)weapon_state->hud.charge + 1);
        if (!query(ctx, target, &packet))
            return 0;
        d.x = stored((double)packet.position.x - entity->position.x);
        d.y = stored((double)packet.position.y - entity->position.y);
        d.z = stored((double)packet.position.z - entity->position.z);
        aim = entity->player.aim_direction;
        projection = ((double)aim.x * d.x + (double)aim.y * d.y) + (double)aim.z * d.z;
        dot = stored(projection);
        length = sqrt(((double)d.z * d.z + (double)d.y * d.y) + (double)d.x * d.x);
        angle = stored(1 - (double)dot / length);
        if (dot < 0 || dot > 150 || angle > .4 || (weapon_state->hud.charge < 2 && angle > .3))
            weapon_state->hud.target = NULL;
        return 1;
    }
    target = ctx->actors_owner ? *ctx->actors_owner : ctx->actors;
    while (target)
    {
        if (++n > ctx->max_actors)
        {
            ctx->error = BH_WEAPON_TARGET_LIMIT;
            return 0;
        }
        if (!query(ctx, target, &packet))
            return 0;
        x = (double)packet.position.x - entity->position.x;
        y = (double)packet.position.y - entity->position.y;
        z = (double)packet.position.z - entity->position.z;
        d.y = stored(y);
        d.z = stored(z);
        d.x = stored(x);
        aim = entity->player.aim_direction;
        projection = ((double)aim.x * d.x + (double)aim.y * d.y) + (double)aim.z * d.z;
        dot = stored(projection);
        if (projection >= 0 && dot < best)
        {
            length = sqrt(((double)d.z * d.z + (double)d.y * d.y) + (double)d.x * d.x);
            if (1 - (double)dot / length < .3)
            {
                if (!ctx->world)
                {
                    ctx->error = BH_WEAPON_TARGET_WORLD;
                    return 0;
                }
                self.kind = BH_WORLD_HIT_ACTOR;
                self.object.actor = (BHEntity *)entity;
                ++ctx->traces;
                result = bh_world_segment_run(ctx->world, &self, &weapon_state->cached_world, &entity->position,
                                              &packet.position, -1, &hit);
                if (result < 0)
                {
                    ctx->error = BH_WEAPON_TARGET_WORLD;
                    return 0;
                }
                if (!result)
                {
                    weapon_state->hud.target = target;
                    best = dot;
                }
            }
        }
        memcpy(&target, &target->actor.global_next, 4);
    }
    weapon_state->hud.charge = 0;
    return 1;
}
