#ifndef BH_SDL_DIAGNOSTICS_H
#define BH_SDL_DIAGNOSTICS_H

#include <SDL3/SDL_error.h>
#include <cstdio>

namespace bh_sdl_diagnostics
{
inline char message[1024];

inline void set_message(const char *text)
{
    std::snprintf(message, sizeof(message), "%s", text);
}

inline void set_error(const char *context)
{
    std::snprintf(message, sizeof(message), "%s: %s", context, SDL_GetError());
}
} // namespace bh_sdl_diagnostics

#endif
