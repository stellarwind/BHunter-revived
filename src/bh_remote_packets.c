#include "bh_remote_packets.h"
#include <string.h>
static unsigned long word(BHEntity *e, unsigned long o)
{
    unsigned long n;
    memcpy(&n, (char *)e + o, 4);
    return n;
}
static void setword(BHEntity *e, unsigned long o, unsigned long n)
{
    memcpy((char *)e + o, &n, 4);
}
static short half(BHEntity *e, unsigned long o)
{
    short n;
    memcpy(&n, (char *)e + o, 2);
    return n;
}
static void sethalf(BHEntity *e, unsigned long o, unsigned short n)
{
    memcpy((char *)e + o, &n, 2);
}
static short signed16(unsigned long n)
{
    unsigned short u = (unsigned short)n;
    short s;
    memcpy(&s, &u, 2);
    return s;
}
static int open(BHRemotePackets *c, void *p, unsigned long n, unsigned long capacity)
{
    if (!c || !p)
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->error = 0;
    ++c->pending;
    if (!bh_packet_receive(&c->reader, p, n, capacity, &c->framing))
    {
        --c->pending;
        ++c->bad;
        c->error = 2;
        return 0;
    }
    return 1;
}
static int take(BHRemotePackets *c, void *out, unsigned long n)
{
    BHPacketReader *r = &c->reader;
    if (r->cursor > r->size || n > r->size - r->cursor)
    {
        c->error = 3;
        r->error = BH_PACKET_TRUNCATED;
        return 0;
    }
    memcpy(out, r->data + r->cursor, n);
    r->cursor += n;
    return 1;
}
static int done(BHRemotePackets *c)
{
    --c->pending;
    ++c->processed;
    return !c->error;
}
static void __cdecl query(BHEntity *e, void *p)
{
    BHRemotePackets *c;
    memcpy(&c, e->payload + 536, 4);
    if (c && c->query)
    {
        c->query->tick = (unsigned short)*c->simulation_tick;
        bh_remote_query(c->query, e, (BHRemoteQueryPacket *)p);
    }
}
int __cdecl bh_remote_entity_draw(BHRemotePackets *c, BHEntity *e)
{
    BHRemotePackets *bound;
    if (!c || !e || !c->render || !c->simulation_tick)
        return 0;
    memcpy(&bound, e->payload + 536, 4);
    if (bound != c || word(e, 0xe8) != 0x00423c80UL || !c->remote || half(e, 0x114) < 0 || half(e, 0x114) > 7 ||
        c->remote[half(e, 0x114)] != e)
        return 0;
    c->render->tick = (unsigned short)*c->simulation_tick;
    return bh_remote_render(c->render, e);
}
int __cdecl bh_remote_spawn_packet(BHRemotePackets *c, void *p, unsigned long n, unsigned long capacity)
{
    unsigned long id, i, j;
    unsigned char slot, name[20];
    BHEntity *e, *next;
    BHPickupRoom *room = 0;
    BHWorldGeometry *world;
    unsigned short stamp;
    if (!open(c, p, n, capacity))
        return 0;
    if (!c->pool || !c->remote || !c->head || !c->count || !c->network || !c->simulation_tick || !c->local_slot ||
        !c->query || !c->render || !c->tick || !c->tick->fire || c->tick->fire->tick != c->simulation_tick ||
        !c->spawn_world)
    {
        c->error = 1;
        return 0;
    }
    if (!take(c, &id, 4) || !take(c, &slot, 1))
        return 0;
    if (slot >= 8)
    {
        c->error = 4;
        --c->pending;
        ++c->bad;
        return 0;
    }
    if (id == c->network->local_player)
    {
        memcpy(&stamp, c->reader.data + 2, 2);
        *c->simulation_tick = (*c->simulation_tick & 0xffff0000UL) | stamp;
        *c->local_slot = slot;
        return done(c);
    }
    if (!take(c, name, 20))
        return 0;
    if (c->remote[slot])
    {
        c->error = 4;
        return 0;
    }
    world = *c->spawn_world;
    if (world)
    {
        if (!c->rooms)
        {
            c->error = 1;
            return 0;
        }
        for (i = 0; i < c->room_count; ++i)
            if (c->rooms[i].world == world)
            {
                room = c->rooms + i;
                break;
            }
        if (!room)
        {
            c->error = 1;
            return 0;
        }
    }
    e = bh_entity_acquire(c->pool);
    if (!e)
    {
        c->error = 5;
        return 0;
    }
    c->remote[slot] = e;
    sethalf(e, 0x114, slot);
    setword(e, 0x138, id);
    sethalf(e, 0x116, 0xffff);
    for (i = 1; i < 8; ++i)
        if (c->network->players[i].active && c->network->players[i].id == id)
            sethalf(e, 0x116, (unsigned short)i);
    setword(e, 0xe8, 0x00423c80UL);
    stamp = (unsigned short)*c->simulation_tick;
    sethalf(e, 0x17c, stamp);
    sethalf(e, 0x15c, (unsigned short)(stamp - 1));
    sethalf(e, 0x13c, (unsigned short)(stamp - 2));
    for (i = 0; i < 3; ++i)
        memset((char *)e + 0x140 + 32 * i, 0, 24);
    for (j = 0; j < 19 && name[j]; ++j)
        *((char *)e + 0x11e + j) = (char)name[j];
    *((char *)e + 0x11e + j) = 0;
    e->flags |= 0x100;
    sethalf(e, 0x11a, 0);
    sethalf(e, 0x11c, 0);
    memset((char *)e + 0x48, 0, 48);
    setword(e, 0xfc, 8);
    setword(e, 0x100, 0);
    setword(e, 0x108, 0);
    setword(e, 0x110, BH_SOUND_INVALID);
    setword(e, 0x104, BH_SOUND_INVALID);
    setword(e, 0x10c, c->engine_sounds[slot]);
    next = *c->head;
    setword(e, 0xf0, (unsigned long)next);
    *c->head = e;
    setword(e, 0xec, 0);
    if (next)
        setword(next, 0xec, (unsigned long)e);
    ++*c->count;
    if (room)
    {
        next = room->actors;
        setword(e, 0xf4, (unsigned long)next);
        room->actors = e;
        setword(e, 0xf8, 0);
        if (next)
            setword(next, 0xf8, (unsigned long)e);
    }
    setword(e, 0x198, (unsigned long)world);
    memcpy(e->payload + 536, &c, 4);
    if (!bh_remote_tick_bind(c->tick, e) || !bh_entity_set_query(c->pool, e, query))
    {
        c->error = 1;
        return 0;
    }
    setword(e, 0x134, 0);
    sethalf(e, 0x118, 0);
    for (i = 0; i < 9; ++i)
        sethalf(e, 0x19c + 12 * i, 0);
    ++c->spawns;
    return done(c);
}
int __cdecl bh_remote_shot_packet(BHRemotePackets *c, void *p, unsigned long n, unsigned long capacity)
{
    unsigned short sender;
    unsigned char slot, count, spacing, type, target_slot;
    unsigned long offset;
    short lag;
    BHEntity *e, *target = 0;
    if (!open(c, p, n, capacity))
        return 0;
    if (!c->simulation_tick || !c->local_slot || !c->remote || !c->tick || !c->tick->fire ||
        !c->tick->fire->sync_identity || c->tick->fire->tick != c->simulation_tick)
    {
        c->error = 1;
        return 0;
    }
    memcpy(&sender, c->reader.data + 2, 2);
    lag = signed16(*c->simulation_tick - sender);
    if (lag >= 61)
        return 1;
    if (!take(c, &slot, 1))
        return 0;
    if (slot >= 8)
        return 1;
    if (!take(c, &count, 1) || !take(c, &spacing, 1) || !take(c, &type, 1) || !take(c, &target_slot, 1))
        return 0;
    if (type > 8)
    {
        c->error = 4;
        --c->pending;
        ++c->bad;
        return 0;
    }
    if ((long)target_slot == *c->local_slot)
        target = c->local;
    else if (target_slot < 8)
        target = c->remote[target_slot];
    e = c->remote[slot];
    if (!e)
        return 1;
    offset = 0x19c + 12 * type;
    sethalf(e, offset + 6, count);
    setword(e, offset + 8, (unsigned long)target);
    if (type == 5)
    {
        if (!half(e, 0x1d8))
            sethalf(e, 0x1d8, 60);
        else if (half(e, 0x1d8) < 30)
            sethalf(e, 0x1d8, 30);
        return done(c);
    }
    if (count == 1)
    {
        lag = 0;
        *c->tick->fire->sync_identity = ((unsigned long)(slot | 0x4000UL) << 16) | sender;
    }
    while (lag >= 0)
    {
        if (count < 1)
            return done(c);
        if (!bh_remote_fire(c->tick->fire, e, (short)sender, type, target))
        {
            c->error = 6;
            return 0;
        }
        ++c->immediate;
        lag = signed16((unsigned short)lag - spacing);
        --count;
        sender = (unsigned short)(sender + spacing);
    }
    if (count)
    {
        sethalf(e, offset, (unsigned short)((unsigned short)half(e, offset) + count));
        sethalf(e, offset + 2, spacing);
        sethalf(e, offset + 4, (unsigned short)(0 - (unsigned short)lag));
        c->queued += count;
    }
    return done(c);
}
