#include "bh_paths.h"
#include <windows.h>
#include <string.h>

static int join(char *output, unsigned long capacity, const char *directory, const char *name)
{
    unsigned long length = 0, tail = (unsigned long)strlen(name), separator;
    if (!output || !capacity || !directory)
        return 0;
    while (length < 256 && directory[length])
        ++length;
    if (!length || length == 256)
        return 0;
    separator = directory[length - 1] != '\\';
    if (length + separator + tail >= capacity)
        return 0;
    memcpy(output, directory, length);
    if (separator)
        output[length++] = '\\';
    memcpy(output + length, name, tail + 1);
    return 1;
}

int __cdecl bh_paths_init(BHPaths *paths, const char *windows_directory, const char *current_directory)
{
    BHPaths next;
    char archive[] = "CIVOUFS/CJO";
    int i;
    if (!paths)
        return 0;
    next = *paths;
    for (i = 0; archive[i]; i++)
        --archive[i];
    if (windows_directory && !join(next.ini, sizeof(next.ini), windows_directory, "BHUNTER.INI"))
        return 0;
    if (!join(next.archive, sizeof(next.archive), current_directory, archive))
        return 0;
    *paths = next;
    return 2;
}

int __cdecl bh_paths_config(char *output, unsigned long capacity, const char *windows_directory)
{
    char next[256];
    unsigned long length;
    if (!output || !capacity)
        return 0;
    if (windows_directory)
    {
        if (!join(next, sizeof(next), windows_directory, "BHUNTER.CFG"))
            return 0;
    }
    else
        strcpy(next, "BHUNTER.CFG");
    length = (unsigned long)strlen(next);
    if (length >= capacity)
        return 0;
    memcpy(output, next, length + 1);
    return 1;
}

int __cdecl bh_paths_discover(BHPaths *paths)
{
    char windows_directory[256], current_directory[256];
    unsigned int windows_length;
    unsigned long current_length;
    windows_length = GetWindowsDirectoryA(windows_directory, 255);
    current_length = GetCurrentDirectoryA(255, current_directory);
    if (windows_length >= 255 || !current_length || current_length >= 255)
        return 0;
    return bh_paths_init(paths, windows_length ? windows_directory : NULL, current_directory);
}
