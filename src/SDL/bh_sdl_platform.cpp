#include "../bh_platform.h"
#include "../bh_game_settings.h"
#include "../bh_sdl_audio.h"
#include "../bh_video_layout.h"
#include "bh_sdl_diagnostics.h"
#include "bh_sdl_gpu.h"
#include "bh_sdl_input.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
using bh_sdl_diagnostics::set_error;
SDL_Window *window;
bool quitting, hidden, renderer_open;
char report_text[1024];
std::string settings_directory;

std::vector<std::pair<int, int>> modes;
int selected_resolution = 0;
void output_size(int &w, int &h)
{
    if (!SDL_GetWindowSizeInPixels(window, &w, &h))
    {
        w = bh_game_settings.window_width;
        h = bh_game_settings.window_height;
    }
    if (bh_game_settings.borderless && !modes.empty())
    {
        w = modes[selected_resolution].first;
        h = modes[selected_resolution].second;
    }
    w = std::max(w, 1);
    h = std::max(h, 1);
}
bh_sdl_gpu::Display display_geometry()
{
    int w, h;
    output_size(w, h);
    const auto &selected = bh_game_aspects[bh_game_settings.aspect];
    float aspect = (float)selected.numerator / selected.denominator;
    /* The selector groups common modes (e.g. 2560x1080 and 3440x1440 are
       both sold as 21:9). Use the chosen mode's exact shape in Borderless,
       never the desktop's shape, so its pixels and the UI stay square. */
    if (bh_game_settings.borderless && !modes.empty() &&
        video_layout_aspect_matches(w, h, selected.numerator, selected.denominator))
        aspect = (float)w / h;
    bh_sdl_gpu::Display display{w, h, 0, 0, bh_game_settings.aspect, bh_game_settings.borderless};
    video_layout_canvas(aspect, &display.margin_x, &display.margin_y);
    return display;
}
bool matching_mode(int index)
{
    const auto &aspect = bh_game_aspects[bh_game_settings.aspect];
    return video_layout_aspect_matches(modes[index].first, modes[index].second, aspect.numerator, aspect.denominator);
}
void select_matching_mode()
{
    if (modes.empty() || matching_mode(selected_resolution))
        return;
    int best = -1;
    long long distance = 0;
    const auto current = modes[selected_resolution];
    for (int i = 0; i < (int)modes.size(); ++i)
        if (matching_mode(i))
        {
            long long delta =
                std::llabs((long long)modes[i].first * modes[i].second - (long long)current.first * current.second);
            if (best < 0 || delta < distance)
            {
                best = i;
                distance = delta;
            }
        }
    if (best >= 0)
        selected_resolution = best;
}
void finish_modes(int saved_width, int saved_height)
{
    const std::pair<int, int> render_sizes[] = {
        {640, 480},   {800, 600},  {1024, 768}, {1280, 960},  {1600, 1200}, {1920, 1440}, {2560, 1920}, {2880, 2160},
        {854, 480},   {960, 540},  {1280, 720}, {1366, 768},  {1600, 900},  {1920, 1080}, {2560, 1440}, {3200, 1800},
        {3840, 2160}, {1120, 480}, {1680, 720}, {2560, 1080}, {3440, 1440}, {3840, 1600},
        {768, 480},   {1280, 800}, {1440, 900}, {1680, 1050}, {1920, 1200}, {2560, 1600}, {3456, 2160}};
    const auto &aspect = bh_game_aspects[bh_game_settings.aspect];
    if (saved_width >= 640 && saved_height >= 480 && saved_width <= 3840 && saved_height <= 2160 &&
        video_layout_aspect_matches(saved_width, saved_height, aspect.numerator, aspect.denominator) &&
        std::find(modes.begin(), modes.end(), std::make_pair(saved_width, saved_height)) == modes.end())
        modes.push_back({saved_width, saved_height});
    for (auto size : render_sizes)
        if (std::find(modes.begin(), modes.end(), size) == modes.end())
            modes.push_back(size);
    if (modes.empty())
        modes.push_back({bh_game_settings.window_width, bh_game_settings.window_height});
    std::sort(modes.begin(), modes.end());
    selected_resolution = std::clamp(selected_resolution, 0, (int)modes.size() - 1);
    auto saved_mode = std::find(modes.begin(), modes.end(), std::make_pair(saved_width, saved_height));
    if (saved_mode != modes.end())
        selected_resolution = (int)(saved_mode - modes.begin());
    select_matching_mode();
}
void save_settings()
{
    if (!window || hidden)
        return;
    const auto resolution = modes.empty() ? std::make_pair(bh_game_settings.window_width, bh_game_settings.window_height)
                                          : modes[selected_resolution];
    bh_game_settings.render_width = resolution.first;
    bh_game_settings.render_height = resolution.second;
    bh_game_settings_save();
}
void remember_window()
{
    auto flags = SDL_GetWindowFlags(window);
    if (!(flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_MINIMIZED)))
    {
        bh_game_settings.maximized = (flags & SDL_WINDOW_MAXIMIZED) != 0;
        if (!bh_game_settings.maximized)
        {
            SDL_GetWindowPosition(window, &bh_game_settings.window_x, &bh_game_settings.window_y);
            SDL_GetWindowSize(window, &bh_game_settings.window_width, &bh_game_settings.window_height);
        }
    }
    save_settings();
}
#ifdef __linux__
void enable_deck_keyboard()
{
    // Direct launches on the Deck desktop may lack Steam's launch environment.
    if (!SDL_GetHintBoolean("SteamDeck", false))
    {
        char product[32] = {};
        FILE *file = fopen("/sys/devices/virtual/dmi/id/product_name", "r");
        if (file)
        {
            if (fscanf(file, "%31s", product) != 1)
                product[0] = '\0';
            fclose(file);
        }
        if (strcmp(product, "Jupiter") && strcmp(product, "Galileo"))
            return;
        // Older SDL X11 backends use SteamDeck for this capability.
        SDL_SetHintWithPriority("SteamDeck", "1", SDL_HINT_DEFAULT);
    }
    SDL_SetHintWithPriority("SDL_ENABLE_STEAM_SCREEN_KEYBOARD", "1", SDL_HINT_DEFAULT);
    SDL_SetHintWithPriority(SDL_HINT_ENABLE_SCREEN_KEYBOARD, "1", SDL_HINT_DEFAULT);
}
#endif
void apply_mode()
{
    if (!bh_game_settings.borderless)
    {
        SDL_SetWindowFullscreen(window, false);
        SDL_SyncWindow(window);
        SDL_SetWindowSize(window, bh_game_settings.window_width, bh_game_settings.window_height);
        SDL_SetWindowPosition(window, bh_game_settings.window_x, bh_game_settings.window_y);
        if (bh_game_settings.maximized)
            SDL_MaximizeWindow(window);
    }
    else
    {
        SDL_SetWindowFullscreenMode(window, nullptr);
        SDL_SetWindowFullscreen(window, true);
    }
    SDL_SyncWindow(window);
    save_settings();
}
} // namespace

extern "C"
{
    int bh_sdl_open(int hide, BHPlatformInput callback, void *user)
    {
        hidden = hide != 0;
        quitting = false;
        bh_sdl_input::reset(callback, user);
        if (window && renderer_open)
        {
            // A new world owns new draw state, but can use the existing swapchain.
            bh_sdl_gpu::reset_draw_state();
            return 1;
        }
#ifdef __linux__
        enable_deck_keyboard();
#endif
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            set_error("SDL init");
            return 0;
        }
        /* A missing controller subsystem must not prevent keyboard play. */
        bh_sdl_input::open_gamepad();
        if (hidden)
        {
            bh_game_settings.window_width = 640;
            bh_game_settings.window_height = 480;
            bh_game_settings.borderless = false;
            bh_game_settings.maximized = false;
            bh_game_settings.aspect = 0;
        }
        window = SDL_CreateWindow("BHunter", bh_game_settings.window_width, bh_game_settings.window_height,
                                  SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
        if (!window)
        {
            set_error("Create window");
            return 0;
        }
        bh_sdl_input::set_window(window);
        SDL_SetWindowMinimumSize(window, 640, 480);
        SDL_SetWindowPosition(window, bh_game_settings.window_x, bh_game_settings.window_y);
        int count = 0;
        auto **available = SDL_GetFullscreenDisplayModes(SDL_GetDisplayForWindow(window), &count);
        for (int i = 0; i < count; ++i)
        {
            auto *m = available[i];
            std::pair<int, int> size{m->w, m->h};
            if (m->w >= 640 && m->h >= 480 && m->w <= 3840 && m->h <= 2160 &&
                std::find(modes.begin(), modes.end(), size) == modes.end())
                modes.push_back(size);
        }
        SDL_free(available);
        finish_modes(bh_game_settings.render_width, bh_game_settings.render_height);
        renderer_open = bh_sdl_gpu::open(window, display_geometry);
        if (!renderer_open)
            return 0;
        apply_mode();
        bh_sdl_clear_frame();
        if (!hidden)
        {
            SDL_HideCursor();
            SDL_ShowWindow(window);
        }
        return 1;
    }
    uint32_t bh_sdl_ticks()
    {
        return (uint32_t)SDL_GetTicks();
    }
    uint64_t bh_sdl_nanoseconds()
    {
        return SDL_GetTicksNS();
    }
    void bh_sdl_delay(uint32_t milliseconds)
    {
        SDL_Delay(milliseconds);
    }
    int bh_sdl_paths(const char *assets, const char *settings)
    {
        namespace fs = std::filesystem;
        std::error_code error;
        const char *base = SDL_GetBasePath();
        if (!base)
            return 0;
        auto root = fs::absolute(assets ? fs::path(assets) : fs::path(base), error);
        if (error)
            return 0;
        auto output = settings ? fs::absolute(fs::path(settings), error) : root;
        if (error || !fs::is_directory(output, error) || error)
            return 0;
        settings_directory = output.generic_string();
        fs::current_path(root, error);
        return !error;
    }
    const char *bh_sdl_settings_file(const char *name)
    {
        static std::string path;
        path = (std::filesystem::path(settings_directory) / name).generic_string();
        return path.c_str();
    }
    void bh_sdl_error_dialog()
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "BHunter",
                                 report_text[0] ? report_text : "The game stopped before startup completed.", window);
    }
    void bh_sdl_report_error(const char *details)
    {
        snprintf(report_text, sizeof(report_text), "%s", details);
        fprintf(stderr, "%s\n", report_text);
    }
    double __cdecl bh_sdl_seconds()
    {
        return SDL_GetTicksNS() / 1000000000.0;
    }
    void __cdecl bh_sdl_pace()
    {
        static Uint64 deadline = 0;
        const Uint64 interval = 1000000000ULL / 60;
        Uint64 now = SDL_GetTicksNS();
        if (!deadline || now > deadline + interval * 4)
            deadline = now;
        deadline += interval;
        if (deadline > now)
            SDL_DelayPrecise(deadline - now);
    }
    int __cdecl bh_sdl_poll()
    {
        bh_audio_poll();
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if ((event.type == SDL_EVENT_AUDIO_DEVICE_ADDED || event.type == SDL_EVENT_AUDIO_DEVICE_REMOVED) &&
                !event.adevice.recording)
                bh_audio_device_changed();
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                quitting = true;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.windowID == SDL_GetWindowID(window) && !event.key.repeat &&
                    event.key.scancode > SDL_SCANCODE_UNKNOWN && event.key.scancode < SDL_SCANCODE_COUNT &&
                    event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT))
                {
                    remember_window();
                    bh_game_settings.borderless = !bh_game_settings.borderless;
                    apply_mode();
                }
                else
                    bh_sdl_input::handle_event(event);
                break;
            case SDL_EVENT_WINDOW_MOVED:
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_MAXIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
                remember_window();
                break;
            default:
                bh_sdl_input::handle_event(event);
                break;
            }
        }
        return !quitting;
    }
    const char *__cdecl bh_sdl_error()
    {
        return bh_sdl_diagnostics::message;
    }
    void __cdecl bh_sdl_close()
    {
        bh_sdl_input::close();
        if (window)
            remember_window();
        bh_sdl_gpu::close();
        renderer_open = false;
        modes.clear();
        if (window)
        {
            SDL_ShowCursor();
            SDL_DestroyWindow(window);
        }
        window = nullptr;
        SDL_Quit();
    }
    void __cdecl bh_sdl_video_cycle(int row)
    {
        if (row == 0)
        {
            remember_window();
            bh_game_settings.borderless = !bh_game_settings.borderless;
            apply_mode();
        }
        if (row == 1)
        {
            bh_game_settings.aspect = (bh_game_settings.aspect + 1) % std::size(bh_game_aspects);
            select_matching_mode();
        }
        if (row == 2 && bh_game_settings.borderless && !modes.empty())
        {
            for (int i = 1; i <= (int)modes.size(); ++i)
            {
                int next = (selected_resolution + i) % modes.size();
                if (matching_mode(next))
                {
                    selected_resolution = next;
                    break;
                }
            }
        }
        save_settings();
    }
    const char *__cdecl bh_sdl_video_label(int row)
    {
        static char text[64];
        if (row == 0)
            return bh_game_settings.borderless ? "Borderless" : "Windowed";
        if (row == 1)
            return bh_game_aspects[bh_game_settings.aspect].label;
        if (!bh_game_settings.borderless)
            return "Resize window";
        if (!modes.empty() && !matching_mode(selected_resolution))
            return "Unavailable";
        int w, h;
        SDL_GetWindowSizeInPixels(window, &w, &h);
        if (bh_game_settings.borderless && !modes.empty())
        {
            w = modes[selected_resolution].first;
            h = modes[selected_resolution].second;
        }
        snprintf(text, sizeof(text), "%d x %d", w, h);
        return text;
    }
    int bh_sdl_fov_degrees()
    {
        return bh_game_settings.field_of_view;
    }
    void __cdecl bh_sdl_canvas_margins(float *x, float *y)
    {
        if (window && x && y)
        {
            const auto display = display_geometry();
            *x = display.margin_x;
            *y = display.margin_y;
        }
    }
}
