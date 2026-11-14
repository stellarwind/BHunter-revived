#ifndef BH_SDL_GPU_H
#define BH_SDL_GPU_H

#include <SDL3/SDL_video.h>

namespace bh_sdl_gpu
{
struct Display
{
    int width, height;
    float margin_x, margin_y;
    int aspect;
    bool borderless;
};

// The caller owns the window and supplies current geometry on request.
bool open(SDL_Window *window, Display (*query_display)());
void reset_draw_state();
void close();
} // namespace bh_sdl_gpu

#endif
