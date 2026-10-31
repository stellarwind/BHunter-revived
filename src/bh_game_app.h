#ifndef BH_GAME_APP_H
#define BH_GAME_APP_H
#include <windows.h>
/* Source-owned window/application composition; optional framerate capture. */
int __cdecl bh_game_app_run(HINSTANCE instance, const char *data_directory, int profile, int show);
int __cdecl bh_game_assets_verify(const char *archive, unsigned long *bytes, unsigned long *crc);
#endif
