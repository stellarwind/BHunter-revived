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
static short delta(unsigned short a, unsigned short b)
{
    unsigned short u = (unsigned short)(a - b);
    short s;
    memcpy(&s, &u, 2);
    return s;
}
static int take(BHRemoteMessages *messages, void *out, unsigned long n)
{
    BHPacketReader *reader = &messages->packets->reader;
    if (reader->cursor > reader->size || n > reader->size - reader->cursor)
    {
        messages->error = 3;
        reader->error = BH_PACKET_TRUNCATED;
        return 0;
    }
    memcpy(out, reader->data + reader->cursor, n);
    reader->cursor += n;
    return 1;
}
static int open(BHRemoteMessages *messages, void *p, unsigned long size, unsigned long capacity)
{
    BHRemotePackets *packets = messages->packets;
    ++packets->pending;
    if (!bh_packet_receive(&packets->reader, p, size, capacity, &packets->framing))
    {
        --packets->pending;
        ++packets->bad;
        messages->error = 2;
        return 0;
    }
    return 1;
}
static int done(BHRemoteMessages *messages)
{
    --messages->packets->pending;
    ++messages->packets->processed;
    return !messages->error;
}
static int hud(BHRemoteMessages *messages)
{
    if (!messages->scores || !messages->scores->scores || !messages->scores->text)
    {
        messages->error = 1;
        return 0;
    }
    messages->scores->scores->local_index = *messages->packets->local_slot;
    if (!bh_hud_scores_text(messages->scores->text, messages->scores->scores))
        messages->error = 4;
    return !messages->error;
}
static int score(BHRemoteMessages *messages)
{
    if (!messages->scores || !messages->scores->scores)
    {
        messages->error = 1;
        return 0;
    }
    messages->scores->frame = (unsigned short)*messages->packets->simulation_tick;
    messages->scores->scores->local_index = *messages->packets->local_slot;
    if (!bh_network_scores_broadcast(messages->scores))
        messages->error = 4;
    return !messages->error;
}
static int send_packet(BHRemoteMessages *messages, unsigned char type, unsigned short tick, const void *p,
                       unsigned long n, unsigned long to, int reliable)
{
    if (!messages->transport || !bh_packet_begin(&messages->writer, messages->outgoing, 64, type, tick) ||
        !bh_packet_append(&messages->writer, p, n))
    {
        messages->error = 1;
        return 0;
    }
    messages->send_result = bh_network_packet_send(messages->transport, &messages->writer, to, reliable);
    ++messages->sends;
    return 1;
}
static int announce(BHRemoteMessages *messages, unsigned long slot, unsigned long to)
{
    unsigned char payload[BH_NETWORK_ROSTER_BYTES];
    BHNetworkPlayer *player = messages->packets->network->players + slot;
    unsigned long j;
    memcpy(payload, &player->id, 4);
    payload[4] = (unsigned char)slot;
    for (j = 0; j < 19 && player->name[j]; ++j)
        payload[5 + j] = (unsigned char)player->name[j];
    memset(payload + 5 + j, 0, 20 - j);
    payload[BH_NETWORK_ROSTER_BYTES - 1] = player->car;
    return send_packet(messages, 3, (unsigned short)*messages->packets->simulation_tick, payload, sizeof(payload), to, 1);
}
static void query(BHEntity *entity, void *packet)
{
    BHRemotePackets *packets;
    memcpy(&packets, entity->payload + 536, 4);
    if (packets && packets->query)
    {
        packets->query->sound = packets->tick->sound;
        packets->query->tick = (unsigned short)*packets->simulation_tick;
        bh_remote_query(packets->query, entity, (BHRemoteQueryPacket *)packet);
    }
}
static int birth(BHRemoteMessages *messages, unsigned long slot)
{
    BHRemotePackets *packets = messages->packets;
    BHNetworkPlayer *player = packets->network->players + slot;
    BHEntity *entity, *next;
    BHWorldGeometry *world;
    BHPickupRoom *room = 0;
    unsigned long i;
    unsigned short stamp;
    if (!packets->pool || !packets->head || !packets->count || !packets->spawn_world || !packets->tick ||
        !packets->query || packets->remote[slot])
        return 0;
    world = *packets->spawn_world;
    if (world)
    {
        for (i = 0; i < packets->room_count; ++i)
            if (packets->rooms[i].world == world)
            {
                room = packets->rooms + i;
                break;
            }
        if (!room)
            return 0;
    }
    entity = bh_entity_acquire(packets->pool);
    if (!entity)
        return 0;
    packets->remote[slot] = entity;
    sethalf(entity, 0x114, (unsigned short)slot);
    sethalf(entity, 0x116, (unsigned short)slot);
    setword(entity, 0x138, player->id);
    bh_remote_bind_draw(packets, entity, slot);
    strncpy((char *)entity + 0x11e, player->name, 19);
    *((char *)entity + 0x131) = 0;
    stamp = (unsigned short)*packets->simulation_tick;
    sethalf(entity, 0x17c, stamp);
    sethalf(entity, 0x15c, (unsigned short)(stamp - 1));
    sethalf(entity, 0x13c, (unsigned short)(stamp - 2));
    for (i = 0; i < 3; ++i)
        memset((char *)entity + 0x140 + 32 * i, 0, 24);
    memset((char *)entity + 0x48, 0, 48);
    entity->flags |= 0x100;
    sethalf(entity, 0x11a, 0);
    sethalf(entity, 0x11c, 0);
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
    setword(entity, 0x134, 0);
    sethalf(entity, 0x118, 0);
    for (i = 0; i < 9; ++i)
        sethalf(entity, 0x19c + 12 * i, 0);
    memcpy(entity->payload + 536, &packets, 4);
    if (!bh_remote_tick_bind(packets->tick, entity) || !bh_entity_set_query(packets->pool, entity, query))
        return 0;
    ++packets->spawns;
    return 1;
}
static int retire(BHRemoteMessages *messages, unsigned long slot)
{
    BHRemotePackets *packets = messages->packets;
    BHEntity *entity = packets->remote[slot], *previous, *next;
    BHWorldGeometry *world;
    BHPickupRoom *room = 0;
    unsigned long i;
    BHRemoteQueryPacket packet;
    if (!entity || !packets->head || !packets->count || !*packets->count)
        return 0;
    world = (BHWorldGeometry *)word(entity, 0x198);
    if (world)
    {
        for (i = 0; i < packets->room_count; ++i)
            if (packets->rooms[i].world == world)
            {
                room = packets->rooms + i;
                break;
            }
        if (!room)
            return 0;
        previous = (BHEntity *)word(entity, 0xf8);
        next = (BHEntity *)word(entity, 0xf4);
        if ((previous ? (BHEntity *)word(previous, 0xf4) : room->actors) != entity ||
            (next && (BHEntity *)word(next, 0xf8) != entity))
            return 0;
    }
    previous = (BHEntity *)word(entity, 0xec);
    next = (BHEntity *)word(entity, 0xf0);
    if ((previous ? (BHEntity *)word(previous, 0xf0) : *packets->head) != entity ||
        (next && (BHEntity *)word(next, 0xec) != entity))
        return 0;
    /* Departure bypasses the event-20 retirement path. Stop before the
       entity (and its only copy of the engine handle) is released. */
    i = word(entity, 0x104);
    if (i != BH_SOUND_INVALID)
    {
        if (!messages->sound)
            return 0;
        bh_sound_stop(messages->sound, i);
        setword(entity, 0x104, BH_SOUND_INVALID);
    }
    packets->remote[slot] = 0;
    if (room)
    {
        previous = (BHEntity *)word(entity, 0xf8);
        next = (BHEntity *)word(entity, 0xf4);
        if (previous)
            setword(previous, 0xf4, (unsigned long)next);
        else
            room->actors = next;
        if (next)
            setword(next, 0xf8, (unsigned long)previous);
    }
    previous = (BHEntity *)word(entity, 0xec);
    next = (BHEntity *)word(entity, 0xf0);
    if (previous)
        setword(previous, 0xf0, (unsigned long)next);
    else
        *packets->head = next;
    if (next)
        setword(next, 0xec, (unsigned long)previous);
    --*packets->count;
    packet.words[0] = 17;
    packet.words[1] = (long)entity;
    if (!bh_entities_query(packets->pool, 0, 0, 0, 0, &packet))
        return 0;
    ++messages->departures;
    return bh_entity_release(packets->pool, entity);
}
int bh_remote_roster(BHRemoteMessages *messages, unsigned int slot, int host)
{
    BHRemotePackets *packets = messages->packets;
    BHNetworkPlayer *player;
    unsigned long i;
    char text[128];
    int entering;
    if (slot >= 8)
        return 0;
    messages->error = 0;
    player = packets->network->players + slot;
    for (i = 0; i < 32 && player->name[i]; ++i)
    {
    }
    if (i == 32)
    {
        messages->error = 1;
        return 0;
    }
    entering = player->active != 0;
    sprintf(text,
            entering ? "1BHunter %s\nhas entered\nthe city."
            : host   ? "1BHunter %s\nhas left\nthe city."
                     : "1BHunter %s\nhas left the city.",
            player->name);
    if (!bh_message_enqueue(messages->messages, text))
        messages->error = 4;
    if (!entering)
    {
        if (host)
        {
            if (!retire(messages, slot))
                messages->error = 5;
        }
        else
            for (i = 0; i < 8; ++i)
                if (packets->remote[i] && word(packets->remote[i], 0x138) == player->id)
                    if (!retire(messages, i))
                        messages->error = 5;
        return !messages->error;
    }
    if (!host)
    {
        for (i = 0; i < 8; ++i)
            if (packets->remote[i] && word(packets->remote[i], 0x138) == player->id)
                sethalf(packets->remote[i], 0x116, slot);
        hud(messages);
        return !messages->error;
    }
    if (!announce(messages, slot, 0))
        return 0;
    for (i = 0; i < 8; ++i)
        if (i != slot && packets->network->players[i].active)
            if (!announce(messages, i, player->id))
                return 0;
    if (!birth(messages, slot))
    {
        messages->error = 5;
        return 0;
    }
    score(messages);
    return !messages->error;
}
int bh_remote_clear(BHRemoteMessages *messages)
{
    unsigned int slot;
    for (slot = 0; slot < 8; ++slot)
        if (messages->packets->remote[slot] && !retire(messages, slot))
            return 0;
    return 1;
}
static int helper(BHRemoteMessages *messages, void *p, unsigned long n, unsigned long capacity, int launch)
{
    BHPacketStats stats = {0, 0}, *old_stats;
    unsigned long *old_good;
    int result;
    BHRemotePackets *packets = messages->packets;
    if (launch)
    {
        if (!messages->launch)
        {
            messages->error = 1;
            return 0;
        }
        old_stats = messages->launch->stats;
        old_good = messages->launch->good;
        messages->launch->stats = &stats;
        messages->launch->good = &packets->processed;
        result = bh_actor_launch_receive(messages->launch, p, n, capacity);
        packets->reader = messages->launch->reader;
        messages->launch->stats = old_stats;
        messages->launch->good = old_good;
        messages->launch->reader.stats = old_stats;
    }
    else
    {
        if (!messages->effects)
        {
            messages->error = 1;
            return 0;
        }
        old_stats = messages->effects->stats;
        old_good = messages->effects->good;
        messages->effects->stats = &stats;
        messages->effects->good = &packets->processed;
        result = bh_network_effect_receive(messages->effects, p, n, capacity);
        packets->reader = messages->effects->reader;
        messages->effects->stats = old_stats;
        messages->effects->good = old_good;
        messages->effects->reader.stats = old_stats;
    }
    packets->pending += stats.accepted;
    packets->bad += stats.rejected;
    packets->reader.stats = &packets->framing;
    if (result < 1)
        messages->error = 6;
    return result > 0;
}
static int position(BHRemoteMessages *messages, BHVec3 *point)
{
    BHWorldGeometry *world = 0;
    int result;
    if (!messages->packets->navigation)
    {
        messages->error = 1;
        return 0;
    }
    result = bh_packet_read_position(&messages->packets->reader, messages->packets->navigation, &world, point);
    if (result != 1)
    {
        if (!result)
        {
            --messages->packets->pending;
            ++messages->packets->bad;
        }
        messages->error = 3;
        return 0;
    }
    return 1;
}
static int sound(BHRemoteMessages *messages)
{
    BHVec3 point;
    unsigned char payload[4];
    long resource;
    unsigned long i;
    int random;
    if (!position(messages, &point) || !take(messages, payload, 4))
        return 0;
    if (!messages->sound || (payload[3] && !messages->contacts))
    {
        messages->error = 1;
        return 0;
    }
    resource = (long)(signed char)payload[0];
    if (resource < -1)
        resource += 256;
    bh_sound_request(messages->sound, (unsigned long)resource, rf((double)payload[1] * (1.0 / 128)),
                     rf((double)payload[2] * (1.0 / 64)), &point, 0);
    ++messages->sounds;
    for (i = 0; i < payload[3]; ++i)
    {
        random = rand();
        bh_contact_spark_create(messages->contacts, &point, rf(.7 - (double)random * (-1.0 / 131072)), 1);
        ++messages->sparks;
    }
    return done(messages);
}
static int damage(BHRemoteMessages *messages)
{
    BHVec3 point;
    unsigned short slot;
    unsigned char severity;
    BHRemoteQueryPacket packet;
    BHRemotePackets *packets = messages->packets;
    BHEntity *attacker = 0;
    if (!position(messages, &point) || !take(messages, &slot, 2) || !take(messages, &severity, 1))
        return 0;
    if ((long)slot == *packets->local_slot)
        attacker = packets->local;
    else if (slot < 8)
        attacker = packets->remote[slot];
    else
    {
        --packets->pending;
        ++packets->bad;
    }
    if (!packets->local || !packets->local->query)
    {
        messages->error = 1;
        return 0;
    }
    packet.damage.event = 0;
    packet.damage.severity = rf((double)severity * (1.0 / 128));
    packet.damage.attacker = attacker;
    packets->local->query(packets->local, &packet);
    return done(messages);
}
static int clock(BHRemoteMessages *messages, int host)
{
    BHRemotePackets *packets = messages->packets;
    unsigned short sender, value;
    unsigned char slot, peers;
    memcpy(&sender, packets->reader.data + 2, 2);
    if (host)
    {
        if (!messages->latency)
        {
            messages->error = 1;
            return 0;
        }
        if (!take(messages, &slot, 1))
            return 0;
        if (slot < 8)
        {
            messages->latency[slot] = rf((double)delta((unsigned short)*packets->simulation_tick, sender) * .1 -
                                         (double)messages->latency[slot] * (-.8));
            return done(messages);
        }
        return 1;
    }
    if (!messages->peers || *packets->local_slot < 0 || *packets->local_slot > 7)
    {
        messages->error = 1;
        return 0;
    }
    if (packets->reader.size - packets->reader.cursor < 17)
    {
        messages->error = 3;
        return 0;
    }
    memcpy(&value, packets->reader.data + 8 + *packets->local_slot * 2, 2);
    peers = packets->reader.data[24];
    packets->reader.cursor = 25;
    *messages->peers = peers > 3 ? 3 : peers;
    *packets->simulation_tick = (*packets->simulation_tick & 0xffff0000UL) | value;
    slot = (unsigned char)*packets->local_slot;
    if (!send_packet(messages, 5, value, &slot, 1, 1, 0))
        return 0;
    return done(messages);
}
static int scores(BHRemoteMessages *messages, int host)
{
    BHRemotePackets *packets = messages->packets;
    unsigned long i, bits;
    unsigned short values[8], value;
    signed char ids[2];
    BHEntity *entity;
    if (!messages->scores || !messages->scores->scores)
    {
        messages->error = 1;
        return 0;
    }
    if (host)
    {
        if (!take(messages, ids, 2))
            return 0;
        done(messages);
        for (i = 0; i < 2; ++i)
            if (ids[i] >= 0 && ids[i] < 8)
            {
                if ((long)ids[i] == *packets->local_slot)
                {
                    bits = (unsigned long)messages->scores->scores->local_score + (i ? 1UL : 0xffffffffUL);
                    memcpy(&messages->scores->scores->local_score, &bits, 4);
                }
                else if ((entity = packets->remote[(unsigned char)ids[i]]) != 0)
                {
                    value = (unsigned short)half(entity, 0x11c);
                    sethalf(entity, 0x11c, (unsigned short)(value + (i ? 1 : -1)));
                }
            }
        return score(messages);
    }
    if (!take(messages, values, 16))
        return 0;
    for (i = 0; i < 8; ++i)
    {
        if ((long)i == *packets->local_slot)
            messages->scores->scores->local_score = (long)delta(values[i], 0);
        else if (packets->remote[i])
            sethalf(packets->remote[i], 0x11c, values[i]);
    }
    done(messages);
    return hud(messages);
}
static int message(BHRemoteMessages *messages, void *p, unsigned long n, unsigned long capacity, int host)
{
    unsigned char type;
    unsigned short sender;
    short age;
    int result;
    if (!messages || !messages->packets || !p || !n || !messages->packets->remote ||
        !messages->packets->simulation_tick || !messages->packets->local_slot || !bh_math_environment_supported())
    {
        if (messages)
            messages->error = 1;
        return 0;
    }
    messages->error = 0;
    type = *(unsigned char *)p;
    switch (type)
    {
    case 3:
        if (!host)
        {
            result = bh_remote_spawn_packet(messages->packets, p, n, capacity);
            hud(messages);
            return result && !messages->error;
        }
        return 1;
    case 4:
        return bh_remote_position_packet(messages->packets, p, n, capacity);
    case 8:
        return bh_remote_shot_packet(messages->packets, p, n, capacity);
    case 9:
        return helper(messages, p, n, capacity, 0);
    case 10:
        if (!host)
            return helper(messages, p, n, capacity, 1);
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
    if (!open(messages, p, n, capacity))
        return 0;
    memcpy(&sender, messages->packets->reader.data + 2, 2);
    age = delta((unsigned short)*messages->packets->simulation_tick, sender);
    switch (type)
    {
    case 5:
        return clock(messages, host);
    case 6:
        if (age < 16)
            return sound(messages);
        return 1;
    case 7:
        if (age < 31)
            return damage(messages);
        return 1;
    case 11:
    case 12:
        return scores(messages, host);
    }
    return 1;
}
int bh_remote_host_message(BHRemoteMessages *messages, void *p, unsigned long n, unsigned long capacity)
{
    return message(messages, p, n, capacity, 1);
}
int bh_remote_client_message(BHRemoteMessages *messages, void *p, unsigned long n, unsigned long capacity)
{
    return message(messages, p, n, capacity, 0);
}
