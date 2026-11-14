#include "../Entities/bh_field_access.h"
/* Remaining death/healthy return of00439910. Source lifetime/legacy-angle and
   checked registry adapters are explicit; they are not new original owners. */
#define RESPONSE_FINISH_CURVE(t, p)                                                                                    \
    bh_camera_curve((t), (p), &actor->actor.route_start, &actor->actor.route_first, &actor->actor.route_second, &actor->actor.route_end)
void bh_actor_response_finish(BHEntity *actor, BHActorResponseContext *context, const BHVec3 *death_position, int smoked)
{
    BHActorDamageContext *actor_damage_context = context->damage;
    BHVec3 before, at, after, velocity, spin;
    BHEntity *mission, *owner;
    unsigned long type, token, material, special;
    float t, strength;
    int draw;
    long first = -1, second = -1, row, loot = 0;
    short slot;
    struct
    {
        long event;
        BHEntity *actor;
        long tag;
    } packet;
    if (!smoked || bh_field_f32(&actor->actor.damage) < 1)
    {
        if (bh_field_f32(&actor->actor.damage) < .05 && bh_field_u32(&actor->actor.speed_scale) == 0x3f800000UL &&
            bh_field_u32(&actor->actor.fade) == 0x3f800000UL)
            actor->step = bh_actor_vehicle_tick;
        return;
    }
    if (!actor_damage_context->legacy_death_angles)
    {
        context->error = 1;
        return;
    }
    bh_entity_set_query(actor_damage_context->pool, actor, 0);
    bh_sound_stop(actor_damage_context->route->sound, bh_field_u32(&actor->actor.sound));
    t = response_query_stored((double)bh_field_f32(&actor->actor.phase) / bh_field_f32(&actor->actor.duration));
    RESPONSE_FINISH_CURVE(response_query_stored((double)t - 1.0 / 30.0), &before);
    RESPONSE_FINISH_CURVE(t, &at);
    RESPONSE_FINISH_CURVE(response_query_stored((double)t - (-1.0 / 30.0)), &after);
    /* 00439e7a..00439edc has the same retained-X/spilled-YZ contract as
       00435bf9..00435c52. Its original-byte oracle is recorded separately. */
    bh_actor_damage_death_velocity(&after, &at, &velocity);
    bh_script_debris_create(actor_damage_context->fragments, (rand() & 7) + 1, death_position, 8, 10, 1);
    strength = response_query_stored(.75 - (double)rand() * (-1.0 / 65536.0));
    bh_sound_request(actor_damage_context->route->sound, actor_damage_context->death_sound, 2, strength, death_position, &velocity);
    spin.x = response_query_stored(((double)rand() - 16384) * (1.0 / 4096.0));
    spin.y = response_query_stored(((double)rand() - 16384) * (1.0 / 4096.0));
    spin.z = response_query_stored(((double)rand() - 16384) * (1.0 / 4096.0));
    slot = bh_field_i16(&actor->actor.primary_weapon);
    if (!slot)
        first = 10;
    else if (slot == 1)
        first = 12;
    else if (slot == 2)
        first = 11;
    slot = bh_field_i16(&actor->actor.secondary_weapon);
    if (!slot)
        second = 5;
    else if (slot == 1)
        second = 3;
    else if (slot == 2)
        second = 6;
    type = bh_field_u32(&actor->actor.kind);
    row = type == 6 ? 3 : type == 1 ? 1 : 2;
    if (!bh_remote_choose_slot(actor_damage_context->loot_rows, actor_damage_context->loot_row_count, row, first, second, &loot))
    {
        context->error = 1;
        return;
    }
    /* The original reloads kind after loot, and again after cue allocation. */
    if (bh_field_u32(&actor->actor.kind) == 6 && (mission = bh_entity_acquire(actor_damage_context->pool)) != 0)
    {
        if (!bh_script_lifetime_bind_context(context->mission, mission))
        {
            bh_entity_release(actor_damage_context->pool, mission);
            context->error = 1;
            return;
        }
        *(volatile float *)(&mission->script_lifetime.mission_countdown) = 1.5f;
        *(volatile unsigned long *)(&mission->script_lifetime.mission_resource) = context->mission_resource;
        *(BHEntityStep volatile *)&mission->step = bh_mission_sound_tick;
    }
    special = bh_field_u32(&actor->actor.kind) == 6;
    token = bh_field_u32(&actor->actor.bounty_token);
    owner = (BHEntity *)bh_field_u32(&actor->actor.damage_source);
    draw = rand();
    material = bh_field_u32(&actor->actor.model_resource);
    strength = response_query_stored(1.0 - (double)draw * (-1.0 / 16384.0));
    bh_target_construct_moving(actor_damage_context->targets, death_position, actor_damage_context->legacy_death_angles, &velocity, &spin, material, 10000,
                               strength, owner, (long)token, loot, special);
    packet.event = 17;
    packet.actor = actor;
    bh_entities_query(actor_damage_context->pool, 0, 0, 0, 0, &packet);
    if (bh_field_u32(&actor->actor.death_tag))
    {
        packet.event = 14;
        packet.tag = (long)bh_field_u32(&actor->actor.death_tag);
        bh_entities_query(actor_damage_context->pool, 0, 0, 0, 0, &packet);
    }
    if (unlink_actor(context, actor))
        bh_entity_release(actor_damage_context->pool, actor);
}
#undef RESPONSE_FINISH_CURVE
