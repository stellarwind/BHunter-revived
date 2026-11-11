#include "../bh_server_session.h"
#include "../bh_server_traffic.h"
#include "../../bh_vfile.h"
#include <enet/enet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static BHServerSession server;
static BHServerTraffic traffic;
static BHNetworkGame clients[BH_NETWORK_PLAYERS + 1];
static unsigned long departures, world_packets;
static BHLevelIdentity level = {"stadium", 0};

static void check(int condition, const char *description)
{
    if (!condition)
    {
        fprintf(stderr, "%s\n", description);
        exit(1);
    }
}

static void world_message(void *bytes, unsigned long size, void *user)
{
    (void)size;
    (void)user;
    if (*(unsigned char *)bytes == 4)
        ++world_packets;
}

static void client_message(void *bytes, unsigned long size, void *user)
{
    if (*(unsigned char *)bytes == 10)
    {
        const BHNetworkGame *client = user;
        check(client && client->received[5] && size == 25, "traffic follows the initial clock");
    }
}

static void client_roster(unsigned int slot, void *user)
{
    (void)user;
    if (slot == 0)
        ++departures;
}

static void pump_clients(void)
{
    for (unsigned int i = 0; i <= BH_NETWORK_PLAYERS; ++i)
    {
        BHNetworkGame *client = clients + i;
        bh_network_receive(client, client_message, client_roster, client);
        if (client->level.name[0] && !client->level_ready)
        {
            check(!strcmp(client->level.name, level.name), "host-selected level name");
            check(!bh_network_level_ready(client, 0), "client detects mismatched fingerprint");
            check(bh_network_level_ready(client, level.fingerprint), "client confirms matching fingerprint");
        }
    }
}

static void wait_for_io(BHNetworkGame *host)
{
    enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE;
    enet_socket_wait(host->host->socket, &condition, 1);
}

static void pump(void)
{
    check(bh_server_session_poll(&server), "server session is healthy");
    pump_clients();
    wait_for_io(&server.game);
}

static void wait_for(const unsigned long *counter, unsigned long expected, const char *description)
{
    enet_uint32 start = enet_time_get();
    while (*counter < expected && enet_time_get() - start < 3000)
        pump();
    check(*counter == expected, description);
}

static void send_packet(BHNetworkGame *game, unsigned char type, unsigned long to,
                        const unsigned char *payload, unsigned long size)
{
    BHNetworkTransport transport = {game};
    BHPacketWriter writer;
    unsigned char packet[64];
    check(bh_packet_begin(&writer, packet, sizeof(packet), type, 1) &&
          bh_packet_append(&writer, payload, size) &&
          bh_network_packet_send(&transport, &writer, to, 1) == 0, "enqueue test packet");
}

static unsigned short test_dedicated(void)
{
    unsigned short port;
    unsigned char pose[BH_NETWORK_POSE_BYTES - 8] = {0};
    unsigned char report[] = {0, 1};
    unsigned char clock_reply[] = {0};
    unsigned char damage[9] = {0};
    unsigned long score_packets, damage_packets;

    for (port = 27960; port < 27976; ++port)
        if (bh_server_session_open(&server, port, &level, world_message, bh_server_traffic_roster, &traffic))
            break;
    check(port < 27976, "open dedicated session");
    check(!server.game.local_player && !server.game.players[0].active, "dedicated host owns no player");
    for (unsigned int i = 0; i < BH_NETWORK_PLAYERS; ++i)
    {
        check(bh_network_open(clients + i, "127.0.0.1", port, "loopback", 0), "open client");
        wait_for(&clients[i].local_player, i + 1, "assign player ID including one and eight");
        check(clients[i].dedicated, "client negotiated dedicated server");
    }
    for (unsigned int i = 0; i < BH_NETWORK_PLAYERS; ++i)
    {
        wait_for(&clients[i].received[3], BH_NETWORK_PLAYERS, "complete eight-player roster");
        wait_for(&clients[i].received[12], BH_NETWORK_PLAYERS - i, "score snapshots follow the roster");
        wait_for(&clients[i].received[5], 1, "initial server clock reaches each client");
        wait_for(&clients[i].received[10], 10, "Stadium launches ten traffic actors for each client");
    }

    check(bh_network_open(clients + BH_NETWORK_PLAYERS, "127.0.0.1", port, "overflow", 0), "open ninth client");
    for (unsigned int i = 0; i < 200; ++i)
        pump();
    check(!clients[BH_NETWORK_PLAYERS].local_player, "ninth client remains unassigned");
    bh_network_close(clients + BH_NETWORK_PLAYERS);

    send_packet(&clients[0], 4, 0, pose, sizeof(pose));
    wait_for(&clients[7].received[4], 1, "player one's pose reaches player eight");
    check(world_packets == 1, "world receives player one's pose");
    send_packet(&clients[1], 7, 1, damage, sizeof(damage));
    wait_for(&clients[0].received[7], 1, "directed damage reaches player one");
    check(!clients[7].received[7], "directed damage stays with its recipient");
    damage_packets = clients[0].received[7];
    send_packet(&server.game, 7, 1, damage, sizeof(damage));
    wait_for(&clients[0].received[7], damage_packets + 1, "dedicated world can address player one");

    score_packets = clients[0].received[12];
    send_packet(&clients[0], 11, BH_NETWORK_SERVER, report, sizeof(report));
    wait_for(&clients[0].received[12], score_packets + 1, "server processes player one's score report");
    check(server.scores[0] == UINT16_MAX && server.scores[1] == 1, "scores use the real player slots");
    check(!clients[7].received[11], "score report stays with the server");
    for (unsigned int i = 0; i < 64; ++i)
        check(bh_server_session_tick(&server), "advance session clock");
    wait_for(&clients[0].received[5], 2, "periodic dedicated clock reaches player one");
    send_packet(&clients[0], 5, BH_NETWORK_SERVER, clock_reply, sizeof(clock_reply));
    wait_for(&server.game.received[5], 1, "player one's clock reply reaches server");

    bh_network_close(&clients[0]);
    wait_for(&departures, BH_NETWORK_PLAYERS - 1, "all remaining clients see player one leave");
    check(bh_network_open(&clients[0], "127.0.0.1", port, "replacement", 0), "open replacement client");
    wait_for(&clients[0].local_player, 1, "vacated first slot can be reused");
    wait_for(&clients[0].received[3], BH_NETWORK_PLAYERS, "replacement gets complete roster");
    wait_for(&clients[0].received[10], 10, "replacement gets Stadium traffic");
    check(!server.scores[0], "slot reuse resets the score");

    for (unsigned int i = 0; i < BH_NETWORK_PLAYERS; ++i)
        bh_network_close(clients + i);
    bh_server_session_close(&server);
    return port;
}

static void listen_roster(unsigned int slot, void *user)
{
    BHNetworkGame *host = user;
    unsigned char roster[BH_NETWORK_ROSTER_BYTES];
    if (!host->players[slot].active)
        return;
    bh_network_roster_payload(host->players + slot, roster);
    send_packet(host, 3, 0, roster, sizeof(roster));
    bh_network_roster_payload(host->players, roster);
    send_packet(host, 3, slot + 1, roster, sizeof(roster));
}

static void test_listen(unsigned short port)
{
    BHNetworkGame host = {0};
    unsigned char reply[] = {1};
    enet_uint32 start;
    check(bh_network_open(&host, NULL, port, "listen", 0), "open listen host");
    check(host.local_player == 1 && host.host->peerCount == BH_NETWORK_PLAYERS - 1,
          "listen host retains its slot and seven remote seats");
    check(bh_network_open(clients, "127.0.0.1", port, "loopback", 0), "join listen host");
    start = enet_time_get();
    while (clients[0].received[3] < 2 && enet_time_get() - start < 3000)
    {
        bh_network_receive(&host, client_message, listen_roster, &host);
        pump_clients();
        wait_for_io(&host);
    }
    check(clients[0].local_player == 2 && clients[0].received[3] == 2 && !clients[0].dedicated,
          "listen welcome and roster remain compatible");
    send_packet(&clients[0], 5, BH_NETWORK_SERVER, reply, sizeof(reply));
    start = enet_time_get();
    while (!host.received[5] && enet_time_get() - start < 3000)
    {
        pump_clients();
        bh_network_receive(&host, client_message, listen_roster, &host);
        wait_for_io(&host);
    }
    check(host.received[5] == 1, "server address translates to player one for a listen host");
    bh_network_close(clients);
    bh_network_close(&host);
}

int main(int argc, char **argv)
{
    check(argc == 2 && bh_vfile_mount_level(argv[1], &level.fingerprint), "mount the Stadium package");
    check(bh_server_traffic_load(&traffic, &server, &level), "load Stadium traffic");
    test_listen(test_dedicated());
    bh_server_traffic_close(&traffic);
    bh_vfile_shutdown();
    puts("Dedicated networking passed: eight players, level handshake, Stadium traffic, routing, scoring, clock, and slot reuse.");
    puts("Listen host regression passed.");
    return 0;
}
