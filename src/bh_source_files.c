#include "bh_source_files.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void __cdecl bh_source_files_init(BHSourceFiles *c)
{
    if (!c)
        return;
    memset(c, 0, sizeof(*c));
    c->root = HKEY_LOCAL_MACHINE;
    c->subkey = "SOFTWARE\\InterActive Vision\\BHunter\\Paths";
    c->expected_checksum = 0x6b405d08UL;
}
int __cdecl bh_source_files_check(BHSourceFiles *c)
{
    HKEY key;
    DWORD size = 0, type = 0;
    char *directory = NULL;
    char paths[3][512];
    static const char *names[3] = {"copyright.txt", "BIP_B2.TXT", "BIP_B1.TXT"};
    unsigned char *buffer = NULL;
    FILE *file = NULL;
    long length;
    unsigned long remaining, n, i, j, used;
    int ok = 0;
    const char *separator;
    if (!c || !c->root || !c->subkey)
        return 0;
    c->checksum = c->bytes = c->blocks = 0;
    c->error = 0;
    c->registry_result = RegOpenKeyExA(c->root, c->subkey, 0, KEY_QUERY_VALUE, &key);
    if (c->registry_result != ERROR_SUCCESS)
    {
        c->error = 1;
        return 0;
    }
    c->registry_result = RegQueryValueExA(key, "SourceDir", NULL, &type, NULL, &size);
    if (c->registry_result != ERROR_SUCCESS || !size || size > 65536)
    {
        c->error = 1;
        goto done;
    }
    directory = (char *)malloc(size);
    if (!directory)
    {
        c->error = 2;
        goto done;
    }
    c->registry_result = RegQueryValueExA(key, "SourceDir", NULL, &type, (LPBYTE)directory, &size);
    if (c->registry_result != ERROR_SUCCESS || !size || !memchr(directory, 0, size))
    {
        c->error = 1;
        goto done;
    }
    used = strlen(directory);
    if (!used || used > sizeof(paths[0]) - 16)
    {
        c->error = 3;
        goto done;
    }
    separator = directory[used - 1] == '\\' ? "" : "\\";
    for (i = 0; i < 3; i++)
        sprintf(paths[i], "%s%s%s", directory, separator, names[i]);
    free(directory);
    directory = NULL;
    file = fopen(paths[0], "rb");
    if (!file)
    {
        c->error = 4;
        goto done;
    }
    if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET))
    {
        c->error = 4;
        goto done;
    }
    remaining = (unsigned long)length;
    buffer = (unsigned char *)malloc(65536);
    if (!buffer)
    {
        c->error = 2;
        goto done;
    }
    while (remaining)
    {
        n = remaining > 65536 ? 65536 : remaining;
        if (fread(buffer, 1, n, file) != n)
        {
            c->error = 4;
            goto done;
        }
        for (j = 0; j < n / 4; j++)
        {
            i = j * 4;
            c->checksum ^= (unsigned long)buffer[i] | ((unsigned long)buffer[i + 1] << 8) |
                           ((unsigned long)buffer[i + 2] << 16) | ((unsigned long)buffer[i + 3] << 24);
        }
        c->bytes += n;
        ++c->blocks;
        remaining -= n;
    }
    ok = c->checksum == c->expected_checksum;
done:
    if (buffer)
        free(buffer);
    if (file)
        fclose(file);
    if (directory)
        free(directory);
    RegCloseKey(key);
    return ok;
}
