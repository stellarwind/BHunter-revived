#include "bh_player_tick.h"
#include "bh_player_propulsion.h"
#include "bh_input_evaluate.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum
{
    DEATH_WRECK_FRAME = 75,
    DEATH_HIDE_FRAME = 95,
    DEATH_RESPAWN_FRAME = 200
};

static BHPlayerTick *bound_player;
static const BHVec3 zero = {0, 0, 0};
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long word(BHEntity *entity, unsigned long o)
{
    unsigned long n;
    memcpy(&n, (char *)entity + o, 4);
    return n;
}
static void setword(BHEntity *entity, unsigned long o, unsigned long n)
{
    memcpy((char *)entity + o, &n, 4);
}
static int fail(BHPlayerTick *ctx, int n)
{
    ctx->error = n;
    return 0;
}
static int key(BHPlayerTick *ctx, unsigned long index)
{
    unsigned char result;
    ctx->decoded->values[index] = ctx->player->player.input.words[index];
    result = bh_player_input_transition(ctx->decoded, index);
    ctx->player->player.input.words[index] = ctx->decoded->values[index];
    return result;
}
static int input(BHPlayerTick *ctx)
{
    BHPlayerInputState state;
    unsigned long i;
    if (!ctx->input || !ctx->decoded)
        return fail(ctx, 1);
    state = *ctx->decoded;
    memcpy(state.values, ctx->player->player.input.words, sizeof(ctx->player->player.input.words));
    if (!bh_player_input_apply_packet_checked(&state, ctx->input->state, sizeof(ctx->input->state)))
        return fail(ctx, 2);
    for (i = BH_PLAYER_INPUT_WORDS; i < 256; ++i)
        if (state.values[i] != ctx->decoded->values[i])
            return fail(ctx, 2);
    *ctx->decoded = state;
    memcpy(ctx->player->player.input.words, state.values, sizeof(ctx->player->player.input.words));
    return 1;
}
static unsigned long sound(BHPlayerTick *ctx, unsigned long resource, float level, float pitch, const BHVec3 *position,
                           const BHVec3 *velocity)
{
    return bh_sound_request(ctx->sound, resource, level, pitch, position, velocity);
}
static void restore_volume(BHPlayerTick *ctx)
{
    if (ctx->state->paused)
    {
        ctx->state->paused = 0;
        bh_cd_set_volume(ctx->media, ctx->state->saved_cd_volume);
        bh_wave_set_volume(ctx->media, ctx->state->saved_wave_volume);
    }
}
static void menu_cue(BHPlayerTick *ctx, float a, float b)
{
    sound(ctx, ctx->resources.menu_cue, 1, a, 0, 0);
    sound(ctx, ctx->resources.menu_cue, 1, b, 0, 0);
}
static int camera_reset(BHPlayerTick *ctx, unsigned long index)
{
    if (!ctx->camera)
        return fail(ctx, 1);
    ctx->camera->player = ctx->player;
    ctx->camera->player_count = ctx->state->mode;
    if (ctx->state->mode > 1 && ctx->network && ctx->network->packets)
        ctx->camera->local_player_index = *ctx->network->packets->local_slot;
    return bh_player_camera_reset(ctx->camera, index) ? 1 : fail(ctx, 3);
}
static int return_menu(BHPlayerTick *ctx)
{
    BHMessagePanelFields *fields;
    unsigned long event = 20;
    ctx->menu_return_stage = "validate bindings";
    if (!ctx->reset || !ctx->panel || !ctx->panel->entity || !ctx->snapshot || !ctx->world_reset || !ctx->menu)
        return fail(ctx, 1);
    ctx->menu_return_stage = "reset player camera";
    if (!camera_reset(ctx, 0))
        return fail(ctx, 3);
    ctx->menu_return_stage = "reset player";
    if (!bh_player_reset(ctx->reset))
        return fail(ctx, 3);
    /* Keep the engine request for the next mission and mute it in the menu. */
    bh_sound_update(ctx->sound, ctx->player->player.engine_sound, 0, 1, NULL, NULL);
    ctx->panel->messages->ring_start = 0;
    ctx->panel->messages->ring_count = 0;
    ctx->panel->messages->last_index = -1;
    ctx->panel->messages->display_ticks = ctx->panel->messages->display_phase = 0;
    ctx->panel->messages->arena_used = 0;
    fields = bh_message_panel_fields(ctx->panel->entity);
    fields->timer = fields->scale = 0;
    ctx->panel->mode_identity = 0x00442bf0UL;
    ctx->panel->entity->step = bh_message_panel_step;
    ctx->menu_return_stage = "restore message panel and input";
    if (!bh_entity_set_query(ctx->pool, ctx->panel->entity, bh_message_panel_query) ||
        !bh_input_set_enabled(ctx->input, ctx->snapshot, 0))
        return fail(ctx, 3);
    ctx->menu_return_stage = "broadcast entity reset (event 20)";
    if (!bh_entities_query(ctx->pool, 0, 0, 0, 0, &event))
        return fail(ctx, 3);
    ctx->menu_return_stage = "reset world and scripts";
    if (!bh_world_reset(ctx->world_reset))
        return fail(ctx, 3);
    ctx->menu_return_stage = "activate menu and settle flyby camera";
    if (!bh_menu_runtime_return(ctx->menu))
        return fail(ctx, 3);
    ctx->state->mode = 0;
    ctx->menu_return_stage = "complete";
    return 1;
}
static int packet_send(BHPlayerTick *ctx, unsigned char type, const void *payload, unsigned long n)
{
    if (!ctx->network || !ctx->network->transport ||
        !bh_packet_begin(&ctx->writer, ctx->packet, 64, type, (unsigned short)ctx->state->tick) ||
        !bh_packet_append(&ctx->writer, payload, n))
        return fail(ctx, 1);
    bh_network_packet_send(ctx->network->transport, &ctx->writer, 0, 0);
    ctx->packet_bytes = ctx->writer.cursor;
    ++ctx->packets;
    return 1;
}
static int sound_packet(BHPlayerTick *ctx, unsigned long resource, float level, float pitch, unsigned long sparks)
{
    BHWorldGeometry *world;
    unsigned long i;
    unsigned char tail[4];
    int result;
    if (ctx->state->mode < 2)
        return 1;
    if (!ctx->navigation || !ctx->network || !ctx->network->transport)
        return fail(ctx, 1);
    world = ctx->player->world;
    if (!world)
        return 1;
    for (i = 0; i < ctx->navigation->node_count; ++i)
        if (ctx->navigation->nodes[i].placement == world)
            break;
    if (i == ctx->navigation->node_count)
        return fail(ctx, 1);
    if (!bh_packet_begin(&ctx->writer, ctx->packet, 64, 6, (unsigned short)ctx->state->tick))
        return fail(ctx, 2);
    result = bh_packet_write_position(&ctx->writer, world, (unsigned short)i, &ctx->player->position);
    if (result < 0)
        return fail(ctx, 2);
    if (!result)
        return 1;
    tail[0] = (unsigned char)resource;
    tail[1] = (unsigned char)(__int64)((double)level * 128);
    tail[2] = (unsigned char)(__int64)((double)pitch * 64);
    tail[3] = (unsigned char)sparks;
    if (!bh_packet_append(&ctx->writer, tail, 4))
        return fail(ctx, 2);
    bh_network_packet_send(ctx->network->transport, &ctx->writer, 0, 0);
    ++ctx->packets;
    ctx->packet_bytes = ctx->writer.cursor;
    return 1;
}
static int network(BHPlayerTick *ctx)
{
    BHPlayerTickState *state = ctx->state;
    unsigned long i, n = 0;
    unsigned char payload[17];
    unsigned short tick;
    long converted;
    double sum = 0;
    if (!state->mode)
        return 1;
    ++state->tick;
    if (!state->ending && !input(ctx))
        return 0;
    if (state->mode < 2)
        return 1;
    if (!ctx->network || !ctx->game)
        return fail(ctx, 1);
    if (state->mode == 2)
    {
        *ctx->network->packets->local_slot = 0;
        if (!(state->tick & 63))
        {
            if (!ctx->latency)
                return fail(ctx, 1);
            for (i = 0; i < 8; ++i)
            {
                converted = (long)(__int64)((long)(short)(unsigned short)state->tick + (double)ctx->latency[i]);
                tick = (unsigned short)converted;
                memcpy(payload + i * 2, &tick, 2);
            }
            for (i = 1; i < 8; ++i)
                if (ctx->game->players[i].active)
                {
                    sum += ctx->latency[i];
                    ++n;
                }
            if (!n)
                return 1;
            sum = sum * 1.4 / n;
            state->peers = sum < 0 ? 0 : sum > 3 ? 3 : (unsigned long)sum;
            payload[16] = (unsigned char)state->peers;
            if (!packet_send(ctx, 5, payload, 17))
                return 0;
        }
    }
    return 1;
}
static int send_pose(BHPlayerTick *ctx, int reliable)
{
    BHPlayerTickState *state = ctx->state;
    BHEntity *entity = ctx->player;
    BHWorldGeometry *world;
    unsigned long i;
    unsigned short slot;
    unsigned char flags, angle, propulsion[BH_PROPULSION_CHANNELS] = {0};
    int result;
    float angles[3];
    if (state->mode < 2)
        return 1;
    if (!reliable && state->send_delay)
    {
        --state->send_delay;
        return 1;
    }
    state->send_delay = (long)state->peers;
    if (!ctx->network || !ctx->navigation)
        return fail(ctx, 1);
    if (*ctx->network->packets->local_slot < 0)
        return 1;
    slot = (unsigned short)*ctx->network->packets->local_slot;
    world = entity->world;
    if (!world)
        return 1;
    for (i = 0; i < ctx->navigation->node_count; ++i)
        if (ctx->navigation->nodes[i].placement == world)
            break;
    if (i == ctx->navigation->node_count)
        return fail(ctx, 1);
    if (!bh_packet_begin(&ctx->writer, ctx->packet, 64, 4, (unsigned short)state->tick) ||
        !bh_packet_append(&ctx->writer, &slot, 2))
        return fail(ctx, 2);
    result = bh_packet_write_position(&ctx->writer, world, (unsigned short)i, &entity->position);
    if (result < 0)
        return fail(ctx, 2);
    if (!result)
        return 1;
    memcpy(angles, &entity->euler, sizeof(angles));
    for (i = 0; i < 3; ++i)
    {
        angle = (unsigned char)(__int64)((double)angles[i] * 40.7436637878418);
        if (!bh_packet_append(&ctx->writer, &angle, 1))
            return fail(ctx, 2);
    }
    if (state->death_frames >= DEATH_HIDE_FRAME)
        flags = BH_REMOTE_POSE_HIDDEN;
    else
    {
        flags = entity->player.shield_time > 0 ? BH_REMOTE_POSE_SHIELD : 0;
        if (entity->player.ground_sound != BH_SOUND_INVALID)
            flags |= BH_REMOTE_POSE_GROUND_RING;
        if (entity->player.disabled_frames)
            flags |= BH_REMOTE_POSE_SMOKE;
        if (state->death_frames >= DEATH_WRECK_FRAME)
            flags |= BH_REMOTE_POSE_WRECK;
    }
    if (!bh_packet_append(&ctx->writer, &flags, 1))
        return fail(ctx, 2);
    if (!(flags & (BH_REMOTE_POSE_WRECK | BH_REMOTE_POSE_HIDDEN)))
        for (i = 0; i < sizeof(propulsion); ++i)
        {
            float channel = entity->player.propulsion.channels[1 + i] * BH_PROPULSION_WIRE_SCALE;
            if (channel > 0)
                propulsion[i] = channel >= 255 ? 255 : (unsigned char)(channel + .5f);
        }
    if (!bh_packet_append(&ctx->writer, propulsion, sizeof(propulsion)))
        return fail(ctx, 2);
    bh_network_packet_send(ctx->network->transport, &ctx->writer, 0, reliable);
    ++ctx->packets;
    ctx->packet_bytes = ctx->writer.cursor;
    return 1;
}
int bh_player_leave_network(BHPlayerTick *ctx)
{
    ctx->state->paused = ctx->state->ending = ctx->state->confirmation = 0;
    return return_menu(ctx);
}
static void cue(BHPlayerTick *ctx, unsigned long resource, float delay)
{
    BHEntity *entity;
    BHScriptLifetimeState life;
    if (!ctx->lifetime)
    {
        ctx->error = 1;
        return;
    }
    entity = bh_entity_acquire(ctx->pool);
    if (!entity)
        return;
    bh_script_lifetime_read(entity, &life);
    life.script_loaded = 0;
    life.mission_countdown = delay;
    life.mission_resource = resource;
    if (!bh_script_lifetime_bind(ctx->lifetime, entity, &life, bh_script_unloaded_query))
    {
        bh_entity_release(ctx->pool, entity);
        ctx->error = 1;
        return;
    }
    entity->step = bh_mission_sound_tick;
}
static void recoil(BHEntity *entity)
{
    BHVec3 position = entity->player.recoil_position, speed = entity->player.recoil_velocity, difference, clamped;
    float length;
    unsigned long i;
    for (i = 0; i < 3; ++i)
        ((float *)&position)[i] = rf((double)((float *)&position)[i] + ((float *)&speed)[i]);
    for (i = 0; i < 3; ++i)
        ((float *)&difference)[i] = rf((double)((float *)&entity->position)[i] - ((float *)&position)[i]);
    length = rf(sqrt(((double)difference.z * difference.z + (double)difference.y * difference.y) +
                     (double)difference.x * difference.x));
    clamped = difference;
    if (length > .5)
        for (i = 0; i < 3; ++i)
            ((float *)&clamped)[i] = rf(((float *)&difference)[i] * (.5 / length));
    for (i = 0; i < 3; ++i)
    {
        ((float *)&position)[i] = rf((double)((float *)&clamped)[i] + ((float *)&position)[i]);
        ((float *)&speed)[i] = rf((((double)((float *)&speed)[i] - ((float *)&difference)[i] * (-.6))) * .6);
    }
    entity->player.recoil_position = position;
    entity->player.recoil_velocity = speed;
}

static int near_contacts(BHPlayerTick *ctx)
{
    BHEntity *entity = ctx->player, *actor;
    BHWorldGeometry *world = entity->world;
    BHPickupRoom *room = 0;
    BHEntityPositionPacket position;
    BHPhysicsContactPacket contact;
    BHRemoteQueryPacket damage;
    BHVec3 d, point, mid;
    unsigned long i, kind, n = 0;
    float distance, root, raw, strength, level, coefficient, particle;
    int random;
    if (!world)
        return 1;
    if (!ctx->rooms || !ctx->contacts || !ctx->flash)
        return fail(ctx, 1);
    if (ctx->room_index)
    {
        long found = bh_binding_find(ctx->room_index, world, NULL);
        if (found >= 0 && (unsigned long)found < ctx->room_count && ctx->rooms[found].world == world)
            room = ctx->rooms + found;
    }
    if (!room)
        for (i = 0; i < ctx->room_count; ++i)
            if (ctx->rooms[i].world == world)
            {
                room = ctx->rooms + i;
                break;
            }
    if (!room)
        return fail(ctx, 1);
    actor = room->actors;
    while (actor)
    {
        if (++n > ctx->pool->capacity || !actor->query)
            return fail(ctx, 5);
        position.event = 3;
        actor->query(actor, &position);
        for (i = 0; i < 3; ++i)
            ((float *)&d)[i] = rf((double)((float *)&position.position)[i] - ((float *)&entity->position)[i]);
        distance = rf(((double)d.z * d.z + (double)d.y * d.y) + (double)d.x * d.x);
        if (distance < .1)
            distance = .1f;
        kind = actor->actor.kind;
        if (kind == 4)
            distance = rf((double)distance * .5f);
        else if (kind == 5)
            distance = rf((double)distance * .1666666716337204f);
        else if (kind == 9)
            distance = rf((double)distance * .25f);
        if (distance < 12.25)
        {
            ++ctx->near_contacts;
            if (kind == 9 && word(actor, 0x12c) == (unsigned long)actor && distance < 6.0025 && !word(actor, 0x134) &&
                !word(actor, 0x138))
            {
                setword(actor, 0x134, (unsigned long)entity);
                setword(actor, 0x138, word(actor, 0x128));
                sound(ctx, ctx->resources.target_cue, 2, .7f, &entity->position, &entity->origin);
                sound(ctx, ctx->resources.lock_cue, 2, 1, &entity->position, &entity->origin);
            }
            root = rf(sqrt(distance));
            coefficient = rf(1.0 / root);
            contact.event = 2;
            for (i = 0; i < 3; ++i)
                ((float *)&contact.normal)[i] = rf(((float *)&d)[i] * (double)coefficient);
            raw = rf(1.0 - sqrt(distance) * (1.0 / 3.5));
            level = rf((double)raw + raw);
            strength = rf((double)raw * raw);
            for (i = 0; i < 3; ++i)
            {
                ((float *)&point)[i] =
                    rf((double)((float *)&entity->position)[i] - ((float *)&contact.normal)[i] * (-1.75));
                random = rand();
                ((float *)&point)[i] = rf((double)((float *)&point)[i] - ((double)random - 16384) * (-1.0 / 65536));
            }
            contact.point = point;
            contact.impulse = rf((double)strength * 3);
            contact.severity = rf((double)strength + strength);
            if (kind == 4 || kind == 5)
            {
                contact.impulse = rf((double)contact.impulse * 4);
                contact.severity = rf((double)contact.severity * 8);
                level = rf((double)level * 4);
                if (contact.impulse > 1)
                    contact.impulse = 1;
                if (contact.normal.y < -.5)
                    contact.normal.y = -.5f;
            }
            if (!entity->query)
                return fail(ctx, 1);
            entity->query(entity, &contact);
            for (i = 0; i < 3; ++i)
                ((float *)&contact.normal)[i] = -((float *)&contact.normal)[i];
            contact.impulse = rf((double)strength * (-200));
            contact.severity = 0;
            actor->query(actor, &contact);
            damage.damage.event = 0;
            damage.damage.severity = rf((double)strength * 1.5);
            damage.damage.attacker = entity;
            if (entity->player.shield_time > 0)
            {
                damage.damage.severity = rf((double)damage.damage.severity * 3);
                for (i = 0; i < 3; ++i)
                    ((float *)&mid)[i] =
                        rf(((double)((float *)&entity->position)[i] + ((float *)&position.position)[i]) * .5);
                {
                    BHEntity *flash = bh_entity_acquire(ctx->pool);
                    if (flash)
                    {
                        flash->position = mid;
                        bh_flash_flare_bind(ctx->flash, flash, .1f, 200, 4);
                    }
                }
                sound(ctx, ctx->resources.electro, 1, 1.5f, &mid, &zero);
            }
            if (actor->actor.kind != 8)
                actor->query(actor, &damage);
            if (!sound_packet(ctx, ctx->resources.collision_side, level, rf(.8 - (double)strength * (-.4)),
                              (unsigned long)(__int64)((double)strength * 5)))
                return 0;
            sound(ctx, ctx->resources.collision_side, level, rf(.8 - (double)strength * (-.4)), &point, &zero);
            particle = strength;
            while (particle > .1)
            {
                bh_contact_spark_create(ctx->contacts, &point, particle, 1);
                particle = rf((double)particle - .2);
            }
        }
        actor = actor->actor.room_next;
    }
    return 1;
}
static void ring_refresh(BHEntity *entity, void *unused)
{
    BHPlayerTick *ctx;
    (void)unused;
    memcpy(&ctx, entity->payload + 528, 4);
    if (ctx && ctx->contacts && ctx->camera_yaw)
        bh_impact_ring_draw(ctx->contacts->queue, ctx->resources.water_ring, entity, ctx->camera_yaw);
}
static void ring(BHPlayerTick *ctx, const BHVec3 *position, const BHVec3 *normal, float size)
{
    BHEntity *entity;
    BHImpactState *state;
    if (!ctx->contacts || !ctx->camera_yaw)
    {
        ctx->error = 1;
        return;
    }
    entity = bh_contact_impact_create(ctx->contacts, position, normal, 4);
    if (!entity)
        return;
    state = bh_contact_impact_state(ctx->contacts, entity);
    state->size = size;
    state->duration = rf((double)size * .1);
    memcpy(entity->payload + 528, &ctx, 4);
    entity->refresh = ring_refresh;
}
static int ground(BHPlayerTick *ctx)
{
    BHEntity *entity = ctx->player;
    BHVec3 start, end, right, forward, up, side, a, b, d, position, velocity, acceleration;
    BHWorldGeometry *cached;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    unsigned long handle, i, image;
    int found = 0, result, n, j, random;
    float fraction, strength, cosine, sine, radius, width, fall, angle, size, rate, alpha;
    double raw_angle;
    if (!entity->player.vehicle || !ctx->state->mode)
        return 1;
    handle = entity->player.ground_sound;
    if (!entity->player.disabled_frames)
    {
        if (!ctx->world || !ctx->particles || !ctx->contacts)
            return fail(ctx, 1);
        up = entity->basis[1];
        right = entity->basis[0];
        forward = entity->basis[2];
        for (i = 0; i < 3; ++i)
        {
            ((float *)&start)[i] = rf((double)((float *)&entity->position)[i] - ((float *)&up)[i] * (-.3));
            ((float *)&end)[i] = rf((double)((float *)&entity->position)[i] - ((float *)&up)[i] * (-8.3));
        }
        cached = entity->world;
        self.kind = BH_WORLD_HIT_ACTOR;
        self.object.actor = entity;
        result = bh_world_segment_run(ctx->world, &self, &cached, &start, &end, -1, &hit);
        if (result < 0)
            return fail(ctx, 3);
        if (result)
        {
            fraction = hit.fraction;
            if (!_finite(fraction) || fraction < 0 || fraction > 1)
                return fail(ctx, 4);
            strength = rf(1.0 - fraction);
            random = rand();
            n = (int)(1 - (1 - (double)fraction) * random * (-1.0 / 8192));
            if (handle == BH_SOUND_INVALID)
            {
                handle = sound(ctx, ctx->resources.ground_loop, strength, 1, &hit.point, &entity->origin);
                entity->player.ground_sound = handle;
            }
            else
                bh_sound_update(ctx->sound, handle, strength, 1, &hit.point, &entity->origin);
            for (j = n; j;)
            {
                --j;
                found = 1;
                random = rand();
                raw_angle = (double)random * .00019175345108273588;
                cosine = rf(cos(raw_angle));
                sine = rf(sin(raw_angle));
                bh_vec3_cross(&right, &hit.normal, &a);
                bh_vec3_cross(&forward, &hit.normal, &b);
                for (i = 0; i < 3; ++i)
                    ((float *)&d)[i] = rf((double)cosine * ((float *)&a)[i] + (double)sine * ((float *)&b)[i]);
                bh_vec3_normalize(&d);
                if (!j && (hit.material_flags & 4))
                {
                    random = rand();
                    if (random < 8192)
                    {
                        random = rand();
                        size = rf(14.0 - (double)fraction * 10);
                        fall = rf((double)random * (1.0 / 8192));
                        for (i = 0; i < 3; ++i)
                            ((float *)&position)[i] = rf((double)fall * ((float *)&d)[i] + ((float *)&hit.point)[i]);
                        ring(ctx, &position, &hit.normal, size);
                    }
                }
                radius = rf((1 - (double)fraction) * .25);
                width = rf(2.5 - ((double)fraction + fraction));
                fall = rf((1 - (double)fraction) * (-.02));
                for (i = 0; i < 3; ++i)
                {
                    ((float *)&velocity)[i] = rf((double)((float *)&d)[i] * radius);
                    ((float *)&acceleration)[i] = rf((double)((float *)&velocity)[i] * (-1.0 / 15));
                    ((float *)&position)[i] = rf((double)((float *)&d)[i] * width + ((float *)&hit.point)[i]);
                }
                acceleration.y = rf((double)acceleration.y + fall);
                image = ctx->resources.smoke_image;
                size = .5f;
                rate = 20;
                alpha = radius;
                if (hit.material_flags & 4)
                {
                    image = ctx->resources.water_cloud;
                    size = .7f;
                    rate = 30;
                    alpha = strength;
                }
                random = rand();
                angle = rf((double)random * .00019175345108273588);
                bh_script_particle_create(ctx->particles, image, 3, &position, &velocity, &acceleration, size, angle,
                                          rate, 0, .8f, .9f, rf((1 - (double)fraction) * .5), alpha);
                ++ctx->ground_particles;
            }
        }
    }
    if (!found && handle != BH_SOUND_INVALID)
    {
        bh_sound_stop(ctx->sound, handle);
        entity->player.ground_sound = BH_SOUND_INVALID;
    }
    return !ctx->error;
}
static int weapons(BHPlayerTick *ctx)
{
    BHEntity *entity = ctx->player, *w;
    BHWeaponObjectState *state;
    unsigned long i, j, old, to, slot;
    int first, held, k;
    if (!ctx->weapons || !ctx->weapons->factory)
        return fail(ctx, 1);
    for (j = 0; j < 2; ++j)
        if (key(ctx, 27 + j) == 0)
        {
            old = entity->player.selected_weapon[j];
            if (old < j * 5 || old > j * 5 + 4)
                return fail(ctx, 1);
            to = old;
            for (i = 0; i < 5; ++i)
            {
                to = to + 1;
                if (to > j * 5 + 4)
                    to = j * 5;
                entity->player.selected_weapon[j] = (unsigned char)to;
                if (!bh_weapon_registry_resolve(&ctx->weapons->factory->registry, to, &w, &state))
                    return fail(ctx, 1);
                if (state->common.available_ec)
                    break;
            }
            if (i == 5)
                return fail(ctx, 5);
            if (old == to)
                sound(ctx, ctx->resources.no_weapon, 1, 1, &entity->position, &entity->origin);
            else if (!bh_weapon_dispatch(ctx->weapons, old, BH_WEAPON_CANCEL) ||
                     !bh_weapon_dispatch(ctx->weapons, to, BH_WEAPON_READY))
                return fail(ctx, 3);
        }
    for (i = 0; i < 10; ++i)
        if (key(ctx, 12 + i) == 0)
        {
            slot = i < 5 ? 0 : 1;
            old = entity->player.selected_weapon[slot];
            if (old != i)
            {
                if (!bh_weapon_registry_resolve(&ctx->weapons->factory->registry, i, &w, &state))
                    return fail(ctx, 1);
                if (!state->common.available_ec)
                    sound(ctx, ctx->resources.no_weapon, 1, 1, &entity->position, &entity->origin);
                else
                {
                    if (!bh_weapon_dispatch(ctx->weapons, old, BH_WEAPON_CANCEL))
                        return fail(ctx, 3);
                    entity->player.selected_weapon[slot] = (unsigned char)i;
                    if (!bh_weapon_dispatch(ctx->weapons, i, BH_WEAPON_READY))
                        return fail(ctx, 3);
                }
            }
        }
    for (j = 0; j < 2; ++j)
        if (!bh_weapon_dispatch(ctx->weapons, entity->player.selected_weapon[j], BH_WEAPON_TICK))
            return fail(ctx, 3);
    for (j = 0; j < 2; ++j)
    {
        first = held = 0;
        for (i = 0; i < 3; ++i)
        {
            slot = j ? (i == 0 ? 3 : i == 1 ? 5 : 4) : (i == 0 ? 22 : i == 1 ? 24 : 23);
            k = key(ctx, slot);
            if (!k)
                first = held = 1;
            else if (k == 2)
                held = 1;
        }
        if (first && !bh_weapon_dispatch(ctx->weapons, entity->player.selected_weapon[j], BH_WEAPON_TRIGGER))
            return fail(ctx, 3);
        if (held && !bh_weapon_dispatch(ctx->weapons, entity->player.selected_weapon[j], BH_WEAPON_FIRE))
            return fail(ctx, 3);
    }
    return 1;
}
static int camera_mode(BHPlayerTick *ctx, long mode)
{
    return bh_camera_state_set_mode(bh_player_camera_state(ctx->player), mode) ? 1 : fail(ctx, 3);
}
static void send_death_effect(BHPlayerTick *ctx, const BHVec3 *position, unsigned char stage)
{
    BHWorldGeometry *world;
    long index;
    if (ctx->state->mode < 2 || !ctx->network || !ctx->network->transport)
        return;
    world = ctx->player->world;
    index = bh_navigation_find(ctx->navigation, world);
    if (index < 0 ||
        !bh_packet_begin(&ctx->writer, ctx->packet, sizeof(ctx->packet), BH_NETWORK_DEATH_EFFECT,
                         (unsigned short)ctx->state->tick) ||
        bh_packet_write_position(&ctx->writer, world, (unsigned short)index, position) != 1 ||
        !bh_packet_append(&ctx->writer, &stage, 1))
        return;
    bh_network_packet_send(ctx->network->transport, &ctx->writer, 0, 1);
    ctx->packet_bytes = ctx->writer.cursor;
    ++ctx->packets;
}
static int death(BHPlayerTick *ctx)
{
    BHPlayerTickState *state = ctx->state;
    BHEntity *entity = ctx->player;
    BHVec3 position, acceleration, v;
    float distance, best = 9e37f;
    unsigned long i, j, index = 0;
    if (!state->death_frames)
        return 1;
    position = entity->position;
    for (i = 0; i < 3; ++i)
        ((float *)&position)[i] = rf((double)((float *)&position)[i] + (rand() - 16384) * (1.0 / 16384));
    ++state->death_frames;
    if (state->death_frames == DEATH_WRECK_FRAME || state->death_frames == DEATH_HIDE_FRAME)
    {
        if (!ctx->explosions)
            return fail(ctx, 1);
        acceleration = entity->acceleration;
        if (state->death_frames == DEATH_WRECK_FRAME)
        {
            sound(ctx, ctx->resources.death_cue, 1, 1.5f, &entity->position, &entity->origin);
            bh_script_explosion_create(ctx->explosions, &position, 1, 10, .85f, 0);
            send_death_effect(ctx, &position, BH_NETWORK_DEATH_FIRST);
            state->saved_model = entity->player.model_resource;
            entity->player.model_resource = ctx->resources.death_model;
            bh_sound_stop(ctx->sound, entity->player.death_sound);
            entity->player.death_sound = BH_SOUND_INVALID;
        }
        else
        {
            entity->player.model_resource = 0;
            bh_script_explosion_create(ctx->explosions, &entity->position, 1.2f, 14, .85f, 0);
            send_death_effect(ctx, &entity->position, BH_NETWORK_DEATH_FINAL);
        }
        entity->acceleration = acceleration;
        if (ctx->explosions->error)
            return fail(ctx, 3);
    }
    if (state->death_frames > DEATH_WRECK_FRAME)
    {
        entity->velocity.x = rf((double)entity->velocity.x * .9);
        entity->velocity.y = rf((double)entity->velocity.y * .9);
        entity->velocity.z = rf((double)entity->velocity.z * .9);
        entity->acceleration.x = rf((double)entity->acceleration.x * .9);
        entity->acceleration.y = rf((double)entity->acceleration.y * .9);
        entity->acceleration.z = rf((double)entity->acceleration.z * .9);
    }
    if (state->death_frames == DEATH_RESPAWN_FRAME)
    {
        if (state->mode == 1 && !state->ending)
        {
            state->confirmation = 0;
            return return_menu(ctx) ? 2 : 0;
        }
        if (!ctx->camera || ctx->camera->start_count < 8 || !ctx->reset)
            return fail(ctx, 1);
        entity->player.model_resource = state->saved_model;
        state->confirmation = state->death_frames = 0;
        for (i = 0; i < 8; ++i)
        {
            for (j = 0; j < 3; ++j)
                ((float *)&v)[j] =
                    rf((double)((float *)&entity->position)[j] - ((const float *)&ctx->camera->starts[i].position)[j]);
            distance = rf(sqrt(((double)v.z * v.z + (double)v.y * v.y) + (double)v.x * v.x));
            if (distance < best)
            {
                best = distance;
                index = i;
            }
        }
        if (!camera_reset(ctx, index) || !bh_player_reset(ctx->reset))
            return fail(ctx, 3);
        entity->player.shield_time = 5;
        entity->player.shield_fade = 1;
        entity->player.shield_pitch_time = 5;
        entity->player.camera_mode = 0;
        entity->player.camera_countdown = 0;
        if (!camera_mode(ctx, 0))
            return 0;
    }
    return 1;
}
static int camera_cheats(BHPlayerTick *ctx)
{
    BHEntity *entity = ctx->player, *w;
    BHWeaponObjectState *weapon;
    BHCameraInput in;
    static const long modes[6] = {0, 1, 7, 9, 8, 4};
    unsigned long i, bits;
    long ammo;
    if (entity->player.vehicle)
    {
        for (i = 0; i < 6; ++i)
            if (!key(ctx, 6 + i))
            {
                long selected_mode = modes[i];
#ifdef BH_SDL_BACKEND
                if (i == 0 && ctx->snapshot && (ctx->snapshot->gamepad.buttons & (1UL << BH_PAD_BACK)))
                {
                    unsigned long j;
                    for (j = 0; j < 6; ++j)
                        if (modes[j] == entity->player.camera_mode)
                            break;
                    selected_mode = j < 6 ? modes[(j + 1) % 6] : modes[0];
                }
#endif
                entity->player.camera_mode = (short)selected_mode;
                if (!camera_mode(ctx, selected_mode))
                    return 0;
                sound(ctx, ctx->resources.ui_cue, 1, 2, 0, 0);
            }
        if (!ctx->camera || !ctx->camera->camera)
            return fail(ctx, 1);
        in.target = entity->position;
        in.angles = entity->euler;
        in.basis_x = entity->basis[0];
        in.basis_y = entity->basis[1];
        in.basis_z = entity->basis[2];
        in.actor_token = (unsigned long)entity;
        if (!bh_camera_state_update(ctx->camera->camera, bh_player_camera_state(entity), &in, 1.0 / 30,
                                    entity->player.input.axes[BH_PLAYER_INPUT_AIM_X],
                                    entity->player.input.axes[BH_PLAYER_INPUT_AIM_Y]))
            return fail(ctx, 3);
    }
    if (!key(ctx, 31) && ctx->state->mode == 1)
        entity->player.model_resource = ctx->resources.cheat_model;
    if (!key(ctx, 29) && ctx->state->mode == 1)
        for (i = 0; i < 10; ++i)
        {
            if (!bh_weapon_registry_resolve(&ctx->weapons->factory->registry, i, &w, &weapon))
                return fail(ctx, 1);
            weapon->common.available_ec = 1;
            bits = (unsigned long)weapon->common.ammo + 999;
            memcpy(&ammo, &bits, 4);
            if (ammo > weapon->common.ammo_limit)
                ammo = weapon->common.ammo_limit;
            weapon->common.ammo = ammo;
        }
    if (!key(ctx, 30) && ctx->state->mode == 1)
        ctx->state->god_mode = !ctx->state->god_mode;
    if (ctx->state->god_mode)
    {
        entity->player.damage = 0;
        entity->player.wear = 0;
        entity->player.shield_time = 1;
    }
    return 1;
}
static int damage_timers(BHPlayerTick *ctx)
{
    BHEntity *entity = ctx->player, *w;
    BHWeaponObjectState *weapon;
    BHPlayerTickState *state = ctx->state;
    BHVec3 velocity, acceleration = {0, -.002f, 0}, forward = entity->basis[2], motion = entity->velocity;
    float speed, spin, angle, pitch, timer;
    unsigned long i, image, handle;
    short countdown;
    speed = rf(((double)forward.x * motion.x + (double)forward.z * motion.z) + (double)forward.y * motion.y);
    bh_sound_update(ctx->sound, entity->player.engine_sound, state->mode ? rf(.3 + (double)entity->player.engine_output * .05) : 0,
                    rf(.5 + fabs(speed) * .01), &entity->position, &motion);
    /* The player still ticks behind the flyby menu. It must not recreate
       damage/boost loops after return_menu has retired their handles. */
    if (!state->mode)
        return 1;
    if (entity->player.disabled_frames && rand() < 16384 && state->death_frames < DEATH_HIDE_FRAME)
    {
        if (!ctx->particles)
            return fail(ctx, 1);
        for (i = 0; i < 3; ++i)
            ((float *)&velocity)[i] = rf((rand() - 16384) * (1.0 / 262144));
        image = entity->player.wear == 1 ? ctx->resources.second_smoke : ctx->resources.smoke_image;
        spin = rf((rand() - 16384) * (1.0 / 16384));
        angle = rf(rand() * (1.0 / 512));
        bh_script_particle_create(ctx->particles, image, 3, &entity->position, &velocity, &acceleration, .5f, angle, 12,
                                  spin, .9f, .95f, 2, .3f);
    }
    if (!entity->player.disabled_frames)
    {
        if (entity->player.damage == 1.0f)
        {
            if (state->mode > 1)
            {
                pitch = rf(.75 - (double)rand() * (-1.0 / 65536));
                if (!sound_packet(ctx, ctx->resources.death_cue, 0, pitch, 0))
                    return 0;
            }
            if (entity->player.vehicle)
                entity->player.death_sound = sound(ctx, ctx->resources.death_loop, .8f, 1, 0, 0);
            entity->player.disabled_frames = 1;
            entity->player.wear = rf((double)entity->player.wear + .2);
            if (state->mode > 1)
            {
                if (!ctx->scores || !ctx->damage_source || !ctx->network || !ctx->network->packets)
                    return fail(ctx, 1);
                ctx->scores->mode = state->mode;
                ctx->scores->frame = (unsigned short)state->tick;
                ctx->scores->local_player = entity;
                if (!bh_network_scores_death(ctx->scores, *ctx->network->packets->local_slot, *ctx->damage_source))
                    return fail(ctx, 3);
            }
            if (entity->player.wear >= 1)
            {
                entity->player.wear = 1;
                if (!state->death_frames)
                    state->death_frames = 1;
                state->confirmation = 0;
                sound(ctx, ctx->resources.death_cue, 2, .8f, &entity->position, &motion);
            }
            entity->player.camera_countdown = (short)-3;
        }
    }
    else if (entity->player.damage < .5 && entity->player.wear < 1)
    {
        if (entity->player.vehicle && entity->player.death_sound != BH_SOUND_INVALID)
            bh_sound_stop(ctx->sound, entity->player.death_sound);
        entity->player.death_sound = BH_SOUND_INVALID;
        entity->player.disabled_frames = 0;
        if (state->mode > 1)
        {
            timer = entity->player.shield_time;
            if (timer == 0)
            {
                entity->player.shield_fade = 1;
                entity->player.shield_time = 5;
                entity->player.shield_pitch_time = 5;
            }
            else if (timer < 5)
            {
                entity->player.shield_time = 5;
                entity->player.shield_pitch_time = 5;
            }
        }
        entity->player.camera_countdown = 30;
        if (entity->player.wear >= .8 && !state->repair_warning)
        {
            state->repair_warning = 1;
            if (!ctx->panel || !ctx->panel->messages)
                return fail(ctx, 1);
            if (!bh_message_enqueue(
                    ctx->panel->messages,
                    "4Hey Buddy!\n\nYour car looks\npretty wrecked!\n\n\nDrop by one of our\nrepair shops and we'll\nmake you an offer.\n"))
                return fail(ctx, 3);
        }
    }
    entity->player.damage = rf((double)entity->player.damage * .995);
    countdown = entity->player.camera_countdown;
    if (countdown)
    {
        if (countdown < 0)
        {
            ++countdown;
            entity->player.camera_countdown = (short)countdown;
            if (!countdown && !camera_mode(ctx, state->death_frames ? 8 : 9))
                return 0;
        }
        else
        {
            --countdown;
            entity->player.camera_countdown = (short)countdown;
            if (!countdown && !camera_mode(ctx, entity->player.camera_mode))
                return 0;
        }
    }
    timer = entity->player.shield_time;
    if (timer > 0)
    {
        motion = entity->velocity;
        handle = entity->player.shield_sound;
        if (handle == BH_SOUND_INVALID)
        {
            sound(ctx, ctx->resources.boost_start, .6f, .6f, &entity->position, &motion);
            entity->player.shield_sound = sound(ctx, ctx->resources.boost_loop, .6f, .6f, &entity->position, &motion);
        }
        else
            bh_sound_update(ctx->sound, handle, .6f, .6f, &entity->position, &motion);
        if (timer <= entity->player.shield_pitch_time)
        {
            entity->player.shield_pitch_time = rf((double)timer * .9);
            pitch = rf(1.2 - (double)entity->player.shield_pitch_time * (1.0 / 30));
            sound(ctx, ctx->resources.menu_cue, .5f, pitch, 0, 0);
            sound(ctx, ctx->resources.menu_cue, .5f, rf((double)pitch * .5), 0, 0);
            sound(ctx, ctx->resources.ui_cue, .5f, pitch, 0, 0);
        }
        if (entity->player.shield_fade > 0)
            entity->player.shield_fade = rf((double)entity->player.shield_fade - 1.0 / 30);
        timer = rf((double)timer - 1.0 / 30);
        entity->player.shield_time = timer;
        entity->player.damage = 0;
        if (timer < 1 && timer >= .9666666666666667)
        {
            bh_sound_stop(ctx->sound, entity->player.shield_sound);
            entity->player.shield_sound = BH_SOUND_INVALID;
            sound(ctx, ctx->resources.boost_end, .6f, .6f, &entity->position, &motion);
        }
        if (timer <= 0)
        {
            entity->player.shield_time = 0; /* Source lifetime fix: the original threshold stop can be followed by a restart during the final second. Retire the owned voice at actual expiry. */
            handle = entity->player.shield_sound;
            if (handle != BH_SOUND_INVALID)
            {
                bh_sound_stop(ctx->sound, handle);
                entity->player.shield_sound = BH_SOUND_INVALID;
            }
        }
    }
    if (!(entity->player.frame_phase & 7))
    {
        if (!bh_weapon_registry_resolve(&ctx->weapons->factory->registry, 0, &w, &weapon))
            return fail(ctx, 1);
        if (weapon->common.ammo < 200)
        {
            ++weapon->common.ammo;
            if (weapon->common.ammo > weapon->common.ammo_limit)
                weapon->common.ammo = weapon->common.ammo_limit;
        }
    }
    if (entity->player.disabled_frames)
        entity->player.disabled_frames = (short)(entity->player.disabled_frames + 1);
    return 1;
}
int bh_player_tick(BHPlayerTick *ctx)
{
    BHEntity *entity;
    BHPlayerTickState *state;
    BHMessageState *messages;
    int k, result, send_death_pose;
    long last;
    if (!ctx || !ctx->player || !ctx->state)
        return 0;
    ctx->error = 0;
    ++ctx->calls;
    entity = ctx->player;
    state = ctx->state;
    if (state->paused)
    {
        if (!input(ctx))
            return 0;
        if (!key(ctx, 35) || !key(ctx, BH_PLAYER_INPUT_ESCAPE))
        {
            if (!state->mode)
                return 1;
            if (state->confirmation)
            {
                restore_volume(ctx);
                menu_cue(ctx, .7f, .5f);
                state->confirmation = 0;
            }
        }
        if (!state->mode)
            return 1;
        k = key(ctx, 34);
        if ((k != 0 || !state->confirmation) && state->ending != 2)
            return 1;
        restore_volume(ctx);
        if (!state->ending)
            menu_cue(ctx, .7f, .9f);
        state->ending = state->confirmation = 0;
        return return_menu(ctx);
    }
    if (state->world_mode == 1)
        return 1;
    if (!ctx->pool || !ctx->body || !ctx->physics || !ctx->force || !ctx->decoded || !ctx->sound)
        return fail(ctx, 1);
    if (state->force != bh_player_force_control_a && state->force != bh_player_force_control_b &&
        state->force != bh_player_force_control_c && state->force != bh_player_force_control_d)
        return fail(ctx, 1);
    entity->player.frame_phase = (unsigned char)(entity->player.frame_phase + 1);
    if (!network(ctx))
        return 0;
    if (ctx->body->entity != entity || ctx->force->body != ctx->body ||
        !bh_physics_step_read_entity(ctx->body->physics, entity))
        return fail(ctx, 1);
    ctx->body->world = entity->world;
    /* The original broad phase reads the live registry. Event 20 can retire
       a wreck through the debris callback before our cached roster is rebuilt. */
    if (ctx->targets && !bh_target_construct_prune_retired(ctx->targets))
        return fail(ctx, 3);
    if (!bh_physics_step(ctx->physics, entity, 1.0f / 30) ||
        !state->force(ctx->force, 1.0f / 30, entity->player.disabled_frames) || !bh_physics_step_publish(ctx->body))
        return fail(ctx, ctx->error ? ctx->error : 3);
    recoil(entity);
    /* Send a single pose for a transition tick, after the model or spawn changes. */
    send_death_pose = state->mode > 1 && (state->death_frames == DEATH_WRECK_FRAME - 1 ||
                                         state->death_frames == DEATH_HIDE_FRAME - 1 ||
                                         state->death_frames == DEATH_RESPAWN_FRAME - 1);
    if (!send_death_pose && !send_pose(ctx, 0))
        return fail(ctx, ctx->error ? ctx->error : 3);
    if (!state->damage_warning)
    {
        if (entity->player.damage >= .6 && !entity->player.disabled_frames)
        {
            state->damage_warning = 1;
            cue(ctx, ctx->resources.damage_warning, .5f);
        }
    }
    else if (entity->player.damage <= .05)
        state->damage_warning = 0;
    if (!state->hull_warning)
    {
        if (entity->player.wear >= .8)
        {
            state->hull_warning = 1;
            cue(ctx, ctx->resources.hull_warning, 1);
        }
    }
    else if (entity->player.wear <= .2)
        state->hull_warning = 0;
    if (ctx->error || !bh_player_aim_update(ctx->body) || !near_contacts(ctx) || !ground(ctx))
        return fail(ctx, ctx->error ? ctx->error : 3);
    if (!key(ctx, 32))
    {
        if (!ctx->panel || !ctx->panel->messages)
            return fail(ctx, 1);
        messages = ctx->panel->messages;
        last = messages->last_index;
        if (last == -1)
            last = (messages->ring_count - 1 + messages->ring_start) % 8;
        else if (last == messages->ring_start)
            last = -1;
        else
        {
            --last;
            if (last < 0)
                last += 8;
        }
        messages->last_index = last;
        messages->display_phase = 1;
        messages->display_ticks = 40;
        sound(ctx, ctx->resources.ui_cue, 1, 2, 0, 0);
    }
    if (!key(ctx, BH_PLAYER_INPUT_ESCAPE) && state->mode && entity->player.wear < 1)
    {
        menu_cue(ctx, .8f, .6f);
        state->confirmation = 1;
        if (state->mode == 1 && !state->paused)
        {
            if (!ctx->media)
                return fail(ctx, 1);
            state->paused = 1;
            state->saved_cd_volume = (float)bh_cd_audio_volume(ctx->media->audio);
            bh_cd_set_volume(ctx->media, 0);
            state->saved_wave_volume = (float)bh_wave_audio_volume(ctx->media->audio);
            bh_wave_set_volume(ctx->media, 0);
        }
    }
    k = key(ctx, 35);
    if (!k && state->mode && state->confirmation)
    {
        restore_volume(ctx);
        menu_cue(ctx, .7f, .5f);
        state->confirmation = 0;
    }
    if ((k != 0 || state->mode) && state->mode)
    {
        k = key(ctx, 34);
        if ((!k && state->confirmation) || state->ending == 2)
        {
            restore_volume(ctx);
            if (!state->ending)
                menu_cue(ctx, .7f, .9f);
            state->ending = state->confirmation = 0;
            return return_menu(ctx);
        }
    }
    result = death(ctx);
    if (!result)
        return 0;
    if (result == 2)
        return 1;
    if (send_death_pose && !send_pose(ctx, 1))
        return 0;
    if (!weapons(ctx) || !camera_cheats(ctx) || !damage_timers(ctx))
        return 0;
    if (!bh_physics_step_read_entity(ctx->body->physics, entity))
        return fail(ctx, 3);
    ctx->body->world = entity->world;
    return !ctx->error;
}
int bh_player_tick_bind(BHPlayerTick *ctx, BHEntity *entity)
{
    BHEntity *p;
    unsigned long n = 0;
    if (!ctx || !entity || !ctx->pool || bound_player)
        return 0;
    for (p = ctx->pool->active; p && n++ < ctx->pool->capacity; p = p->next)
        if (p == entity)
            break;
    if (p != entity)
        return 0;
    ctx->player = entity;
    ctx->generation = entity->generation;
    bound_player = ctx;
    entity->step = bh_player_step;
    return 1;
}
BHPlayerTick *bh_player_tick_context(BHEntity *entity)
{
    BHPlayerTick *ctx = bound_player;
    if (!ctx || ctx->player != entity || !ctx->pool)
        return 0;
    return bh_entity_is_active(ctx->pool, entity) && entity->generation == ctx->generation ? ctx : 0;
}
int bh_player_tick_unbind(BHPlayerTick *ctx)
{
    if (!ctx || bound_player != ctx)
        return 0;
    if (bh_player_tick_context(ctx->player) && ctx->player->step == bh_player_step)
        ctx->player->step = 0;
    bound_player = 0;
    return 1;
}
void bh_player_step(BHEntity *entity)
{
    BHPlayerTick *ctx = bh_player_tick_context(entity);
    if (ctx)
        bh_player_tick(ctx);
}
