#ifndef BH_GAME_SETTINGS_H
#define BH_GAME_SETTINGS_H

#define BH_GAME_SETTINGS_FILENAME "game-config.ini"

#ifdef __cplusplus
#include <string>

struct BHGameSettings
{
    int window_x, window_y, window_width, window_height;
    bool maximized, borderless;
    int aspect, render_width, render_height, field_of_view;
    std::string player_name;
};

struct BHGameAspect
{
    int numerator, denominator;
    const char *label;
};

inline constexpr BHGameAspect bh_game_aspects[] = {{4, 3, "4:3"}, {16, 9, "16:9"}, {16, 10, "16:10"}, {21, 9, "21:9"}};
extern BHGameSettings bh_game_settings;
void bh_game_settings_save();

extern "C"
{
#endif
    void bh_game_settings_load(const char *path);
    const char *bh_game_settings_player_name(void);
#ifdef __cplusplus
}
#endif

#endif
