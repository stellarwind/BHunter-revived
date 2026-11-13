#include "bh_weapon_arc_lifecycle.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef union ArcPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} ArcPacket;
static void begin(BHWeaponArcLifecycleContext *ctx)
{
    ctx->visited = ctx->queries = ctx->world_queries = ctx->requests = ctx->updates = ctx->stops = ctx->random_calls =
        0;
    ctx->error = BH_WEAPON_ARC_LIFECYCLE_OK;
}
static int error(BHWeaponArcLifecycleContext *ctx, BHWeaponArcLifecycleError e)
{
    if (ctx && ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK)
        ctx->error = e;
    return 0;
}
static int valid(BHWeaponArcLifecycleContext *ctx, BHEntity *weapon)
{
    if (!ctx)
        return 0;
    begin(ctx);
    return ctx->sound && weapon ? 1 : error(ctx, BH_WEAPON_ARC_LIFECYCLE_ARGUMENT);
}
static int finite_vector(const BHVec3 *p)
{
    return _finite(p->x) && _finite(p->y) && _finite(p->z);
}
static const BHVec3 *aim(const BHEntity *entity)
{
    return &entity->player.aim_direction;
}
static unsigned long request(BHWeaponArcLifecycleContext *ctx, unsigned long id, const BHEntity *entity, float level,
                             float pitch)
{
    unsigned long handle;
    ++ctx->requests;
    handle = bh_sound_request(ctx->sound, id, level, pitch, &entity->position, &entity->origin);
    if (handle == BH_SOUND_INVALID)
        error(ctx, BH_WEAPON_ARC_LIFECYCLE_SOUND);
    return handle;
}
static void stop(BHWeaponArcLifecycleContext *ctx, unsigned long handle)
{
    ++ctx->stops;
    if (bh_sound_stop(ctx->sound, handle) < 0)
        error(ctx, BH_WEAPON_ARC_LIFECYCLE_SOUND);
}
static int query(BHWeaponArcLifecycleContext *ctx, BHEntity *actor, ArcPacket *packet)
{
    if (!actor || !actor->query)
        return error(ctx, BH_WEAPON_ARC_LIFECYCLE_CALLBACK);
    ++ctx->queries;
    actor->query(actor, packet);
    return finite_vector(&packet->position.position) ? 1 : error(ctx, BH_WEAPON_ARC_LIFECYCLE_NUMERICAL);
}
int bh_weapon_arc_activate(BHWeaponArcLifecycleContext *ctx, BHEntity *weapon, const BHEntity *player)
{
    BHWeaponCommonState *weapon_state;
    float pitch;
    int value;
    if (!valid(ctx, weapon) || !player)
        return error(ctx, BH_WEAPON_ARC_LIFECYCLE_ARGUMENT);
    weapon_state = bh_weapon_common_state(weapon);
    weapon_state->word_e8 = 0;
    weapon_state->hud.target = NULL;
    weapon_state->hud.charge = 0;
    weapon_state->word_100 = -1;
    value = rand();
    ctx->random_calls = 1;
    bh_round_float(&pitch, .9 - (double)value * (-.000006103515625));
    if (request(ctx, ctx->activate_sound, player, .8f, pitch) == BH_SOUND_INVALID &&
        ctx->sound->error == BH_SOUND_PRIORITY)
        ctx->error = BH_WEAPON_ARC_LIFECYCLE_OK;
    weapon_state->word_108 = 0;
    return ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK;
}
int bh_weapon_arc_deactivate(BHWeaponArcLifecycleContext *ctx, BHEntity *weapon)
{
    BHWeaponCommonState *weapon_state;
    unsigned long handle;
    if (!valid(ctx, weapon))
        return 0;
    weapon_state = bh_weapon_common_state(weapon);
    handle = (unsigned long)weapon_state->word_100;
    weapon_state->word_e8 = 0;
    weapon_state->hud.target = NULL;
    weapon_state->hud.charge = 0;
    if (handle != BH_SOUND_INVALID)
        stop(ctx, handle);
    weapon_state->word_108 = 0;
    weapon_state->word_100 = -1;
    return ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK;
}
int bh_weapon_arc_track(BHWeaponArcLifecycleContext *ctx, BHEntity *weapon, const BHEntity *player)
{
    BHWeaponCommonState *weapon_state;
    ArcPacket packet;
    BHEntity *actor, *successor;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    BHVec3 d;
    double x, y, z, dot, length, cone;
    float nearest, projection, stored_cone;
    unsigned long handle;
    int result;
    const BHVec3 *direction;
    if (!valid(ctx, weapon) || !player)
        return error(ctx, BH_WEAPON_ARC_LIFECYCLE_ARGUMENT);
    weapon_state = bh_weapon_common_state(weapon);
    packet.position.event = 3;
    nearest = 150;
    if (!weapon_state->word_108)
    {
        weapon_state->hud.target = NULL;
        handle = (unsigned long)weapon_state->word_100;
        if (handle != BH_SOUND_INVALID)
            stop(ctx, handle);
        weapon_state->word_100 = -1;
        if (weapon_state->hud.charge > 32)
            request(ctx, ctx->release_sound, player, 1, 1);
        weapon_state->hud.charge = 0;
        return ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK;
    }
    weapon_state->word_108 = 0;
    if (weapon_state->hud.target)
    {
        weapon_state->hud.charge = (long)((unsigned long)weapon_state->hud.charge + 1UL);
        if (weapon_state->hud.charge == 16)
            weapon_state->word_100 = (long)request(ctx, ctx->charge16_sound, player, 1, 1);
        else if (weapon_state->hud.charge == 51)
            weapon_state->word_100 = (long)request(ctx, ctx->charge51_sound, player, 1, 1);
        else if ((unsigned long)weapon_state->word_100 != BH_SOUND_INVALID)
        {
            ++ctx->updates;
            if (bh_sound_update(ctx->sound, (unsigned long)weapon_state->word_100, 1, 1, &player->position,
                                &player->origin) < 0)
                error(ctx, BH_WEAPON_ARC_LIFECYCLE_SOUND);
        }
        if (!query(ctx, weapon_state->hud.target, &packet))
            return 0;
        if (!finite_vector(&player->position) || !finite_vector(aim(player)))
            return error(ctx, BH_WEAPON_ARC_LIFECYCLE_NUMERICAL);
        x = (double)packet.position.position.x - player->position.x;
        y = (double)packet.position.position.y - player->position.y;
        z = (double)packet.position.position.z - player->position.z;
        bh_round_float(&d.x, x);
        bh_round_float(&d.y, y);
        length = sqrt((x * d.x + y * d.y) + z * z);
        direction = aim(player);
        dot = ((z * direction->z + (double)direction->y * d.y) + (double)direction->x * d.x);
        cone = 1.0 - dot / length;
        bh_round_float(&stored_cone, cone);
        if (!(dot < 0) && !(dot > 150.0) && !(stored_cone > .1))
            return ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK;
        weapon_state->hud.target = NULL;
        handle = (unsigned long)weapon_state->word_100;
        if (handle != BH_SOUND_INVALID)
            stop(ctx, handle);
        weapon_state->word_100 = -1;
        if (weapon_state->hud.charge > 32)
            request(ctx, ctx->release_sound, player, 1, 1);
        return ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK;
    }
    actor = ctx->head_owner ? *ctx->head_owner : ctx->head;
    while (actor)
    {
        if (ctx->visited >= ctx->limit)
        {
            error(ctx, BH_WEAPON_ARC_LIFECYCLE_LIMIT);
            break;
        }
        ++ctx->visited;
        if (!query(ctx, actor, &packet))
            break;
        if (!finite_vector(&player->position) || !finite_vector(aim(player)))
        {
            error(ctx, BH_WEAPON_ARC_LIFECYCLE_NUMERICAL);
            break;
        }
        bh_round_float(&d.x, (double)packet.position.position.x - player->position.x);
        bh_round_float(&d.y, (double)packet.position.position.y - player->position.y);
        bh_round_float(&d.z, (double)packet.position.position.z - player->position.z);
        direction = aim(player);
        dot = ((double)direction->x * d.x + (double)direction->z * d.z) + (double)direction->y * d.y;
        bh_round_float(&projection, dot);
        if (!(dot < 0) && projection < nearest)
        {
            length = sqrt(((double)d.x * d.x + (double)d.y * d.y) + (double)d.z * d.z);
            if (!(1.0 - (double)projection / length >= .1))
            {
                if (!ctx->world)
                {
                    error(ctx, BH_WEAPON_ARC_LIFECYCLE_WORLD);
                    break;
                }
                self.kind = BH_WORLD_HIT_ACTOR;
                self.object.actor = (BHEntity *)player;
                ++ctx->world_queries;
                result = bh_world_segment_run(ctx->world, &self, &weapon_state->cached_world, &player->position,
                                              &packet.position.position, -1, &hit);
                if (result < 0)
                    error(ctx, BH_WEAPON_ARC_LIFECYCLE_WORLD);
                if (!result)
                {
                    weapon_state->hud.target = actor;
                    nearest = projection;
                }
            }
        }
        memcpy(&successor, &actor->actor.global_next, 4);
        actor = successor;
    }
    weapon_state->hud.charge = 0;
    return ctx->error == BH_WEAPON_ARC_LIFECYCLE_OK;
}
