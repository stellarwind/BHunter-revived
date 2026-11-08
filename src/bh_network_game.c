#include "bh_network_game.h"
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
    HELLO_SIZE = 35,
    HANDSHAKE_TIMEOUT_MS = 8000,
    /* Keep recent reports for half the wrap period of the 16-bit, 30 Hz game clock. */
    SCORE_REPLAY_WINDOW_MS = (1 << 15) * 1000 / 30
};

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

static int send_to(BHNetworkGame *game, const unsigned char *wire, size_t size, unsigned char channel)
{
    unsigned int i;
    int result = 0;
    if (!game->hosting)
        return game->server ? enqueue(game->server, channel, wire, size) : -1;
    for (i = 1; i < BH_NETWORK_PLAYERS; ++i)
    {
        BHNetworkPlayer *player = game->players + i;
        if (player->active && player->id != wire[0] && (!wire[1] || player->id == wire[1]))
            if (enqueue(player->peer, channel, wire, size) < 0)
                result = -1;
    }
    return result;
}

int bh_network_send(BHNetworkTransport *transport, unsigned long recipient, const void *bytes, unsigned long size,
                    int reliable)
{
    BHNetworkGame *game = transport ? transport->game : NULL;
    unsigned char wire[2 + BH_NETWORK_PACKET_CAPACITY], type;
    if (!game || !game->host || !game->local_player)
        return 0;
    if (!bytes || size < 8 || size >= BH_NETWORK_PACKET_CAPACITY || recipient > BH_NETWORK_PLAYERS)
        return -1;
    type = *(const unsigned char *)bytes;
    if (type < 3 || type > BH_NETWORK_DEATH_EFFECT)
        return -1;
    wire[0] = (unsigned char)game->local_player;
    wire[1] = (unsigned char)recipient;
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

int bh_network_open(BHNetworkGame *game, const char *address, unsigned short port, const char *name, unsigned int car)
{
    ENetAddress endpoint = {ENET_HOST_ANY, port};
    if (!port || !name || car >= BH_NETWORK_CARS || enet_initialize() != 0)
        return 0;
    memset(game, 0, sizeof(*game));
    game->initialized = 1;
    game->local_car = (unsigned char)car;
    game->hosting = address == NULL;
    game->host = enet_host_create(address ? NULL : &endpoint, address ? 1 : BH_NETWORK_PLAYERS - 1, 2, 0, 0);
    if (!game->host)
        goto failed;
    game->host->maximumPacketSize = BH_NETWORK_PACKET_CAPACITY + 2;
    game->host->maximumWaitingData = 64 * 1024;
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
    else
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
    for (slot = 1; slot < BH_NETWORK_PLAYERS; ++slot)
        if (game->players[slot].peer == peer)
            return slot;
    return BH_NETWORK_PLAYERS;
}

static void control(BHNetworkGame *game, ENetPeer *peer, const unsigned char *data, size_t size, BHNetworkRoster roster,
                    void *user)
{
    unsigned int slot;
    if (game->hosting && size == HELLO_SIZE && data[1] == HELLO && data[2] < BH_NETWORK_CARS &&
        peer_slot(game, peer) == BH_NETWORK_PLAYERS)
    {
        unsigned char welcome[] = {0, WELCOME, 0};
        for (slot = 1; slot < BH_NETWORK_PLAYERS && game->players[slot].active; ++slot)
            ;
        if (slot == BH_NETWORK_PLAYERS)
            return;
        welcome[2] = (unsigned char)(slot + 1);
        if (enqueue(peer, CONTROL, welcome, sizeof(welcome)) < 0)
            return;
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
        game->players[slot].active = 1;
        roster(slot, user);
    }
    else if (!game->hosting && size == 3 && data[2] > 1 && data[2] <= BH_NETWORK_PLAYERS)
    {
        if (data[1] == WELCOME && !game->local_player)
            game->local_player = data[2];
        else if (data[1] == LEAVE && game->players[data[2] - 1].active && data[2] != game->local_player)
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
    if (size < 10 || size > 265 || wire[3] != size - 10 || wire[1] > BH_NETWORK_PLAYERS)
        return 0;
    memcpy(bytes, wire + 2, size - 2);
    return bh_packet_receive(&reader, bytes, (unsigned long)size - 2, BH_NETWORK_PACKET_CAPACITY, &stats) == 1;
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

static void gameplay(BHNetworkGame *game, const ENetEvent *event, BHNetworkMessage receive, void *user)
{
    const unsigned char *wire = event->packet->data;
    size_t size = event->packet->dataLength;
    unsigned char bytes[BH_NETWORK_PACKET_CAPACITY], type, from, to;
    if (!game->local_player || !packet_valid(wire, size, bytes))
        goto reject;
    from = wire[0];
    to = wire[1];
    type = bytes[0];
    if (type < 3 || type > BH_NETWORK_DEATH_EFFECT || !from || from > BH_NETWORK_PLAYERS)
        goto reject;
    if (game->hosting)
    {
        if (peer_slot(game, event->peer) + 1 != from || type == 3 || type == 10 || type == 12 ||
            ((type == 5 || type == 11) && to != 1))
            goto reject;
    }
    else if (to && to != game->local_player)
        goto reject;
    if (type == 4 && (size != 24 || to || bytes[8] + 1 != from || bytes[9]))
        goto reject;
    if (type == 5 && (game->hosting ? size != 11 || bytes[8] + 1 != from : size != 27 || from != 1))
        goto reject;
    if (type == 11 && (size != 12 || bytes[8] + 1 != from || (bytes[9] > 7 && bytes[9] != 255)))
        goto reject;
    if (type == 12 && (size != 26 || from != 1))
        goto reject;
    if (type == 8 && (size != 15 || to || bytes[8] + 1 != from || bytes[11] > 8))
        goto reject;
    if (type == BH_NETWORK_DEATH_EFFECT &&
        (size != 19 || to || event->channelID != CONTROL || bytes[16] > BH_NETWORK_DEATH_FINAL))
        goto reject;
    if (type == 3)
    {
        unsigned int slot, car;
        if (from != 1 || event->channelID != CONTROL || size != 10 + BH_NETWORK_ROSTER_BYTES)
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
    else if (!game->players[from - 1].active)
        goto reject;
    if (game->hosting && type == 11 && !new_score_report(game->players + from - 1, bytes))
        goto reject;
    if (game->hosting && to != 1)
        send_to(game, wire, size, event->channelID);
    if (!to || to == game->local_player)
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
            if (!game->hosting)
            {
                unsigned char hello[HELLO_SIZE] = {0, HELLO, game->local_car};
                memcpy(hello + 3, game->players[0].name, 32);
                if (enqueue(event.peer, CONTROL, hello, sizeof(hello)) < 0)
                    game->disconnected = 1;
            }
            else if (event.data != PROTOCOL)
                enet_peer_disconnect_now(event.peer, 0);
            else
                enet_peer_timeout(event.peer, 8, 2500, 8000);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            if (event.packet->dataLength && !event.packet->data[0])
            {
                if (event.channelID == CONTROL)
                    control(game, event.peer, event.packet->data, event.packet->dataLength, roster, user);
            }
            else
                gameplay(game, &event, receive, user);
            enet_packet_destroy(event.packet);
            break;
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
