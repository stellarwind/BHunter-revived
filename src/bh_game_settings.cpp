#include "bh_game_settings.h"
#include "../third_party/inih/ini.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <iterator>

namespace
{
constexpr int default_fov = 60, min_fov = 35, max_fov = 100;
const BHGameSettings defaults = {
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 960, false, false, 0, 0, 0, default_fov, {}, false};
std::string settings_path = BH_GAME_SETTINGS_FILENAME;

void read_setting_int(const char *value, int &setting)
{
    int number;
    const char *end = value + strlen(value);
    const auto parsed = std::from_chars(value, end, number);
    if (parsed.ec == std::errc{} && parsed.ptr == end)
        setting = number;
}

void read_setting_bool(const char *value, bool &setting)
{
    if (!SDL_strcasecmp(value, "true"))
        setting = true;
    else if (!SDL_strcasecmp(value, "false"))
        setting = false;
}

int read_setting(void *user, const char *section, const char *key, const char *value)
{
    auto &settings = *static_cast<BHGameSettings *>(user);
    if (!SDL_strcasecmp(section, "Window"))
    {
        if (!SDL_strcasecmp(key, "X"))
            read_setting_int(value, settings.window_x);
        else if (!SDL_strcasecmp(key, "Y"))
            read_setting_int(value, settings.window_y);
        else if (!SDL_strcasecmp(key, "Width"))
            read_setting_int(value, settings.window_width);
        else if (!SDL_strcasecmp(key, "Height"))
            read_setting_int(value, settings.window_height);
        else if (!SDL_strcasecmp(key, "Maximized"))
            read_setting_bool(value, settings.maximized);
    }
    else if (!SDL_strcasecmp(section, "Display"))
    {
        if (!SDL_strcasecmp(key, "Mode"))
        {
            if (!SDL_strcasecmp(value, "Borderless"))
                settings.borderless = true;
            else if (!SDL_strcasecmp(value, "Windowed"))
                settings.borderless = false;
        }
        else if (!SDL_strcasecmp(key, "AspectRatio"))
        {
            for (int i = 0; i < (int)std::size(bh_game_aspects); ++i)
                if (!strcmp(value, bh_game_aspects[i].label))
                    settings.aspect = i;
        }
        else if (!SDL_strcasecmp(key, "RenderWidth"))
            read_setting_int(value, settings.render_width);
        else if (!SDL_strcasecmp(key, "RenderHeight"))
            read_setting_int(value, settings.render_height);
        else if (!SDL_strcasecmp(key, "FOV"))
            read_setting_int(value, settings.field_of_view);
    }
    else if (!SDL_strcasecmp(section, "Multiplayer") && !SDL_strcasecmp(key, "PlayerName"))
        settings.player_name = value;
    else if (!SDL_strcasecmp(section, "Gameplay") && !SDL_strcasecmp(key, "UnlockAllLevels"))
        read_setting_bool(value, settings.unlock_all_levels);
    return 1;
}
} // namespace

BHGameSettings bh_game_settings = defaults;

extern "C"
{
    void bh_game_settings_load(const char *path)
    {
        settings_path = path;
        auto &settings = bh_game_settings;
        settings = defaults;
        ini_parse(settings_path.c_str(), read_setting, &settings);
        settings.field_of_view = std::clamp(settings.field_of_view, min_fov, max_fov);
        settings.window_width = std::clamp(settings.window_width, 640, 3840);
        settings.window_height = std::clamp(settings.window_height, 480, 2160);
    }

    const char *bh_game_settings_player_name(void)
    {
        return bh_game_settings.player_name.c_str();
    }

    int bh_game_settings_unlock_all_levels(void)
    {
        return bh_game_settings.unlock_all_levels;
    }
}

void bh_game_settings_save()
{
    const auto &settings = bh_game_settings;
    const auto temporary = settings_path + ".tmp";
    FILE *f = fopen(temporary.c_str(), "w");
    if (!f)
        return;
    const int written =
        fprintf(f,
                "[Display]\n"
                "; Use Windowed or Borderless.\n"
                "Mode=%s\n"
                "; Choose 4:3, 16:9, 16:10, or 21:9.\n"
                "AspectRatio=%s\n"
                "; Render size in Borderless mode. Windowed mode follows the window size.\n"
                "RenderWidth=%d\n"
                "RenderHeight=%d\n"
                "; Base horizontal FOV in degrees (4:3). Min %d, max %d, default %d. Restart to apply.\n"
                "FOV=%d\n"
                "\n"
                "[Window]\n"
                "; Desktop position of the restored window.\n"
                "X=%d\n"
                "Y=%d\n"
                "; Width and height of the restored window.\n"
                "Width=%d\n"
                "Height=%d\n"
                "; Use true to maximize the window, or false to use the saved size.\n"
                "Maximized=%s\n"
                "\n"
                "[Multiplayer]\n"
                "; Leave blank to use the computer name. The game uses up to 20 bytes.\n"
                "PlayerName=%s\n"
                "\n"
                "[Gameplay]\n"
                "UnlockAllLevels=%s\n",
                settings.borderless ? "Borderless" : "Windowed", bh_game_aspects[settings.aspect].label,
                settings.render_width, settings.render_height, min_fov, max_fov, default_fov, settings.field_of_view,
                settings.window_x, settings.window_y, settings.window_width, settings.window_height,
                settings.maximized ? "true" : "false", settings.player_name.c_str(),
                settings.unlock_all_levels ? "true" : "false");
    if (fclose(f) == 0 && written >= 0)
        SDL_RenamePath(temporary.c_str(), settings_path.c_str());
}
