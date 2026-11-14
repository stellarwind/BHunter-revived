#ifndef BH_SDL_TEXT_H
#define BH_SDL_TEXT_H

#include "../Graphics/Textures/bh_overlay_text.h"

namespace bh_sdl_text
{
bool prepare();
bool rasterize(BHOverlayCanvas &canvas, int x, int y, const char *text, unsigned long rgb);
} // namespace bh_sdl_text

#endif
