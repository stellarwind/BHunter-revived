#include "bh_server_traffic.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
    ARENA_START = 3333,
    ACTOR_LAUNCH = 10
};

struct BHServerLauncher
{
    unsigned short index, count, tag;
    unsigned char type, bounty, speed;
};

static void put_word(unsigned char *bytes, uint16_t value)
{
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
}

int bh_server_traffic_load(BHServerTraffic *traffic, BHServerSession *session, const BHLevelIdentity *identity,
                           const BHLevelRecords *level)
{
    memset(traffic, 0, sizeof(*traffic));
    traffic->session = session;
    traffic->seed = (uint32_t)identity->fingerprint ^ (uint32_t)time(NULL);
    if (!identity->name[0])
        return 1;
    traffic->launchers = calloc(level->placement_count, sizeof(*traffic->launchers));
    if (!traffic->launchers)
        return 0;
    for (uint32_t i = 0; i < level->override_count; ++i)
    {
        const BHLevelPlacementOverride *entry = level->overrides + i;
        const int16_t *values = entry->values;
        const BHLevelPlacement *placement;
        BHServerLauncher *launcher;
        if (entry->placement >= level->placement_count)
            continue;
        placement = level->placements + entry->placement;
        if (_strnicmp(placement->model, "LAUNCHER.BSP", sizeof(placement->model)) &&
            _strnicmp(placement->model, "LAUNCH2.BSP", sizeof(placement->model)))
            continue;
        launcher = traffic->launchers + entry->placement;
        launcher->count = 0;
        if (values[0] == ARENA_START && values[1] > 0 && (values[3] == 0 || values[3] == 3))
        {
            launcher->index = (unsigned short)entry->placement;
            launcher->count = (unsigned short)values[1];
            launcher->tag = (unsigned short)values[2];
            launcher->type = (unsigned char)values[3];
            launcher->bounty = (unsigned char)(values[4] / 5);
            launcher->speed = (unsigned char)values[5];
        }
    }
    for (uint32_t i = 0; i < level->placement_count; ++i)
        if (traffic->launchers[i].count)
            traffic->launchers[traffic->count++] = traffic->launchers[i];
    return 1;
}

void bh_server_traffic_roster(unsigned int slot, void *user)
{
    BHServerTraffic *traffic = user;
    BHServerSession *session = traffic->session;
    const BHNetworkPlayer *player = session->game.players + slot;
    BHNetworkTransport transport = {&session->game};
    uint32_t seed = traffic->seed;
    if (!player->active)
        return;
    for (unsigned int i = 0; i < traffic->count; ++i)
    {
        const BHServerLauncher *launcher = traffic->launchers + i;
        /* The client resolves the launcher placement to its owning room. */
        unsigned char payload[17] = {0, 0, 0, 128, 0, 128, 0, 128};
        put_word(payload, launcher->index);
        put_word(payload + 12, launcher->tag);
        payload[14] = launcher->type;
        payload[15] = launcher->bounty;
        payload[16] = launcher->speed;
        for (unsigned int j = 0; j < launcher->count; ++j)
        {
            unsigned char packet[32];
            BHPacketWriter writer;
            seed = seed * UINT32_C(1664525) + UINT32_C(1013904223);
            put_word(payload + 8, (uint16_t)seed);
            put_word(payload + 10, (uint16_t)(seed >> 16));
            if (!bh_packet_begin(&writer, packet, sizeof(packet), ACTOR_LAUNCH, session->tick) ||
                !bh_packet_append(&writer, payload, sizeof(payload)) ||
                bh_network_packet_send(&transport, &writer, player->id, 1) < 0)
            {
                session->failed = 1;
                return;
            }
        }
    }
}

void bh_server_traffic_close(BHServerTraffic *traffic)
{
    free(traffic->launchers);
}
