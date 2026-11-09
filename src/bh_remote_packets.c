#include "bh_remote_packets.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include <string.h>
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
static short half(BHEntity *entity, unsigned long o)
{
    short n;
    memcpy(&n, (char *)entity + o, 2);
    return n;
}
static void sethalf(BHEntity *entity, unsigned long o, unsigned short n)
{
    memcpy((char *)entity + o, &n, 2);
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
    memcpy(&packets, entity->payload + 536, 4);
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
    memcpy(&bound, entity->payload + 536, 4);
    if (bound != packets || !packets->remote || half(entity, 0x114) < 0 ||
        half(entity, 0x114) > 7 || packets->remote[half(entity, 0x114)] != entity)
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
    BHEntityStep draw = skip_room_draw;
    unsigned int car = packets->network->players[slot].car;
    if (packets->car_models && car < BH_NETWORK_CARS)
        packets->render->models[slot] = packets->car_models[car];
    memcpy((char *)entity + 0xe8, &draw, sizeof(draw));
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
    sethalf(entity, 0x114, slot);
    setword(entity, 0x138, id);
    sethalf(entity, 0x116, 0xffff);
    for (i = 1; i < 8; ++i)
        if (packets->network->players[i].active && packets->network->players[i].id == id)
            sethalf(entity, 0x116, (unsigned short)i);
    bh_remote_bind_draw(packets, entity, slot);
    stamp = (unsigned short)*packets->simulation_tick;
    sethalf(entity, 0x17c, stamp);
    sethalf(entity, 0x15c, (unsigned short)(stamp - 1));
    sethalf(entity, 0x13c, (unsigned short)(stamp - 2));
    for (i = 0; i < 3; ++i)
        memset((char *)entity + 0x140 + 32 * i, 0, 24);
    for (j = 0; j < 19 && name[j]; ++j)
        *((char *)entity + 0x11e + j) = (char)name[j];
    *((char *)entity + 0x11e + j) = 0;
    entity->flags |= 0x100;
    sethalf(entity, 0x11a, 0);
    sethalf(entity, 0x11c, 0);
    memset((char *)entity + 0x48, 0, 48);
    setword(entity, 0xfc, 8);
    setword(entity, 0x100, 0);
    setword(entity, 0x108, 0);
    setword(entity, 0x110, BH_SOUND_INVALID);
    setword(entity, 0x104, BH_SOUND_INVALID);
    setword(entity, 0x10c, packets->engine_sounds[slot]);
    next = *packets->head;
    setword(entity, 0xf0, (unsigned long)next);
    *packets->head = entity;
    setword(entity, 0xec, 0);
    if (next)
        setword(next, 0xec, (unsigned long)entity);
    ++*packets->count;
    if (room)
    {
        next = room->actors;
        setword(entity, 0xf4, (unsigned long)next);
        room->actors = entity;
        setword(entity, 0xf8, 0);
        if (next)
            setword(next, 0xf8, (unsigned long)entity);
    }
    setword(entity, 0x198, (unsigned long)world);
    memcpy(entity->payload + 536, &packets, 4);
    if (!bh_remote_tick_bind(packets->tick, entity) || !bh_entity_set_query(packets->pool, entity, query))
    {
        packets->error = 1;
        return 0;
    }
    setword(entity, 0x134, 0);
    sethalf(entity, 0x118, 0);
    for (i = 0; i < 9; ++i)
        sethalf(entity, 0x19c + 12 * i, 0);
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
    offset = 0x19c + 12 * type;
    sethalf(entity, offset + 6, count);
    setword(entity, offset + 8, (unsigned long)target);
    if (type == 5)
    {
        if (!half(entity, 0x1d8))
            sethalf(entity, 0x1d8, 60);
        else if (half(entity, 0x1d8) < 30)
            sethalf(entity, 0x1d8, 30);
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
        sethalf(entity, offset, (unsigned short)((unsigned short)half(entity, offset) + count));
        sethalf(entity, offset + 2, spacing);
        sethalf(entity, offset + 4, (unsigned short)(0 - (unsigned short)lag));
        packets->queued += count;
    }
    return done(packets);
}
