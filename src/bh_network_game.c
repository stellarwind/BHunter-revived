#include "bh_network_game.h"
#include "bh_player_propulsion.h"
#include <enet/enet.h>
#include <enet/time.h>
#include <string.h>

enum
{
    PROTOCOL = 0x42480002,
    CONTROL = 0,
    SNAPSHOTS = 1,
    HELLO = 1,
    WELCOME = 2,
    LEAVE = 3,
    LEVEL_OFFER = 4,
    LEVEL_READY = 5,
    LEVEL_SUPPORTED = 6,
    DEDICATED_SUPPORTED = 7,
    HELLO_SIZE = 35,
    POSE_PROPULSION = 1,
    DEDICATED_SERVER = 2,
    LEVEL_CAN_LOAD = 1,
    LEVEL_WAS_OFFERED = 2,
    LEVEL_HASH_SIZE = 8,
    LEVEL_READY_SIZE = HELLO_SIZE + LEVEL_HASH_SIZE,
    POSE_WIRE_SIZE = 2 + BH_NETWORK_POSE_BYTES,
    PROPULSION_POSE_WIRE_SIZE = POSE_WIRE_SIZE + BH_PROPULSION_CHANNELS,
    HANDSHAKE_TIMEOUT_MS = 8000,
    LEVEL_TIMEOUT_MS = 60000,
    /* Keep recent reports for half the wrap period of the 16-bit, 30 Hz game clock. */
    SCORE_REPLAY_WINDOW_MS = (1 << 15) * 1000 / 30
};

int bh_level_name_valid(const char *name)
{
    size_t i;
    if (!name)
        return 0;
    for (i = 0; i < BH_LEVEL_NAME_CAPACITY && name[i]; ++i)
    {
        unsigned char c = name[i];
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '_' && c != '-')
            return 0;
    }
    if (!i || i == BH_LEVEL_NAME_CAPACITY)
        return 0;
    /* These basenames are device names on Windows even with an extension. */
    if (i == 3 && (!_stricmp(name, "CON") || !_stricmp(name, "PRN") ||
                   !_stricmp(name, "AUX") || !_stricmp(name, "NUL")))
        return 0;
    return i != 4 || name[3] < '0' || name[3] > '9' ||
           (_strnicmp(name, "COM", 3) && _strnicmp(name, "LPT", 3));
}

static void write_hash(unsigned char *bytes, uint64_t fingerprint)
{
    for (unsigned int i = 0; i < LEVEL_HASH_SIZE; ++i)
        bytes[i] = (unsigned char)(fingerprint >> (8 * i));
}

static uint64_t read_hash(const unsigned char *bytes)
{
    uint64_t fingerprint = 0;
    for (unsigned int i = 0; i < LEVEL_HASH_SIZE; ++i)
        fingerprint |= (uint64_t)bytes[i] << (8 * i);
    return fingerprint;
}

static int enqueue(ENetPeer *peer, unsigned char channel, const void *data, size_t size)
{
    ENetPacket *packet = enet_packet_create(data, size, channel == CONTROL ? ENET_PACKET_FLAG_RELIABLE : 0);
    if (!packet)
        return -1;
    if (enet_peer_send(peer, channel, packet) < 0)
    {
        enet_packet_destroy(packet);
        return -1;
    }
    return 0;
}

static int enqueue_gameplay(ENetPeer *peer, unsigned char channel, const unsigned char *wire, size_t size,
                            int pose_propulsion)
{
    if (wire[2] == 4 && size == PROPULSION_POSE_WIRE_SIZE && !pose_propulsion)
    {
        unsigned char legacy[POSE_WIRE_SIZE + 2]; /* Allow for checksum padding. */
        BHPacketWriter writer = {.data = legacy + 2, .capacity = sizeof(legacy) - 2, .cursor = BH_NETWORK_POSE_BYTES};
        unsigned long packet_size;
        memcpy(legacy, wire, POSE_WIRE_SIZE);
        if (!bh_packet_finish(&writer, &packet_size))
            return -1;
        return enqueue(peer, channel, legacy, packet_size + 2);
    }
    return enqueue(peer, channel, wire, size);
}

static int send_to(BHNetworkGame *game, const unsigned char *wire, size_t size, unsigned char channel)
{
    unsigned int i;
    int result = 0;
    if (!game->hosting)
        return game->server ? enqueue_gameplay(game->server, channel, wire, size, game->server_pose_propulsion) : -1;
    for (i = game->dedicated ? 0 : 1; i < BH_NETWORK_PLAYERS; ++i)
    {
        BHNetworkPlayer *player = game->players + i;
        if (player->active && player->peer && player->peer->state == ENET_PEER_STATE_CONNECTED &&
            player->id != wire[0] && (!wire[1] || player->id == wire[1]))
            if (enqueue_gameplay(player->peer, channel, wire, size, player->pose_propulsion) < 0)
                result = -1;
    }
    return result;
}

static unsigned char server_id(const BHNetworkGame *game)
{
    return game->dedicated ? BH_NETWORK_SERVER : 1;
}

int bh_network_send(BHNetworkTransport *transport, unsigned long recipient, const void *bytes, unsigned long size,
                    int reliable)
{
    BHNetworkGame *game = transport ? transport->game : NULL;
    unsigned char wire[2 + BH_NETWORK_PACKET_CAPACITY], type;
    if (!game || !game->host || (!game->hosting && !game->local_player))
        return 0;
    if (!bytes || size < 8 || size >= BH_NETWORK_PACKET_CAPACITY ||
        (recipient > BH_NETWORK_PLAYERS && (recipient != BH_NETWORK_SERVER || game->hosting)))
        return -1;
    type = *(const unsigned char *)bytes;
    if (type < 3 || type > BH_NETWORK_DEATH_EFFECT)
        return -1;
    wire[0] = game->hosting ? server_id(game) : (unsigned char)game->local_player;
    wire[1] = recipient == BH_NETWORK_SERVER ? server_id(game) : (unsigned char)recipient;
    memcpy(wire + 2, bytes, size);
    /* Damage, scoring and death effects are one-shot events. Keep them reliable. */
    if (type == 7 || type == 11 || type == 12 || type == BH_NETWORK_DEATH_EFFECT)
        reliable = 1;
    ++game->sent[type];
    return send_to(game, wire, size + 2, reliable ? CONTROL : SNAPSHOTS);
}

int bh_network_packet_send(BHNetworkTransport *transport, BHPacketWriter *writer, unsigned long recipient, int reliable)
{
    unsigned long size;
    return bh_packet_finish(writer, &size) ? bh_network_send(transport, recipient, writer->data, size, reliable) : -1;
}

static int open_game(BHNetworkGame *game, const char *address, unsigned short port, const char *name,
                     unsigned int car, int dedicated)
{
    ENetAddress endpoint = {ENET_HOST_ANY, port};
    if (!port || (!dedicated && (!name || car >= BH_NETWORK_CARS)) || enet_initialize() != 0)
        return 0;
    memset(game, 0, sizeof(*game));
    game->initialized = 1;
    game->local_car = (unsigned char)car;
    game->hosting = address == NULL;
    game->dedicated = dedicated;
    game->host = enet_host_create(address ? NULL : &endpoint,
                                  address ? 1 : BH_NETWORK_PLAYERS - !dedicated, 2, 0, 0);
    if (!game->host)
        goto failed;
    game->host->maximumPacketSize = BH_NETWORK_PACKET_CAPACITY + 2;
    game->host->maximumWaitingData = 64 * 1024;
    if (!dedicated)
        strncpy(game->players[0].name, name, sizeof(game->players[0].name) - 1);
    if (address)
    {
        if (enet_address_set_host_ip(&endpoint, address) < 0)
            goto failed;
        game->server = enet_host_connect(game->host, &endpoint, 2, PROTOCOL);
        if (!game->server)
            goto failed;
        enet_peer_timeout(game->server, 8, 2500, 8000);
    }
    else if (!dedicated)
    {
        game->local_player = game->players[0].id = 1;
        game->players[0].car = game->local_car;
        game->players[0].active = 1;
    }
    return 1;
failed:
    bh_network_close(game);
    return 0;
}

int bh_network_open(BHNetworkGame *game, const char *address, unsigned short port, const char *name, unsigned int car)
{
    return open_game(game, address, port, name, car, 0);
}

int bh_network_open_dedicated(BHNetworkGame *game, unsigned short port, const BHLevelIdentity *level)
{
    if (level && level->name[0] && !bh_level_name_valid(level->name))
        return 0;
    if (!open_game(game, NULL, port, NULL, 0, 1))
        return 0;
    if (level)
        game->level = *level;
    return 1;
}

void bh_network_close(BHNetworkGame *game)
{
    unsigned int i;
    if (game->host)
    {
        for (i = 0; i < game->host->peerCount; ++i)
            if (game->host->peers[i].state == ENET_PEER_STATE_CONNECTED)
                enet_peer_disconnect_now(game->host->peers + i, 0);
        enet_host_destroy(game->host);
    }
    if (game->initialized)
        enet_deinitialize();
    memset(game, 0, sizeof(*game));
}

static unsigned int peer_slot(const BHNetworkGame *game, const ENetPeer *peer)
{
    unsigned int slot;
    for (slot = game->dedicated ? 0 : 1; slot < BH_NETWORK_PLAYERS; ++slot)
        if (game->players[slot].peer == peer)
            return slot;
    return BH_NETWORK_PLAYERS;
}

static void hello_data(const BHNetworkGame *game, unsigned char *data, unsigned char type)
{
    data[0] = 0;
    data[1] = type;
    data[2] = game->local_car;
    memcpy(data + 3, game->players[0].name, 31);
    data[HELLO_SIZE - 1] = POSE_PROPULSION;
}

int bh_network_level_ready(BHNetworkGame *game, uint64_t fingerprint)
{
    unsigned char ready[LEVEL_READY_SIZE];
    if (!game->server || !game->level.name[0] || game->level_ready || game->disconnected ||
        fingerprint != game->level.fingerprint)
        return 0;
    hello_data(game, ready, LEVEL_READY);
    write_hash(ready + HELLO_SIZE, fingerprint);
    if (enqueue(game->server, CONTROL, ready, sizeof(ready)) < 0)
        return 0;
    game->level_ready = 1;
    return 1;
}

static void offer_level(BHNetworkGame *game, ENetPeer *peer)
{
    unsigned char offer[2 + LEVEL_HASH_SIZE + BH_LEVEL_NAME_CAPACITY] = {0, LEVEL_OFFER};
    size_t size = strlen(game->level.name) + 1;
    unsigned int index = peer->incomingPeerID;
    if (game->level_handshake[index] == LEVEL_WAS_OFFERED)
        return;
    write_hash(offer + 2, game->level.fingerprint);
    memcpy(offer + 2 + LEVEL_HASH_SIZE, game->level.name, size);
    if (enqueue(peer, CONTROL, offer, 2 + LEVEL_HASH_SIZE + size) < 0)
        return;
    game->level_handshake[index] = LEVEL_WAS_OFFERED;
    game->handshake_deadline[index] = enet_time_get() + LEVEL_TIMEOUT_MS;
    enet_peer_timeout(peer, 8, 15000, LEVEL_TIMEOUT_MS);
}

static void control(BHNetworkGame *game, ENetPeer *peer, const unsigned char *data, size_t size, BHNetworkRoster roster,
                    void *user)
{
    unsigned int slot;
    if (game->hosting && size == 2 && data[1] == DEDICATED_SUPPORTED)
        game->dedicated_supported[peer->incomingPeerID] = 1;
    else if (game->hosting && size == 2 && data[1] == LEVEL_SUPPORTED &&
        game->level_handshake[peer->incomingPeerID] == 0)
        game->level_handshake[peer->incomingPeerID] = LEVEL_CAN_LOAD;
    else if (game->hosting && peer_slot(game, peer) == BH_NETWORK_PLAYERS &&
        ((size == HELLO_SIZE && data[1] == HELLO) || (size == LEVEL_READY_SIZE && data[1] == LEVEL_READY)) &&
        data[2] < BH_NETWORK_CARS)
    {
        unsigned char welcome[] = {0, WELCOME, 0, 0};
        int pose_propulsion = data[HELLO_SIZE - 1] == POSE_PROPULSION;
        if (game->dedicated && !game->dedicated_supported[peer->incomingPeerID])
        {
            enet_peer_disconnect(peer, 0);
            return;
        }
        if (game->level.name[0])
        {
            if (data[1] == HELLO)
            {
                if (game->level_handshake[peer->incomingPeerID])
                    offer_level(game, peer);
                else
                    enet_peer_disconnect(peer, 0);
                return;
            }
            if (game->level_handshake[peer->incomingPeerID] != LEVEL_WAS_OFFERED ||
                read_hash(data + HELLO_SIZE) != game->level.fingerprint)
                return;
        }
        else if (data[1] != HELLO)
            return;
        for (slot = game->dedicated ? 0 : 1; slot < BH_NETWORK_PLAYERS && game->players[slot].active; ++slot)
            ;
        if (slot == BH_NETWORK_PLAYERS)
            return;
        welcome[2] = (unsigned char)(slot + 1);
        welcome[3] = (pose_propulsion ? POSE_PROPULSION : 0) | (game->dedicated ? DEDICATED_SERVER : 0);
        if (enqueue(peer, CONTROL, welcome, welcome[3] ? sizeof(welcome) : 3) < 0)
            return;
        enet_peer_timeout(peer, 8, 2500, HANDSHAKE_TIMEOUT_MS);
        memset(game->players + slot, 0, sizeof(game->players[slot]));
        memcpy(game->players[slot].name, data + 3, 31);
        game->players[slot].name[31] = 0;
        /* Control characters are interpreted by the in-game message renderer. */
        for (unsigned int i = 0; game->players[slot].name[i]; ++i)
        {
            unsigned char c = (unsigned char)game->players[slot].name[i];
            if (c < 32 || c > 126)
                game->players[slot].name[i] = '?';
        }
        game->players[slot].peer = peer;
        game->players[slot].id = slot + 1;
        game->players[slot].car = data[2];
        game->players[slot].pose_propulsion = (unsigned char)pose_propulsion;
        game->players[slot].active = 1;
        roster(slot, user);
    }
    else if (!game->hosting && size > 2 + LEVEL_HASH_SIZE && size <= 2 + LEVEL_HASH_SIZE + BH_LEVEL_NAME_CAPACITY &&
             data[1] == LEVEL_OFFER && !game->local_player && !game->level.name[0])
    {
        const char *name = (const char *)data + 2 + LEVEL_HASH_SIZE;
        if (!name[0] || memchr(name, 0, size - 2 - LEVEL_HASH_SIZE) != data + size - 1 ||
            !bh_level_name_valid(name))
        {
            game->disconnected = 1;
            return;
        }
        strcpy(game->level.name, name);
        game->level.fingerprint = read_hash(data + 2);
        game->handshake_deadline[0] = enet_time_get() + LEVEL_TIMEOUT_MS;
        enet_peer_timeout(peer, 8, 15000, LEVEL_TIMEOUT_MS);
    }
    else if (!game->hosting && (size == 3 || size == 4) && data[2] && data[2] <= BH_NETWORK_PLAYERS)
    {
        unsigned char features = size == 4 ? data[3] : 0;
        if (data[1] == WELCOME && !game->local_player && !(features & ~(POSE_PROPULSION | DEDICATED_SERVER)) &&
            (data[2] > 1 || (features & DEDICATED_SERVER)) &&
            (!game->level.name[0] || game->level_ready))
        {
            game->local_player = data[2];
            game->server_pose_propulsion = (features & POSE_PROPULSION) != 0;
            game->dedicated = (features & DEDICATED_SERVER) != 0;
            enet_peer_timeout(peer, 8, 2500, HANDSHAKE_TIMEOUT_MS);
        }
        else if (data[1] == LEAVE && size == 3 && (data[2] > 1 || game->dedicated) &&
                 game->players[data[2] - 1].active && data[2] != game->local_player)
        {
            game->players[data[2] - 1].active = 0;
            roster(data[2] - 1, user);
        }
    }
}

static int packet_valid(const unsigned char *wire, size_t size, unsigned char *bytes)
{
    BHPacketReader reader;
    BHPacketStats stats = {0, 0};
    if (size < 10 || size > 265 || wire[3] != size - 10 || wire[1] > BH_NETWORK_SERVER)
        return 0;
    memcpy(bytes, wire + 2, size - 2);
    return bh_packet_receive(&reader, bytes, (unsigned long)size - 2, BH_NETWORK_PACKET_CAPACITY, &stats) == 1;
}

void bh_network_roster_payload(const BHNetworkPlayer *player, unsigned char payload[BH_NETWORK_ROSTER_BYTES])
{
    unsigned int length = 0;
    memset(payload, 0, BH_NETWORK_ROSTER_BYTES);
    payload[0] = (unsigned char)player->id;
    payload[4] = (unsigned char)(player->id - 1);
    while (length < 19 && player->name[length])
    {
        payload[5 + length] = (unsigned char)player->name[length];
        ++length;
    }
    payload[BH_NETWORK_ROSTER_BYTES - 1] = player->car;
}

static int new_score_report(BHNetworkPlayer *player, const unsigned char *bytes)
{
    enet_uint32 now = enet_time_get();
    uint16_t frame = bytes[2] | ((uint16_t)bytes[3] << 8);
    for (unsigned int i = 0; i < player->score_history_count; ++i)
    {
        const BHNetworkScoreReport *report = player->score_history + i;
        if (report->frame == frame && ENET_TIME_DIFFERENCE(now, report->received_at) < SCORE_REPLAY_WINDOW_MS)
            return 0;
    }
    player->score_history[player->next_score_report] = (BHNetworkScoreReport){now, frame};
    player->next_score_report = (player->next_score_report + 1) % BH_NETWORK_SCORE_HISTORY;
    if (player->score_history_count < BH_NETWORK_SCORE_HISTORY)
        ++player->score_history_count;
    return 1;
}

int bh_network_shot_pattern_valid(unsigned int type, unsigned int count, unsigned int spacing)
{
    static const struct
    {
        unsigned char count, spacing;
    } patterns[] = {
        {4, 2}, /* Hitscan */
        {4, 2}, /* Ballistic */
        {2, 4}, /* Homing */
        {1, 0}, /* Orbit */
        {5, 1}, /* Area */
        {1, 0}, /* Arc */
        {1, 0}, /* Split */
        {1, 0}, /* Accelerated */
        {1, 0}  /* Pulse projectile */
    };
    return type < sizeof(patterns) / sizeof(patterns[0]) &&
           count == patterns[type].count && spacing == patterns[type].spacing;
}

static void gameplay(BHNetworkGame *game, const ENetEvent *event, BHNetworkMessage receive, void *user)
{
    const unsigned char *wire = event->packet->data;
    size_t size = event->packet->dataLength;
    unsigned char bytes[BH_NETWORK_PACKET_CAPACITY], type, from, to;
    unsigned char server = server_id(game);
    if ((!game->hosting && !game->local_player) || !packet_valid(wire, size, bytes))
        goto reject;
    from = wire[0];
    to = wire[1];
    type = bytes[0];
    if (type < 3 || type > BH_NETWORK_DEATH_EFFECT || !from || from > BH_NETWORK_SERVER ||
        (to > BH_NETWORK_PLAYERS && to != server))
        goto reject;
    if (game->hosting)
    {
        if (from > BH_NETWORK_PLAYERS || peer_slot(game, event->peer) + 1 != from ||
            type == 3 || type == 10 || type == 12 || ((type == 5 || type == 11) && to != server) ||
            (to == BH_NETWORK_SERVER && type != 5 && type != 11))
            goto reject;
    }
    else if (to && to != game->local_player)
        goto reject;
    if (from > BH_NETWORK_PLAYERS && (from != server || type == 4 || type == 8 || type == 11))
        goto reject;
    if (type == 4 &&
        ((size != POSE_WIRE_SIZE && size != PROPULSION_POSE_WIRE_SIZE) || to || bytes[8] + 1 != from || bytes[9]))
        goto reject;
    if (type == 5 && (game->hosting ? size != 11 || bytes[8] + 1 != from : size != 27 || from != server))
        goto reject;
    if (type == 11 && (size != 12 || bytes[8] + 1 != from || (bytes[9] > 7 && bytes[9] != 255)))
        goto reject;
    if ((type == 10 && from != server) || (type == 12 && (size != 26 || from != server)))
        goto reject;
    if (type == 8 &&
        (size != 15 || to || bytes[8] + 1 != from || !bh_network_shot_pattern_valid(bytes[11], bytes[9], bytes[10])))
        goto reject;
    if (type == BH_NETWORK_DEATH_EFFECT &&
        (size != 19 || to || event->channelID != CONTROL || bytes[16] > BH_NETWORK_DEATH_FINAL))
        goto reject;
    if (type == 3)
    {
        unsigned int slot, car;
        if (from != server || event->channelID != CONTROL || size != 10 + BH_NETWORK_ROSTER_BYTES)
            goto reject;
        slot = bytes[12];
        car = bytes[8 + BH_NETWORK_ROSTER_BYTES - 1];
        if (slot >= BH_NETWORK_PLAYERS || car >= BH_NETWORK_CARS || bytes[8] != slot + 1 || bytes[9] || bytes[10] ||
            bytes[11] || game->players[slot].active)
            goto reject;
        game->players[slot].active = 1;
        game->players[slot].id = slot + 1;
        game->players[slot].car = (unsigned char)car;
        memcpy(game->players[slot].name, bytes + 13, 19);
        game->players[slot].name[19] = 0;
    }
    else if (from <= BH_NETWORK_PLAYERS && !game->players[from - 1].active)
        goto reject;
    if (game->hosting && type == 11 && !new_score_report(game->players + from - 1, bytes))
        goto reject;
    if (game->hosting && to != server)
        send_to(game, wire, size, event->channelID);
    if (!to || to == (game->hosting ? server : game->local_player))
    {
        ++game->received[type];
        receive(bytes, (unsigned long)size - 2, user);
    }
    return;
reject:
    ++game->rejected;
}

static void expire_handshakes(BHNetworkGame *game)
{
    enet_uint32 now = enet_time_get();
    for (size_t i = 0; i < game->host->peerCount; ++i)
    {
        ENetPeer *peer = game->host->peers + i;
        if (peer->state != ENET_PEER_STATE_CONNECTED || ENET_TIME_LESS(now, game->handshake_deadline[i]))
            continue;
        if (game->hosting)
        {
            if (peer_slot(game, peer) == BH_NETWORK_PLAYERS)
                enet_peer_disconnect_now(peer, 0);
        }
        else if (!game->local_player)
        {
            game->welcome_timed_out = 1;
            game->disconnected = 1;
        }
    }
}

void bh_network_receive(BHNetworkGame *game, BHNetworkMessage receive, BHNetworkRoster roster, void *user)
{
    ENetEvent event;
    int result = 0;
    unsigned int budget = 128;
    if (!game->host || game->disconnected)
        return;
    while (budget-- && (result = enet_host_service(game->host, &event, 0)) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_CONNECT:
            game->handshake_deadline[event.peer->incomingPeerID] = enet_time_get() + HANDSHAKE_TIMEOUT_MS;
            game->level_handshake[event.peer->incomingPeerID] = 0;
            game->dedicated_supported[event.peer->incomingPeerID] = 0;
            if (!game->hosting)
            {
                static const unsigned char capability[] = {0, LEVEL_SUPPORTED};
                static const unsigned char dedicated[] = {0, DEDICATED_SUPPORTED};
                unsigned char hello[HELLO_SIZE];
                hello_data(game, hello, HELLO);
                if (enqueue(event.peer, CONTROL, capability, sizeof(capability)) < 0 ||
                    enqueue(event.peer, CONTROL, dedicated, sizeof(dedicated)) < 0 ||
                    enqueue(event.peer, CONTROL, hello, sizeof(hello)) < 0)
                    game->disconnected = 1;
            }
            else if (event.data != PROTOCOL)
                enet_peer_disconnect_now(event.peer, 0);
            else
                enet_peer_timeout(event.peer, 8, 2500, 8000);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
        {
            int welcomed = game->local_player != 0;
            if (event.packet->dataLength >= 2 && !event.packet->data[0])
            {
                if (event.channelID == CONTROL)
                    control(game, event.peer, event.packet->data, event.packet->dataLength, roster, user);
            }
            else
                gameplay(game, &event, receive, user);
            enet_packet_destroy(event.packet);
            /* Let the app select and bind the world before receiving the roster. */
            if (game->disconnected || (!game->hosting && !welcomed &&
                (game->local_player || (game->level.name[0] && !game->level_ready))))
            {
                enet_host_flush(game->host);
                return;
            }
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
            if (!game->hosting)
                game->disconnected = 1;
            else
            {
                unsigned int slot = peer_slot(game, event.peer);
                if (slot < BH_NETWORK_PLAYERS)
                {
                    unsigned char leave[] = {0, LEAVE, (unsigned char)(slot + 1)};
                    game->players[slot].active = 0;
                    game->players[slot].peer = NULL;
                    roster(slot, user);
                    ENetPacket *notice = enet_packet_create(leave, sizeof(leave), ENET_PACKET_FLAG_RELIABLE);
                    if (notice)
                        enet_host_broadcast(game->host, CONTROL, notice);
                }
            }
            break;
        default:
            break;
        }
    }
    if (result < 0)
        game->disconnected = 1;
    /* Process queued HELLO/WELCOME packets before checking their deadlines. */
    if (!game->disconnected)
        expire_handshakes(game);
}
