#include "bh_game_app.h"
#include "bh_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bh_network_game.h"
int main(int argc, char **argv)
{
    const char *assets = 0, *settings = 0, *level = NULL;
    int i, result, profile = 0, network = 0, vehicle = -1;
    unsigned long port = BH_NETWORK_PORT;
    const char *address = NULL;
    for (i = 1; i < argc; ++i)
    {
#ifdef DEBUGGING
        if (!strcmp(argv[i], "--profile"))
        {
            profile = 1;
            continue;
        }
#endif
        if (!strcmp(argv[i], "--host") && !network)
            network = 1;
        else if (!strcmp(argv[i], "--join") && !network && i + 1 < argc)
        {
            address = argv[++i];
            network = 1;
        }
        else if (!strcmp(argv[i], "--port") && i + 1 < argc)
        {
            const char *number = argv[++i];
            char *end;
            port = strtoul(number, &end, 10);
            if (*number < '0' || *number > '9' || *end || !port || port > 65535)
                return 2;
        }
        else if (!strcmp(argv[i], "--vehicle") && vehicle < 0 && i + 1 < argc)
        {
            const char *number = argv[++i];
            if (number[0] < '0' || number[0] >= '0' + BH_NETWORK_CARS || number[1])
            {
                fprintf(stderr, "--vehicle must be 0, 1, or 2.\n");
                return 2;
            }
            vehicle = number[0] - '0';
        }
        else if (!strcmp(argv[i], "--asset-dir") && i + 1 < argc)
            assets = argv[++i];
        else if (!strcmp(argv[i], "--settings-dir") && i + 1 < argc)
            settings = argv[++i];
        else if (!strcmp(argv[i], "--level") && !level && i + 1 < argc)
        {
            level = argv[++i];
            if (!bh_level_name_valid(level))
            {
                fprintf(stderr, "--level takes a package name such as crossroads (without .bhlevel).\n");
                return 2;
            }
        }
        else
            return 2;
    }
    if (address && level)
    {
        fprintf(stderr, "--join uses the host's level and cannot be combined with --level.\n");
        return 2;
    }
    if (vehicle >= 0 && !network)
    {
        fprintf(stderr, "--vehicle requires --host or --join.\n");
        return 2;
    }
    if (!bh_sdl_paths(assets, settings))
        return 2;
    result = bh_game_app_run("GAME_DATA", bh_sdl_settings_file("source-preview.cfg"), profile,
                             address, network ? (unsigned short)port : 0, level, vehicle);
    if (result)
        bh_sdl_error_dialog();
    return result;
}
