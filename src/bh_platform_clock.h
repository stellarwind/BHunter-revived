#ifndef BH_PLATFORM_CLOCK_H
#define BH_PLATFORM_CLOCK_H
#ifdef BH_SDL_BACKEND
#include "bh_platform.h"
#define bh_platform_ticks bh_sdl_ticks
#define bh_platform_delay bh_sdl_delay
#else
#include <windows.h>
#define bh_platform_ticks GetTickCount
#define bh_platform_delay Sleep
#endif
#endif
