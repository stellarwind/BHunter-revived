#ifndef BH_SDL_INPUT_H
#define BH_SDL_INPUT_H

#include "../Platform/bh_platform.h"
#include <SDL3/SDL.h>

namespace bh_sdl_input
{
void reset(BHPlatformInput callback, void *user);
void set_window(SDL_Window *window);
void open_gamepad();
void handle_event(const SDL_Event &event);
void close();
} // namespace bh_sdl_input

#endif
