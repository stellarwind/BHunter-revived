#include "bh_remote_messages.h"
#include "bh_callback_helpers.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
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
static short delta(unsigned short a, unsigned short b)
{
    unsigned short u = (unsigned short)(a - b);
    short s;
    memcpy(&s, &u, 2);
    return s;
}
static int take(BHRemoteMessages *c, void *out, unsigned long n)
{
    BHPacketReader *r = &c->packets->reader;
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
static int open(BHRemoteMessages *c, void *p, unsigned long size, unsigned long capacity)
{
    BHRemotePackets *r = c->packets;
    ++r->pending;
    if (!bh_packet_receive(&r->reader, p, size, capacity, &r->framing))
    {
        --r->pending;
        ++r->bad;
        c->error = 2;
        return 0;
    }
    return 1;
}
static int done(BHRemoteMessages *c)
{
    --c->packets->pending;
    ++c->packets->processed;
    return !c->error;
}
static int hud(BHRemoteMessages *c)
{
    if (!c->scores || !c->scores->scores || !c->scores->text)
    {
        c->error = 1;
        return 0;
    }
    c->scores->scores->local_index = *c->packets->local_slot;
    if (!bh_hud_scores_text(c->scores->text, c->scores->scores))
        c->error = 4;
    return !c->error;
}
static int score(BHRemoteMessages *c)
{
    if (!c->scores || !c->scores->scores)
    {
        c->error = 1;
        return 0;
    }
    c->scores->frame = (unsigned short)*c->packets->simulation_tick;
    c->scores->scores->local_index = *c->packets->local_slot;
    if (!bh_network_scores_broadcast(c->scores))
        c->error = 4;
    return !c->error;
}
static int send_packet(BHRemoteMessages *c, unsigned char type, unsigned short tick, const void *p, unsigned long n,
                       DPID to, int reliable)
{
    if (!c->transport || !bh_packet_begin(&c->writer, c->outgoing, 64, type, tick) ||
        !bh_packet_append(&c->writer, p, n))
    {
        c->error = 1;
        return 0;
    }
    c->send_result = bh_network_packet_send(c->transport, &c->writer, to, reliable);
    ++c->sends;
    return 1;
}
static int announce(BHRemoteMessages *c, unsigned long slot, DPID to)
{
    unsigned char payload[25];
    BHNetworkPlayer *p = c->packets->network->players + slot;
    unsigned long j;
    memcpy(payload, &p->id, 4);
    payload[4] = (unsigned char)slot;
    for (j = 0; j < 19 && p->name[j]; ++j)
        payload[5 + j] = (unsigned char)p->name[j];
    memset(payload + 5 + j, 0, 20 - j);
    return send_packet(c, 3, (unsigned short)*c->packets->simulation_tick, payload, 25, to, 1);
}
static void __cdecl query(BHEntity *e, void *packet)
{
    BHRemotePackets *c;
    memcpy(&c, e->payload + 536, 4);
    if (c && c->query)
    {
        c->query->tick = (unsigned short)*c->simulation_tick;
        bh_remote_query(c->query, e, (BHRemoteQueryPacket *)packet);
    }
}
static int birth(BHRemoteMessages *c, unsigned long slot)
{
    BHRemotePackets *r = c->packets;
    BHNetworkPlayer *p = r->network->players + slot;
    BHEntity *e, *next;
    BHWorldGeometry *world;
    BHPickupRoom *room = 0;
    unsigned long i;
    unsigned short stamp;
    if (!r->pool || !r->head || !r->count || !r->spawn_world || !r->tick || !r->query || r->remote[slot])
        return 0;
    world = *r->spawn_world;
    if (world)
    {
        for (i = 0; i < r->room_count; ++i)
            if (r->rooms[i].world == world)
            {
                room = r->rooms + i;
                break;
            }
        if (!room)
            return 0;
    }
    e = bh_entity_acquire(r->pool);
    if (!e)
        return 0;
    r->remote[slot] = e;
    sethalf(e, 0x114, (unsigned short)slot);
    sethalf(e, 0x116, (unsigned short)slot);
    setword(e, 0x138, p->id);
    setword(e, 0xe8, 0x00423c80UL);
    strncpy((char *)e + 0x11e, p->name, 19);
    *((char *)e + 0x131) = 0;
    stamp = (unsigned short)*r->simulation_tick;
    sethalf(e, 0x17c, stamp);
    sethalf(e, 0x15c, (unsigned short)(stamp - 1));
    sethalf(e, 0x13c, (unsigned short)(stamp - 2));
    for (i = 0; i < 3; ++i)
        memset((char *)e + 0x140 + 32 * i, 0, 24);
    memset((char *)e + 0x48, 0, 48);
    e->flags |= 0x100;
    sethalf(e, 0x11a, 0);
    sethalf(e, 0x11c, 0);
    setword(e, 0xfc, 8);
    setword(e, 0x100, 0);
    setword(e, 0x108, 0);
    setword(e, 0x110, BH_SOUND_INVALID);
    setword(e, 0x104, BH_SOUND_INVALID);
    setword(e, 0x10c, r->engine_sounds[slot]);
    next = *r->head;
    setword(e, 0xf0, (unsigned long)next);
    *r->head = e;
    setword(e, 0xec, 0);
    if (next)
        setword(next, 0xec, (unsigned long)e);
    ++*r->count;
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
    setword(e, 0x134, 0);
    sethalf(e, 0x118, 0);
    for (i = 0; i < 9; ++i)
        sethalf(e, 0x19c + 12 * i, 0);
    memcpy(e->payload + 536, &r, 4);
    if (!bh_remote_tick_bind(r->tick, e) || !bh_entity_set_query(r->pool, e, query))
        return 0;
    ++r->spawns;
    return 1;
}
static int retire(BHRemoteMessages *c, unsigned long slot)
{
    BHRemotePackets *r = c->packets;
    BHEntity *e = r->remote[slot], *previous, *next;
    BHWorldGeometry *world;
    BHPickupRoom *room = 0;
    unsigned long i;
    BHRemoteQueryPacket packet;
    if (!e || !r->head || !r->count || !*r->count)
        return 0;
    world = (BHWorldGeometry *)word(e, 0x198);
    if (world)
    {
        for (i = 0; i < r->room_count; ++i)
            if (r->rooms[i].world == world)
            {
                room = r->rooms + i;
                break;
            }
        if (!room)
            return 0;
        previous = (BHEntity *)word(e, 0xf8);
        next = (BHEntity *)word(e, 0xf4);
        if ((previous ? (BHEntity *)word(previous, 0xf4) : room->actors) != e ||
            (next && (BHEntity *)word(next, 0xf8) != e))
            return 0;
    }
    previous = (BHEntity *)word(e, 0xec);
    next = (BHEntity *)word(e, 0xf0);
    if ((previous ? (BHEntity *)word(previous, 0xf0) : *r->head) != e || (next && (BHEntity *)word(next, 0xec) != e))
        return 0;
    r->remote[slot] = 0;
    if (room)
    {
        previous = (BHEntity *)word(e, 0xf8);
        next = (BHEntity *)word(e, 0xf4);
        if (previous)
            setword(previous, 0xf4, (unsigned long)next);
        else
            room->actors = next;
        if (next)
            setword(next, 0xf8, (unsigned long)previous);
    }
    previous = (BHEntity *)word(e, 0xec);
    next = (BHEntity *)word(e, 0xf0);
    if (previous)
        setword(previous, 0xf0, (unsigned long)next);
    else
        *r->head = next;
    if (next)
        setword(next, 0xec, (unsigned long)previous);
    --*r->count;
    packet.words[0] = 17;
    packet.words[1] = (long)e;
    if (!bh_entities_query(r->pool, 0, 0, 0, 0, &packet))
        return 0;
    ++c->departures;
    return bh_entity_release(r->pool, e);
}
static int native(BHRemoteMessages *c, void *p, unsigned long size, int host)
{
    BHRemotePackets *r = c->packets;
    BHNetworkPlayer *player;
    unsigned short slot;
    unsigned long i;
    char text[128];
    int entering;
    if (size < 6 || !r->network || !c->messages)
    {
        c->error = 1;
        return 0;
    }
    memcpy(&slot, (char *)p + 4, 2);
    if (slot > 7)
    {
        c->error = 1;
        return 0;
    }
    player = r->network->players + slot;
    for (i = 0; i < 32 && player->name[i]; ++i)
    {
    }
    if (i == 32)
    {
        c->error = 1;
        return 0;
    }
    entering = player->active != 0;
    sprintf(text,
            entering ? "1BHunter %s\nhas entered\nthe city."
            : host   ? "1BHunter %s\nhas left\nthe city."
                     : "1BHunter %s\nhas left the city.",
            player->name);
    if (!bh_message_enqueue(c->messages, text))
        c->error = 4;
    if (!entering)
    {
        if (host)
        {
            if (!retire(c, slot))
                c->error = 5;
        }
        else
            for (i = 0; i < 8; ++i)
                if (r->remote[i] && word(r->remote[i], 0x138) == player->id)
                    if (!retire(c, i))
                        c->error = 5;
        return !c->error;
    }
    if (!host)
    {
        for (i = 0; i < 8; ++i)
            if (r->remote[i] && word(r->remote[i], 0x138) == player->id)
                sethalf(r->remote[i], 0x116, slot);
        hud(c);
        return !c->error;
    }
    if (!announce(c, slot, 0))
        return 0;
    for (i = 0; i < 8; ++i)
        if (i != slot && r->network->players[i].active)
            if (!announce(c, i, player->id))
                return 0;
    if (!birth(c, slot))
    {
        c->error = 5;
        return 0;
    }
    score(c);
    return !c->error;
}
static int helper(BHRemoteMessages *c, void *p, unsigned long n, unsigned long capacity, int launch)
{
    BHPacketStats stats = {0, 0}, *old_stats;
    unsigned long *old_good;
    int result;
    BHRemotePackets *r = c->packets;
    if (launch)
    {
        if (!c->launch)
        {
            c->error = 1;
            return 0;
        }
        old_stats = c->launch->stats;
        old_good = c->launch->good;
        c->launch->stats = &stats;
        c->launch->good = &r->processed;
        result = bh_actor_launch_receive(c->launch, p, n, capacity);
        r->reader = c->launch->reader;
        c->launch->stats = old_stats;
        c->launch->good = old_good;
        c->launch->reader.stats = old_stats;
    }
    else
    {
        if (!c->effects)
        {
            c->error = 1;
            return 0;
        }
        old_stats = c->effects->stats;
        old_good = c->effects->good;
        c->effects->stats = &stats;
        c->effects->good = &r->processed;
        result = bh_network_effect_receive(c->effects, p, n, capacity);
        r->reader = c->effects->reader;
        c->effects->stats = old_stats;
        c->effects->good = old_good;
        c->effects->reader.stats = old_stats;
    }
    r->pending += stats.accepted;
    r->bad += stats.rejected;
    r->reader.stats = &r->framing;
    if (result < 1)
        c->error = 6;
    return result > 0;
}
static int position(BHRemoteMessages *c, BHVec3 *point)
{
    BHWorldGeometry *world = 0;
    int result;
    if (!c->packets->navigation)
    {
        c->error = 1;
        return 0;
    }
    result = bh_packet_read_position(&c->packets->reader, c->packets->navigation, &world, point);
    if (result != 1)
    {
        if (!result)
        {
            --c->packets->pending;
            ++c->packets->bad;
        }
        c->error = 3;
        return 0;
    }
    return 1;
}
static int sound(BHRemoteMessages *c)
{
    BHVec3 point;
    unsigned char payload[4];
    long resource;
    unsigned long i;
    int random;
    if (!position(c, &point) || !take(c, payload, 4))
        return 0;
    if (!c->sound || (payload[3] && !c->contacts))
    {
        c->error = 1;
        return 0;
    }
    resource = (long)(signed char)payload[0];
    if (resource < -1)
        resource += 256;
    bh_sound_request(c->sound, (unsigned long)resource, rf((double)payload[1] * (1.0 / 128)),
                     rf((double)payload[2] * (1.0 / 64)), &point, 0);
    ++c->sounds;
    for (i = 0; i < payload[3]; ++i)
    {
        random = rand();
        bh_contact_spark_create(c->contacts, &point, rf(.7 - (double)random * (-1.0 / 131072)), 1);
        ++c->sparks;
    }
    return done(c);
}
static int damage(BHRemoteMessages *c)
{
    BHVec3 point;
    unsigned short slot;
    unsigned char severity;
    BHRemoteQueryPacket packet;
    BHRemotePackets *r = c->packets;
    BHEntity *attacker = 0;
    if (!position(c, &point) || !take(c, &slot, 2) || !take(c, &severity, 1))
        return 0;
    if ((long)slot == *r->local_slot)
        attacker = r->local;
    else if (slot < 8)
        attacker = r->remote[slot];
    else
    {
        --r->pending;
        ++r->bad;
    }
    if (!r->local || !r->local->query)
    {
        c->error = 1;
        return 0;
    }
    packet.damage.event = 0;
    packet.damage.severity = rf((double)severity * (1.0 / 128));
    packet.damage.attacker = attacker;
    r->local->query(r->local, &packet);
    return done(c);
}
static int clock(BHRemoteMessages *c, int host)
{
    BHRemotePackets *r = c->packets;
    unsigned short sender, value;
    unsigned char slot, peers;
    memcpy(&sender, r->reader.data + 2, 2);
    if (host)
    {
        if (!c->latency)
        {
            c->error = 1;
            return 0;
        }
        if (!take(c, &slot, 1))
            return 0;
        if (slot < 8)
        {
            c->latency[slot] =
                rf((double)delta((unsigned short)*r->simulation_tick, sender) * .1 - (double)c->latency[slot] * (-.8));
            return done(c);
        }
        return 1;
    }
    if (!c->peers || *r->local_slot < 0 || *r->local_slot > 6)
    {
        c->error = 1;
        return 0;
    }
    if (r->reader.size - r->reader.cursor < 15)
    {
        c->error = 3;
        return 0;
    }
    memcpy(&value, r->reader.data + 8 + *r->local_slot * 2, 2);
    peers = r->reader.data[22];
    r->reader.cursor = 23;
    *c->peers = peers;
    *r->simulation_tick = (*r->simulation_tick & 0xffff0000UL) | value;
    slot = (unsigned char)*r->local_slot;
    if (!send_packet(c, 5, value, &slot, 1, 1, 0))
        return 0;
    return done(c);
}
static int scores(BHRemoteMessages *c, int host)
{
    BHRemotePackets *r = c->packets;
    unsigned long i, bits;
    unsigned short values[8], value;
    signed char ids[2];
    BHEntity *e;
    if (!c->scores || !c->scores->scores)
    {
        c->error = 1;
        return 0;
    }
    if (host)
    {
        if (!take(c, ids, 2))
            return 0;
        done(c);
        for (i = 0; i < 2; ++i)
            if (ids[i] >= 0 && ids[i] < 8)
            {
                if ((long)ids[i] == *r->local_slot)
                {
                    bits = (unsigned long)c->scores->scores->local_score + (i ? 1UL : 0xffffffffUL);
                    memcpy(&c->scores->scores->local_score, &bits, 4);
                }
                else if ((e = r->remote[(unsigned char)ids[i]]) != 0)
                {
                    value = (unsigned short)half(e, 0x11c);
                    sethalf(e, 0x11c, (unsigned short)(value + (i ? 1 : -1)));
                }
            }
        return score(c);
    }
    if (!take(c, values, 16))
        return 0;
    for (i = 0; i < 8; ++i)
    {
        if ((long)i == *r->local_slot)
            c->scores->scores->local_score = (long)delta(values[i], 0);
        else if (r->remote[i])
            sethalf(r->remote[i], 0x11c, values[i]);
    }
    done(c);
    return hud(c);
}
static int message(BHRemoteMessages *c, void *p, unsigned long n, unsigned long capacity, int host)
{
    unsigned char type;
    unsigned short sender;
    short age;
    int result;
    if (!c || !c->packets || !p || !n || !c->packets->remote || !c->packets->simulation_tick ||
        !c->packets->local_slot || !bh_math_environment_supported())
    {
        if (c)
            c->error = 1;
        return 0;
    }
    c->error = 0;
    type = *(unsigned char *)p;
    if (!type)
        return native(c, p, n, host);
    switch (type)
    {
    case 3:
        if (!host)
        {
            result = bh_remote_spawn_packet(c->packets, p, n, capacity);
            hud(c);
            return result && !c->error;
        }
        return 1;
    case 4:
        return bh_remote_position_packet(c->packets, p, n, capacity);
    case 8:
        return bh_remote_shot_packet(c->packets, p, n, capacity);
    case 9:
        return helper(c, p, n, capacity, 0);
    case 10:
        if (!host)
            return helper(c, p, n, capacity, 1);
        return 1;
    case 5:
    case 6:
    case 7:
        break;
    case 11:
        if (!host)
            return 1;
        break;
    case 12:
        if (host)
            return 1;
        break;
    default:
        return 1;
    }
    if (!open(c, p, n, capacity))
        return 0;
    memcpy(&sender, c->packets->reader.data + 2, 2);
    age = delta((unsigned short)*c->packets->simulation_tick, sender);
    switch (type)
    {
    case 5:
        return clock(c, host);
    case 6:
        if (age < 16)
            return sound(c);
        return 1;
    case 7:
        if (age < 31)
            return damage(c);
        return 1;
    case 11:
    case 12:
        return scores(c, host);
    }
    return 1;
}
int __cdecl bh_remote_host_message(BHRemoteMessages *c, void *p, unsigned long n, unsigned long capacity)
{
    return message(c, p, n, capacity, 1);
}
int __cdecl bh_remote_client_message(BHRemoteMessages *c, void *p, unsigned long n, unsigned long capacity)
{
    return message(c, p, n, capacity, 0);
}
