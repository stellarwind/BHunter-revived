#include "../Entities/bh_field_access.h"
#include "bh_remote_packets.h"
#ifdef BH_SDL_BACKEND
#include "../../Graphics/bh_render_interpolation.h"
#endif
#include <string.h>
static void setword(BHEntity *entity, unsigned long o, unsigned long n)
{
    memcpy((char *)entity + o, &n, 4);
}
static short signed16(unsigned long n)
{
    unsigned short u = (unsigned short)n;
    short s;
    memcpy(&s, &u, 2);
    return s;
}
static int open(BHRemotePackets *packets, void *p, unsigned long n, unsigned long capacity)
{
    if (!packets || !p)
    {
        if (packets)
            packets->error = 1;
        return 0;
    }
    packets->error = 0;
    ++packets->pending;
    if (!bh_packet_receive(&packets->reader, p, n, capacity, &packets->framing))
    {
        --packets->pending;
        ++packets->bad;
        packets->error = 2;
        return 0;
    }
    return 1;
}
static int take(BHRemotePackets *packets, void *out, unsigned long n)
{
    BHPacketReader *reader = &packets->reader;
    if (reader->cursor > reader->size || n > reader->size - reader->cursor)
    {
        packets->error = 3;
        reader->error = BH_PACKET_TRUNCATED;
        return 0;
    }
    memcpy(out, reader->data + reader->cursor, n);
    reader->cursor += n;
    return 1;
}
static int done(BHRemotePackets *packets)
{
    --packets->pending;
    ++packets->processed;
    return !packets->error;
}
static void query(BHEntity *entity, void *p)
{
    BHRemotePackets *packets;
    memcpy(&packets, &entity->bindings.remote_packets, 4);
    if (packets && packets->query)
    {
        packets->query->sound = packets->tick->sound;
        packets->query->tick = (unsigned short)*packets->simulation_tick;
        bh_remote_query(packets->query, entity, (BHRemoteQueryPacket *)p);
    }
}
int bh_remote_entity_draw(BHRemotePackets *packets, BHEntity *entity)
{
    BHRemotePackets *bound;
    int result;
#ifdef BH_SDL_BACKEND
    unsigned long mark;
#endif
    if (!packets || !entity || !packets->render || !packets->simulation_tick)
        return 0;
    memcpy(&bound, &entity->bindings.remote_packets, 4);
    if (bound != packets || !packets->remote || bh_field_i16(&entity->remote.slot) < 0 ||
        bh_field_i16(&entity->remote.slot) > 7 || packets->remote[bh_field_i16(&entity->remote.slot)] != entity)
        return 0;
    packets->render->tick = (unsigned short)*packets->simulation_tick;
#ifdef BH_SDL_BACKEND
    mark = bh_interpolation_mark();
#endif
    result = bh_remote_render(packets->render, entity);
#ifdef BH_SDL_BACKEND
    bh_interpolation_stamp(mark, (unsigned long)entity, entity->generation);
#endif
    return result;
}

void bh_remote_draw_scene(BHRemotePackets *packets)
{
    for (unsigned int slot = 0; slot < BH_NETWORK_PLAYERS; ++slot)
        if (packets->remote[slot])
            bh_remote_entity_draw(packets, packets->remote[slot]);
}

/* Room membership still drives queries. Drawing waits for the viewport reset. */
static void skip_room_draw(BHEntity *entity)
{
    (void)entity;
}

void bh_remote_bind_draw(BHRemotePackets *packets, BHEntity *entity, unsigned int slot)
{
    unsigned int car = packets->network->players[slot].car;
    if (packets->car_models && car < BH_NETWORK_CARS)
        packets->render->models[slot] = packets->car_models[car];
    memset(entity->remote.propulsion, 0, sizeof(entity->remote.propulsion));
    entity->actor.draw = skip_room_draw;
}
int bh_remote_spawn_packet(BHRemotePackets *packets, void *p, unsigned long n, unsigned long capacity)
{
    unsigned long id, i, j;
    unsigned char slot, name[20];
    BHEntity *entity, *next;
    BHPickupRoom *room = 0;
    BHWorldGeometry *world;
    unsigned short stamp;
    if (!open(packets, p, n, capacity))
        return 0;
    if (!packets->pool || !packets->remote || !packets->head || !packets->count || !packets->network ||
        !packets->simulation_tick || !packets->local_slot || !packets->query || !packets->render || !packets->tick ||
        !packets->tick->fire || packets->tick->fire->tick != packets->simulation_tick || !packets->spawn_world)
    {
        packets->error = 1;
        return 0;
    }
    if (!take(packets, &id, 4) || !take(packets, &slot, 1))
        return 0;
    if (slot >= 8)
    {
        packets->error = 4;
        --packets->pending;
        ++packets->bad;
        return 0;
    }
    if (id == packets->network->local_player)
    {
        memcpy(&stamp, packets->reader.data + 2, 2);
        *packets->simulation_tick = (*packets->simulation_tick & 0xffff0000UL) | stamp;
        *packets->local_slot = slot;
        return done(packets);
    }
    if (!take(packets, name, 20))
        return 0;
    if (packets->remote[slot])
    {
        packets->error = 4;
        return 0;
    }
    world = *packets->spawn_world;
    if (world)
    {
        if (!packets->rooms)
        {
            packets->error = 1;
            return 0;
        }
        for (i = 0; i < packets->room_count; ++i)
            if (packets->rooms[i].world == world)
            {
                room = packets->rooms + i;
                break;
            }
        if (!room)
        {
            packets->error = 1;
            return 0;
        }
    }
    entity = bh_entity_acquire(packets->pool);
    if (!entity)
    {
        packets->error = 5;
        return 0;
    }
    packets->remote[slot] = entity;
    bh_field_store_u16(&entity->remote.slot, slot);
    bh_field_store_u32(&entity->remote.player_id, id);
    bh_field_store_u16(&entity->remote.roster_slot, 0xffff);
    for (i = 1; i < 8; ++i)
        if (packets->network->players[i].active && packets->network->players[i].id == id)
            bh_field_store_u16(&entity->remote.roster_slot, (unsigned short)i);
    bh_remote_bind_draw(packets, entity, slot);
    stamp = (unsigned short)*packets->simulation_tick;
    entity->remote.newest.tick = stamp;
    entity->remote.middle.tick = (unsigned short)(stamp - 1);
    entity->remote.oldest.tick = (unsigned short)(stamp - 2);
    entity->remote.oldest.position = entity->remote.oldest.angles = (BHVec3){0};
    entity->remote.middle.position = entity->remote.middle.angles = (BHVec3){0};
    entity->remote.newest.position = entity->remote.newest.angles = (BHVec3){0};
    for (j = 0; j < 19 && name[j]; ++j)
        entity->remote.name[j] = (char)name[j];
    entity->remote.name[j] = 0;
    entity->flags |= 0x100;
    entity->remote.pose_flags = 0;
    bh_field_store_u16(&entity->remote.score, 0);
    entity->velocity = (BHVec3){0};
    entity->angular_velocity = (BHVec3){0};
    entity->acceleration = (BHVec3){0};
    entity->angular_acceleration = (BHVec3){0};
    entity->actor.kind = 8;
    /* Original constructor clears +100; its remaining role is unrecovered. */
    setword(entity, 0x100, 0);
    bh_field_store_u32(&entity->remote.sound_frames, 0);
    bh_field_store_u32(&entity->remote.reserved_sound, BH_SOUND_INVALID);
    bh_field_store_u32(&entity->remote.engine_sound, BH_SOUND_INVALID);
    bh_field_store_u32(&entity->remote.engine_resource, packets->engine_sounds[slot]);
    next = *packets->head;
    entity->actor.global_next = next;
    *packets->head = entity;
    entity->actor.global_previous = 0;
    if (next)
        next->actor.global_previous = entity;
    ++*packets->count;
    if (room)
    {
        next = room->actors;
        entity->actor.room_next = next;
        room->actors = entity;
        entity->actor.room_previous = 0;
        if (next)
            next->actor.room_previous = entity;
    }
    entity->remote.newest.world = world;
    memcpy(&entity->bindings.remote_packets, &packets, 4);
    if (!bh_remote_tick_bind(packets->tick, entity) || !bh_entity_set_query(packets->pool, entity, query))
    {
        packets->error = 1;
        return 0;
    }
    bh_field_store_u32(&entity->remote.cached_world, 0);
    bh_field_store_u16(&entity->remote.muzzle_phase, 0);
    for (i = 0; i < 9; ++i)
        bh_field_store_u16(&entity->remote.firing[i].remaining, 0);
    ++packets->spawns;
    return done(packets);
}
int bh_remote_shot_packet(BHRemotePackets *packets, void *p, unsigned long n, unsigned long capacity)
{
    unsigned short sender;
    unsigned char slot, count, spacing, type, target_slot;
    unsigned long offset;
    short lag;
    BHEntity *entity, *target = 0;
    if (!open(packets, p, n, capacity))
        return 0;
    if (!packets->simulation_tick || !packets->local_slot || !packets->remote || !packets->tick ||
        !packets->tick->fire || !packets->tick->fire->sync_identity ||
        packets->tick->fire->tick != packets->simulation_tick)
    {
        packets->error = 1;
        return 0;
    }
    memcpy(&sender, packets->reader.data + 2, 2);
    lag = signed16(*packets->simulation_tick - sender);
    if (lag >= 61)
        return 1;
    if (!take(packets, &slot, 1))
        return 0;
    if (slot >= 8)
        return 1;
    if (!take(packets, &count, 1) || !take(packets, &spacing, 1) || !take(packets, &type, 1) ||
        !take(packets, &target_slot, 1))
        return 0;
    if (!bh_network_shot_pattern_valid(type, count, spacing))
    {
        packets->error = 4;
        --packets->pending;
        ++packets->bad;
        return 0;
    }
    if ((long)target_slot == *packets->local_slot)
        target = packets->local;
    else if (target_slot < 8)
        target = packets->remote[target_slot];
    entity = packets->remote[slot];
    if (!entity)
        return 1;

    bh_field_store_u16(&entity->remote.firing[type].packet_count, count);
    bh_field_store_u32(&entity->remote.firing[type].target, (unsigned long)target);
    if (type == 5)
    {
        if (!bh_field_i16(&entity->remote.firing[5].remaining))
            bh_field_store_u16(&entity->remote.firing[5].remaining, 60);
        else if (bh_field_i16(&entity->remote.firing[5].remaining) < 30)
            bh_field_store_u16(&entity->remote.firing[5].remaining, 30);
        return done(packets);
    }
    if (count == 1)
    {
        lag = 0;
        *packets->tick->fire->sync_identity = ((unsigned long)(slot | 0x4000UL) << 16) | sender;
    }
    while (lag >= 0)
    {
        if (count < 1)
            return done(packets);
        if (!bh_remote_fire(packets->tick->fire, entity, (short)sender, type, target))
        {
            packets->error = 6;
            return 0;
        }
        ++packets->immediate;
        lag = signed16((unsigned short)lag - spacing);
        --count;
        sender = (unsigned short)(sender + spacing);
    }
    if (count)
    {
        bh_field_store_u16(
            &entity->remote.firing[type].remaining,
            (unsigned short)((unsigned short)bh_field_i16(&entity->remote.firing[type].remaining) + count));
        bh_field_store_u16(&entity->remote.firing[type].spacing, spacing);
        bh_field_store_u16(&entity->remote.firing[type].countdown, (unsigned short)(0 - (unsigned short)lag));
        packets->queued += count;
    }
    return done(packets);
}
