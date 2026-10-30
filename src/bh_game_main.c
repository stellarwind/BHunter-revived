#include "bh_game_app.h"
#include <string.h>
int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show)
{
    char path[MAX_PATH], *end, *option;
    DWORD length;
    int result, check = 0, profile = 0, box = 0, automatic = 0;
    (void)previous;
    for (option = strtok(command, " \t"); option; option = strtok(0, " \t"))
    {
        if (!strcmp(option, "--startup-check"))
            check = 1;
#ifdef DEBUGGING
        else if (!strcmp(option, "--box-test"))
            box = 1;
        else if (!strcmp(option, "--box-auto"))
        {
            box = 1;
            automatic = 1;
        }
        else if (!strcmp(option, "--box-aggression"))
        {
            box = 1;
            automatic = 2;
        }
        else if (!strcmp(option, "--profile"))
            profile = 1;
#endif
        else
            return 2;
    }

#ifndef DEBUGGING
    if (profile || box || automatic)
        return 2;
#else
    profile = 1;
#endif
    length = GetModuleFileNameA(instance, path, sizeof(path));
    if (!length || length >= sizeof(path))
        return 2;
#ifdef DEBUGGING
    if (strstr(path, "BHunter-box.exe"))
    {
        box = 1;
    }
#endif
    end = strrchr(path, '\\');
    if (!end)
        return 2;
    *end = 0;
    if (!SetCurrentDirectoryA(path))
        return 2;
    /* Internal diagnostic bits: bounded startup1, profile8, box16, auto32. */
    result = bh_game_app_run(instance, "GAME_DATA",
                             check | profile * 8 | box * 16 | automatic * 32,
                             check ? SW_HIDE : show);

#ifdef DEBUGGING
    if (result && !check && !automatic)
        MessageBoxA(
            0, "The source preview stopped at an explicit dependency error. See startup.log for details.",
            "BHunter source preview", MB_OK | MB_ICONERROR);
#else
    if (result && !check)
        MessageBoxA(0, "The game stopped at a dependency error.", "BHunter", MB_OK | MB_ICONERROR);
#endif
    return result;
}
