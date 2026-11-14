#include "bh_vm_queries.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <string.h>
static int live(BHEntityPool *pool, const BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
static BHBlockDamageContext *damage_context(BHEntity *block)
{
    BHBlockDamageContext *context;
    memcpy(&context, &block->bindings.block_damage, 4);
    return context;
}
int bh_block_damage_read(const BHEntity *block, BHBlockDamageState *state)
{
    if (!block || !state)
        return 0;
    memcpy(&state->health, &block->placement.damage, 4);
    memcpy(&state->owner, &block->placement.damage_source, 4);
    return 1;
}
int bh_block_damage_bind(BHEntityPool *pool, BHEntity *block, BHBlockDamageContext *context,
                         const BHBlockDamageState *state, int player_only)
{
    if (!live(pool, block) || !context || !context->world || !state)
        return 0;
    memcpy(&block->placement.damage, &state->health, 4);
    memcpy(&block->placement.damage_source, &state->owner, 4);
    memcpy(&block->bindings.block_damage, &context, 4);
    return bh_entity_set_query(pool, block, player_only ? bh_block_player_damage_query : bh_block_damage_query);
}
static void damage(BHEntity *block, float value)
{
    float health;
    memcpy(&health, &block->placement.damage, 4);
    bh_round_float(&health, (double)health - (double)value * (-10.0f));
    memcpy(&block->placement.damage, &health, 4);
}
void bh_block_player_damage_query(BHEntity *block, void *packet)
{
    long event;
    BHEntity *owner, *watched;
    float amount;
    BHBlockDamageContext *context = damage_context(block);
    memcpy(&event, packet, 4);
    if (event == 0)
    {
        memcpy(&owner, (char *)packet + 8, 4);
        if (owner == context->player)
        {
            memcpy(&amount, (char *)packet + 4, 4);
            damage(block, amount);
            memcpy(&block->placement.damage_source, &owner, 4);
        }
    }
    else if (event == 17)
    {
        memcpy(&watched, &block->placement.damage_source, 4);
        memcpy(&owner, (char *)packet + 4, 4);
        if (owner == watched)
        {
            owner = NULL;
            memcpy(&block->placement.damage_source, &owner, 4);
        }
    }
}
void bh_block_damage_query(BHEntity *block, void *packet)
{
    long event;
    BHEntity *owner, *watched;
    float amount, health, distance, x, y, z;
    double dx, dy, dz, length;
    BHDebrisBlastPacket blast;
    BHBlockDamageContext *context = damage_context(block);
    memcpy(&event, packet, 4);
    switch (event)
    {
    case 0:
        memcpy(&amount, (char *)packet + 4, 4);
        damage(block, amount);
        memcpy(&owner, (char *)packet + 8, 4);
        memcpy(&block->placement.damage_source, &owner, 4);
        return;
    case 2:
        memcpy(&amount, (char *)packet + 0x20, 4);
        damage(block, amount);
        return;
    case 17:
        memcpy(&watched, &block->placement.damage_source, 4);
        memcpy(&owner, (char *)packet + 4, 4);
        if (owner == watched)
        {
            owner = NULL;
            memcpy(&block->placement.damage_source, &owner, 4);
        }
        return;
    case 1:
        memcpy(&blast, packet, sizeof(blast));
        /* Original X midpoint is spilled; Y midpoint stays extended until its
       delta is formed. Z midpoint spills before subtraction. Each delta is
       stored before a retained extended copy multiplies that binary32 spill. */
        bh_round_float(&x, ((double)context->world->minimum.x + context->world->maximum.x) * .5);
        bh_round_float(&z, ((double)context->world->minimum.z + context->world->maximum.z) * .5);
        dx = (double)x - blast.center.x;
        dy = ((double)context->world->minimum.y + context->world->maximum.y) * .5 - blast.center.y;
        dz = (double)z - blast.center.z;
        bh_round_float(&x, dx);
        bh_round_float(&y, dy);
        bh_round_float(&z, dz);
        length = sqrt((dx * x + dy * y) + dz * z);
        bh_round_float(&distance, length);
        if (distance > 0 && distance < blast.radius)
        {
            memcpy(&health, &block->placement.damage, 4);
            bh_round_float(&health, (double)health -
                                        ((double)blast.strength * (1.0 - (double)distance / blast.radius)) * (-.0008));
            memcpy(&block->placement.damage, &health, 4);
        }
        return;
    default:
        return;
    }
}
static BHRadarQueryContext *radar_context(BHEntity *marker)
{
    BHRadarQueryContext *context;
    memcpy(&context, &marker->bindings.radar_query, 4);
    return context;
}
int bh_radar_query_bind(BHRadarQueryContext *context, BHEntity *marker, BHEntity *owner, long id)
{
    BHEntity *zero = NULL;
    if (!context || !context->lifetime || context->lifetime->timers.pool != context->pool ||
        !context->lifetime->sounds || !live(context->pool, marker) || context->count >= 65535)
        return 0;
    memcpy(&marker->actor.global_previous, &zero, 4);
    memcpy(&marker->actor.global_next, &context->head, 4);
    if (context->head)
        memcpy(&context->head->actor.global_previous, &marker, 4);
    memcpy(&marker->actor.radar_id, &id, 4);
    memcpy(&marker->actor.radar_owner, &owner, 4);
    memcpy(&marker->bindings.radar_query, &context, 4);
    if (!bh_entity_set_query(context->pool, marker, bh_radar_entity_query))
        return 0;
    context->head = marker;
    ++context->count;
    return 1;
}
static int retire(BHRadarQueryContext *context, BHEntity *marker)
{
    BHEntity *previous = marker->actor.global_previous, *next = marker->actor.global_next;
    if (!context->count || !live(context->pool, marker) ||
        (previous && (!live(context->pool, previous) || previous->actor.global_next != marker)) ||
        (next && (!live(context->pool, next) || next->actor.global_previous != marker)) ||
        (!previous && context->head != marker))
    {
        context->error = 1;
        return 0;
    }
    if (previous)
        memcpy(&previous->actor.global_next, &next, 4);
    else
        context->head = next;
    if (next)
        memcpy(&next->actor.global_previous, &previous, 4);
    --context->count;
    if (!bh_entity_release(context->pool, marker))
    {
        context->error = 1;
        return 0;
    }
    return 1;
}
void bh_radar_entity_query(BHEntity *marker, void *packet)
{
    BHRadarQueryContext *context = radar_context(marker);
    long event, id, matched;
    BHEntity *owner, *incoming, *sound;
    BHScriptLifetimeState state;
    BHVec3 zero = {0, 0, 0};
    memcpy(&event, packet, 4);
    switch (event)
    {
    case 3:
        memcpy((char *)packet + 4, &marker->position, 12);
        return;
    case 16:
        memcpy((char *)packet + 4, &marker->position, 12);
        memcpy((char *)packet + 16, &zero, 12);
        memcpy((char *)packet + 28, &zero, 12);
        return;
    case 14:
        memcpy(&id, &marker->actor.radar_id, 4);
        memcpy(&matched, (char *)packet + 8, 4);
        if (id && matched == id)
            retire(context, marker);
        return;
    case 20:
        retire(context, marker);
        return;
    case 17:
        memcpy(&owner, &marker->actor.radar_owner, 4);
        memcpy(&incoming, (char *)packet + 4, 4);
        if (owner != incoming)
            return;
        if (!retire(context, marker))
            return;
        sound = bh_entity_acquire(context->pool);
        if (!sound)
            return;
        memset(&state, 0, sizeof(state));
        state.mission_resource = context->retired_sound;
        state.mission_countdown = 1.5f;
        if (!bh_script_lifetime_bind(context->lifetime, sound, &state, NULL))
        {
            bh_entity_release(context->pool, sound);
            context->error = 1;
            return;
        }
        sound->step = bh_mission_sound_tick;
        return;
    default:
        return;
    }
}
