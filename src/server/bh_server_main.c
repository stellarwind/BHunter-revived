#include "bh_server_session.h"
#include "bh_server_traffic.h"
#include "../bh_vfile.h"
#include <enet/enet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

enum { SERVER_HZ = 30, MAX_CATCHUP_TICKS = 8 };

static volatile sig_atomic_t stopping;

static void request_stop(int signal_number)
{
    (void)signal_number;
    stopping = 1;
}

static int monotonic_ms(uint64_t *milliseconds)
{
#ifdef _WIN32
    *milliseconds = GetTickCount64();
#else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0)
        return 0;
    *milliseconds = (uint64_t)now.tv_sec * 1000 + (uint64_t)now.tv_nsec / 1000000;
#endif
    return 1;
}

static int run(BHServerSession *session)
{
    uint64_t now, next_tick;
    if (!monotonic_ms(&now))
        return 0;
    next_tick = now * SERVER_HZ + 1000;
    while (!stopping)
    {
        enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE | ENET_SOCKET_WAIT_INTERRUPT;
        unsigned int ticks = 0;
        if (!bh_server_session_poll(session) || !monotonic_ms(&now))
            return 0;
        now *= SERVER_HZ;
        while (now >= next_tick && ticks < MAX_CATCHUP_TICKS)
        {
            if (!bh_server_session_tick(session))
                return 0;
            ++ticks;
            next_tick += 1000;
        }
        if (now >= next_tick)
            next_tick = now + 1000;
        enet_host_flush(session->game.host);
        if (enet_socket_wait(session->game.host->socket, &condition,
                             (enet_uint32)((next_tick - now + SERVER_HZ - 1) / SERVER_HZ)) < 0)
            return stopping != 0;
    }
    return 1;
}

static void usage(FILE *stream)
{
    fputs("Usage: BHunter-server [--port <udp-port>] [--level <name>] [--asset-dir <directory>]\n"
          "Defaults: UDP port 27890, stock arena, assets in GAME_DATA under the current directory.\n"
          "For a custom arena, place <name>.bhlevel in the asset directory on the server and clients.\n",
          stream);
}

int main(int argc, char **argv)
{
    BHServerSession session;
    BHServerTraffic traffic;
    BHLevelIdentity level = {0};
    const char *level_name = NULL, *asset_dir = "GAME_DATA";
    unsigned long port = BH_NETWORK_PORT;
    int result;

    if (argc == 2 && !strcmp(argv[1], "--help"))
    {
        usage(stdout);
        return 0;
    }
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--port") && i + 1 < argc)
        {
            const char *value = argv[++i];
            char *end;
            port = strtoul(value, &end, 10);
            if (*value < '0' || *value > '9' || *end || !port || port > 65535)
            {
                fprintf(stderr, "Invalid UDP port: %s\n", value);
                return 2;
            }
        }
        else if (!strcmp(argv[i], "--level") && i + 1 < argc)
            level_name = argv[++i];
        else if (!strcmp(argv[i], "--asset-dir") && i + 1 < argc)
            asset_dir = argv[++i];
        else
        {
            usage(stderr);
            return 2;
        }
    }
    if (level_name)
    {
        char path[512];
        int length;
        if (!bh_level_name_valid(level_name))
        {
            fputs("--level expects a package name without .bhlevel.\n", stderr);
            return 2;
        }
        length = snprintf(path, sizeof(path), "%s/%s.bhlevel", asset_dir, level_name);
        if (length < 0 || (size_t)length >= sizeof(path))
        {
            fputs("The level package path is too long.\n", stderr);
            return 2;
        }
        if (!bh_vfile_mount_level(path, &level.fingerprint))
        {
            fprintf(stderr, "Could not mount level package: %s\n", path);
            return 2;
        }
        strcpy(level.name, level_name);
    }
    if (!bh_server_traffic_load(&traffic, &session, &level))
    {
        fputs("Could not read traffic launchers from the level package.\n", stderr);
        bh_vfile_shutdown();
        return 2;
    }
    if (!bh_server_session_open(&session, (unsigned short)port, &level, NULL, bh_server_traffic_roster, &traffic))
    {
        fprintf(stderr, "Could not open dedicated session on UDP port %lu.\n", port);
        bh_server_traffic_close(&traffic);
        bh_vfile_shutdown();
        return 2;
    }
    signal(SIGINT, request_stop);
    signal(SIGTERM, request_stop);
    printf("BHunter dedicated session on UDP port %lu, level %s, %u player slots.\n",
           port, level_name ? level_name : "stock", (unsigned int)BH_NETWORK_PLAYERS);
    fflush(stdout);
    result = run(&session);
    bh_server_session_close(&session);
    bh_server_traffic_close(&traffic);
    bh_vfile_shutdown();
    if (!result)
        fputs("The dedicated session stopped after a network or clock error.\n", stderr);
    return result ? 0 : 1;
}
