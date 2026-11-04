#include "bh_game_app.h"
#include "bh_platform.h"
#include <string.h>
int main(int argc, char **argv)
{
    const char *assets = 0, *settings = 0;
    int i, result, profile = 0;
    for (i = 1; i < argc; ++i)
    {
#ifdef DEBUGGING
        if (!strcmp(argv[i], "--profile"))
        {
            profile = 1;
            continue;
        }
#endif
        if (!strcmp(argv[i], "--asset-dir") && i + 1 < argc)
            assets = argv[++i];
        else if (!strcmp(argv[i], "--settings-dir") && i + 1 < argc)
            settings = argv[++i];
        else
            return 2;
    }
    if (!bh_sdl_paths(assets, settings))
        return 2;
    result = bh_game_app_run("GAME_DATA", bh_sdl_settings_file("source-preview.cfg"), profile);
    if (result)
        bh_sdl_error_dialog();
    return result;
}
