#include "bh_server_session.h"
#include <string.h>

static void send_packet(BHServerSession *session, unsigned char type, const unsigned char *payload,
                        unsigned long size, unsigned long recipient, int reliable)
{
    BHNetworkTransport transport = {&session->game};
    BHPacketWriter writer;
    unsigned char packet[64];
    if (!bh_packet_begin(&writer, packet, sizeof(packet), type, session->tick) ||
        !bh_packet_append(&writer, payload, size) ||
        bh_network_packet_send(&transport, &writer, recipient, reliable) < 0)
        session->failed = 1;
}

static void put_word(unsigned char *bytes, uint16_t value)
{
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
}

static void send_scores(BHServerSession *session)
{
    unsigned char payload[BH_NETWORK_PLAYERS * 2];
    for (unsigned int i = 0; i < BH_NETWORK_PLAYERS; ++i)
        put_word(payload + 2 * i, session->scores[i]);
    send_packet(session, 12, payload, sizeof(payload), 0, 1);
}

static void roster(unsigned int slot, void *user)
{
    BHServerSession *session = user;
    const BHNetworkPlayer *player = &session->game.players[slot];
    unsigned char payload[BH_NETWORK_ROSTER_BYTES];
    session->latency[slot] = 0;
    session->scores[slot] = 0;
    if (player->active)
    {
        bh_network_roster_payload(player, payload);
        send_packet(session, 3, payload, sizeof(payload), 0, 1);
        for (unsigned int i = 0; i < BH_NETWORK_PLAYERS; ++i)
            if (i != slot && session->game.players[i].active)
            {
                bh_network_roster_payload(&session->game.players[i], payload);
                send_packet(session, 3, payload, sizeof(payload), player->id, 1);
            }
        send_scores(session);
    }
    if (session->world_roster)
        session->world_roster(slot, session->world);
}

static void message(void *packet, unsigned long size, void *user)
{
    BHServerSession *session = user;
    const unsigned char *bytes = packet;
    if (bytes[0] == 5)
    {
        unsigned int slot = bytes[8];
        uint16_t sender = bytes[2] | ((uint16_t)bytes[3] << 8);
        int16_t age = (int16_t)(uint16_t)(session->tick - sender);
        session->latency[slot] = (float)(age * .1 + session->latency[slot] * .8);
    }
    else if (bytes[0] == 11)
    {
        unsigned int slot = bytes[8];
        unsigned int attacker = bytes[9];
        --session->scores[slot];
        if (attacker < BH_NETWORK_PLAYERS && session->game.players[attacker].active)
            ++session->scores[attacker];
        send_scores(session);
    }
    else if (session->world_message)
        session->world_message(packet, size, session->world);
}

int bh_server_session_open(BHServerSession *session, unsigned short port, const BHLevelIdentity *level,
                           BHNetworkMessage world_message, BHNetworkRoster world_roster, void *world)
{
    memset(session, 0, sizeof(*session));
    if (!bh_network_open_dedicated(&session->game, port, level))
        return 0;
    session->world_message = world_message;
    session->world_roster = world_roster;
    session->world = world;
    return 1;
}

int bh_server_session_poll(BHServerSession *session)
{
    bh_network_receive(&session->game, message, roster, session);
    return session->game.host && !session->failed && !session->game.disconnected;
}

int bh_server_session_tick(BHServerSession *session)
{
    unsigned char payload[BH_NETWORK_PLAYERS * 2 + 1];
    double sum = 0;
    unsigned int players = 0;
    if (++session->tick & 63)
        return !session->failed;
    for (unsigned int i = 0; i < BH_NETWORK_PLAYERS; ++i)
    {
        int tick = (int)((int16_t)session->tick + (double)session->latency[i]);
        put_word(payload + 2 * i, (uint16_t)tick);
        if (session->game.players[i].active)
        {
            sum += session->latency[i];
            ++players;
        }
    }
    if (players)
    {
        double delay = sum * 1.4 / players;
        payload[BH_NETWORK_PLAYERS * 2] = delay < 0 ? 0 : delay > 3 ? 3 : (unsigned char)delay;
        send_packet(session, 5, payload, sizeof(payload), 0, 0);
    }
    return !session->failed;
}

void bh_server_session_close(BHServerSession *session)
{
    bh_network_close(&session->game);
}
