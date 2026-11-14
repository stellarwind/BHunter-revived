#include "bh_server_traffic.h"
#include "../Assets/bh_vfile.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum
{
    PLACEMENT_BYTES = 32,
    OVERRIDE_BYTES = 20,
    ARENA_START = 3333,
    ACTOR_LAUNCH = 10
};

struct BHServerLauncher
{
    unsigned short index, count, tag;
    unsigned char type, bounty, speed;
    int launcher;
};

static int read(BHVFile *file, void *bytes, unsigned long size)
{
    return bh_vfile_read(bytes, size, 1, file) == 1;
}

static uint16_t word(const unsigned char *bytes)
{
    return (uint16_t)(bytes[0] | (bytes[1] << 8));
}

static uint32_t dword(const unsigned char *bytes)
{
    return word(bytes) | ((uint32_t)word(bytes + 2) << 16);
}

static void put_word(unsigned char *bytes, uint16_t value)
{
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
}

static int read_launchers(BHServerTraffic *traffic, BHVFile *level)
{
    unsigned char record[PLACEMENT_BYTES];
    long length;
    uint32_t count, remaining;
    if (bh_vfile_seek(level, 0, SEEK_END) || (length = bh_vfile_tell(level)) < 4 || bh_vfile_seek(level, 0, SEEK_SET) ||
        !read(level, record, 4))
        return 0;
    count = dword(record);
    if (!count || count > UINT16_MAX || count > (uint32_t)(length - 4) / PLACEMENT_BYTES)
        return 0;
    remaining = (uint32_t)(length - 4) - count * PLACEMENT_BYTES;
    if (remaining % OVERRIDE_BYTES)
        return 0;
    traffic->launchers = calloc(count, sizeof(*traffic->launchers));
    if (!traffic->launchers)
        return 0;
    for (uint32_t i = 0; i < count; ++i)
    {
        BHServerLauncher *launcher = traffic->launchers + i;
        if (!read(level, record, PLACEMENT_BYTES))
            return 0;
        record[16] = 0;
        launcher->launcher =
            !_stricmp((const char *)record, "LAUNCHER.BSP") || !_stricmp((const char *)record, "LAUNCH2.BSP");
        launcher->index = (unsigned short)i;
    }
    for (; remaining; remaining -= OVERRIDE_BYTES)
    {
        uint32_t index;
        if (!read(level, record, OVERRIDE_BYTES))
            return 0;
        index = dword(record);
        if (index < count && traffic->launchers[index].launcher)
        {
            BHServerLauncher *launcher = traffic->launchers + index;
            int16_t amount = (int16_t)word(record + 6);
            uint16_t type = word(record + 10);
            launcher->count = 0;
            if (word(record + 4) == ARENA_START && amount > 0 && (type == 0 || type == 3))
            {
                launcher->count = (unsigned short)amount;
                launcher->tag = word(record + 8);
                launcher->type = (unsigned char)type;
                launcher->bounty = (unsigned char)((int16_t)word(record + 12) / 5);
                launcher->speed = (unsigned char)word(record + 14);
            }
        }
    }
    for (uint32_t i = 0; i < count; ++i)
        if (traffic->launchers[i].count)
            traffic->launchers[traffic->count++] = traffic->launchers[i];
    return 1;
}

int bh_server_traffic_load(BHServerTraffic *traffic, BHServerSession *session, const BHLevelIdentity *identity)
{
    BHVFile *file;
    int result;
    memset(traffic, 0, sizeof(*traffic));
    traffic->session = session;
    traffic->seed = (uint32_t)identity->fingerprint ^ (uint32_t)time(NULL);
    if (!identity->name[0])
        return 1;
    file = bh_vfile_open_level("level.lev");
    if (!file)
        return 0;
    result = read_launchers(traffic, file);
    bh_vfile_close(file);
    if (!result)
        bh_server_traffic_close(traffic);
    return result;
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
