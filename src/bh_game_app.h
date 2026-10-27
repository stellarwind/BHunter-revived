#ifndef BH_GAME_APP_H
#define BH_GAME_APP_H
#include <windows.h>
/* Source-owned window/application composition. No original code loader or
   fixture callback enters this target. Startup-check stops before input
   collection, entity steps, sound playback ticks or the application loop. */
int __cdecl bh_game_app_run(HINSTANCE instance, const char *data_directory, int startup_check, int show);
int __cdecl bh_game_assets_verify(const char *archive, unsigned long *bytes, unsigned long *crc);
#endif
