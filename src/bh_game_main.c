#include "bh_game_app.h"
#include <string.h>
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show)
{
    char path[MAX_PATH], *end, *option;
    DWORD length;
    int result, profile = 0;
    (void)previous;
    for (option = strtok(command, " \t"); option; option = strtok(0, " \t"))
    {
#ifdef DEBUGGING
        if (!strcmp(option, "--profile"))
            profile = 1;
        else
#endif
            return 2;
    }
    length = GetModuleFileNameA(instance, path, sizeof(path));
    if (!length || length >= sizeof(path))
        return 2;
    end = strrchr(path, '\\');
    if (!end)
        return 2;
    *end = 0;
    if (!SetCurrentDirectoryA(path))
        return 2;
    result = bh_game_app_run(instance, "GAME_DATA", profile, show);
#ifdef DEBUGGING
    if (result)
        MessageBoxA(0, "The game stopped at a dependency error. See startup.log for details.", "BHunter",
                    MB_OK | MB_ICONERROR);
#else
    if (result)
        MessageBoxA(0, "The game stopped at a dependency error.", "BHunter", MB_OK | MB_ICONERROR);
#endif
    return result;
}
