#include "bh_menu_edits.h"
#include "../../Platform/bh_platform.h"
#include "bh_menu_edit_names.h"
#include <stdio.h>
#include <string.h>
static void paint(BHMenuEdits *menu_edits, long row, const char *text)
{
    unsigned char body[256];
    unsigned long n;
    if (!menu_edits || !menu_edits->rows || row < 0)
    {
        if (menu_edits)
            menu_edits->error = 1;
        return;
    }
    if (text)
    {
        n = (unsigned long)strlen(text);
        if (n > 255)
        {
            menu_edits->error = 1;
            return;
        }
        memcpy(body, text, n + 1);
    }
    ++menu_edits->refreshes;
    if (!bh_menu_row_refresh(menu_edits->rows, (unsigned long)row, NULL, text ? body : NULL))
        menu_edits->error = 1;
}
static void short_text(char *out, const char *text)
{
    strncpy(out, text, 20);
    out[20] = 0;
}
static BHInputProfile *profile(BHMenuEdits *menu_edits)
{
    return menu_edits && menu_edits->settings ? menu_edits->settings->input_profile : NULL;
}
static void key(BHMenuEdits *menu_edits, long row, const unsigned char *binding, int table)
{
    BHInputProfile *p = profile(menu_edits);
    unsigned long value;
    if (!p || !binding)
    {
        if (menu_edits)
            menu_edits->error = 1;
        return;
    }
    value = *binding;
    if ((table == 1 && value >= 17) || (table == 2 && value >= 5))
    {
        menu_edits->error = 1;
        return;
    }
    paint(menu_edits, row, table == 0 ? key_names[value] : table == 1 ? joystick_names[value] : mouse_names[value]);
}
void bh_menu_edit_video(BHMenuEdits *menu_edits, long row)
{
#ifdef BH_SDL_BACKEND
    if (menu_edits && menu_edits->config && row >= 0 && row <= 2)
        paint(menu_edits, row, bh_sdl_video_label((int)row));
#else
    long value;
    char text[21];
    if (!menu_edits || !menu_edits->config)
        return;
    if (row == 0)
    {
        memcpy(&value, &menu_edits->config->values.hardware_acceleration, 4);
        if (value == 0)
            paint(menu_edits, 0, "Disable");
        else if (value == 1)
            paint(menu_edits, 0, "Allow");
    }
    else if (row == 1)
    {
        short_text(text, menu_edits->config->values.device_name);
        paint(menu_edits, 1, text);
    }
    else if (row == 2)
    {
        memcpy(&value, &menu_edits->config->values.resolution, 4);
        if (value == 0)
            paint(menu_edits, 2, "320 x 240");
        else if (value == 1)
            paint(menu_edits, 2, "640 x 480");
    }
#endif
}

void bh_menu_edit_audio(BHMenuEdits *menu_edits, long row)
{
    char text[128];
    double value;
    long percent;
    if (!menu_edits || !menu_edits->audio)
        return;
    if (row != 1 && row != 3)
        return;
    value = row == 1 ? bh_wave_audio_volume(menu_edits->audio) : bh_cd_audio_volume(menu_edits->audio);
    percent = (long)(.5 - value * (-10.0)) * 10;
    sprintf(text, "%d%% %s", percent, "Volume");
    paint(menu_edits, row == 1 ? 0 : 2, text);
    paint(menu_edits, row, text);
}
void bh_menu_edit_profile(BHMenuEdits *c, long row)
{
    char text[21];
    BHInputProfile *input_profile = profile(c);
    if (row == 0 && input_profile)
    {
        short_text(text, input_profile->name);
        paint(c, 0, text);
    }
    else if (row == 1)
        paint(c, 1, NULL);
}
void bh_menu_edit_motion(BHMenuEdits *c, long row)
{
    BHInputProfile *input_profile = profile(c);
    if (row >= 1 && row <= 6)
        key(c, row, input_profile ? &input_profile->motion_keys[row - 1] : NULL, 0);
    else if (row >= 7 && row <= 9)
        key(c, row, input_profile ? &input_profile->motion_joystick[row - 7] : NULL, 1);
    else if (row >= 10 && row <= 12)
        key(c, row, input_profile ? &input_profile->motion_mouse[row - 10] : NULL, 2);
}
void bh_menu_edit_primary(BHMenuEdits *c, long row)
{
    BHInputProfile *input_profile = profile(c);
    if (row >= 1 && row <= 9)
        key(c, row, input_profile ? &input_profile->primary_keys[row - 1] : NULL, 0);
}
void bh_menu_edit_secondary(BHMenuEdits *c, long row)
{
    BHInputProfile *input_profile = profile(c);
    if (row >= 1 && row <= 9)
        key(c, row, input_profile ? &input_profile->secondary_keys[row - 1] : NULL, 0);
}
void bh_menu_edit_camera(BHMenuEdits *c, long row)
{
    BHInputProfile *input_profile = profile(c);
    if (row >= 1 && row <= 10)
        key(c, row, input_profile ? &input_profile->camera_keys[row - 1] : NULL, 0);
    else if (row == 11)
        key(c, row, input_profile ? &input_profile->camera_joystick[1] : NULL, 1);
    else if (row == 12)
        key(c, row, input_profile ? &input_profile->camera_joystick[0] : NULL, 1);
    else if (row == 13)
        key(c, row, input_profile ? &input_profile->camera_mouse[1] : NULL, 2);
    else if (row == 14)
        key(c, row, input_profile ? &input_profile->camera_mouse[0] : NULL, 2);
}
static void refresh(BHMenuEdits *menu_edits, long row)
{
    BHMenuSettingsRow *menu_settings_row;
    if (!menu_edits || !menu_edits->menu || !menu_edits->menu->rows || row < 0 || (unsigned long)row >= menu_edits->menu->row_count ||
        !(menu_settings_row = menu_edits->menu->rows[row]))
    {
        if (menu_edits)
            menu_edits->error = 1;
        return;
    }
    if (menu_settings_row->refresh)
        menu_settings_row->refresh(menu_edits->menu, row);
}
static void capture(BHMenuEdits *menu_edits, BHMenuKeyCommit commit)
{
    BHMenuKeyCommit previous;
    if (!menu_edits || !menu_edits->capture)
        return;
    menu_edits->capture->settings = menu_edits->settings;
    menu_edits->capture->menu = menu_edits->menu;
    previous = menu_edits->capture->commit;
    menu_edits->capture->commit = commit;
    if (!bh_input_key_capture_begin(menu_edits->capture))
    {
        menu_edits->capture->commit = previous;
        menu_edits->error = 1;
    }
}
void bh_menu_action_motion(BHMenuEdits *c, long row)
{
    BHInputProfile *input_profile = profile(c);
    unsigned char *bytes;
    unsigned long offset;
    if (!input_profile)
        return;
    if (row >= 7 && row <= 12)
    {
        bytes = row <= 9 ? input_profile->motion_joystick : input_profile->motion_mouse;
        offset = (unsigned long)(row <= 9 ? row - 7 : row - 10);
        bytes[offset] = (unsigned char)((bytes[offset] + 1) % (row <= 9 ? 17 : 5));
        refresh(c, row);
    }
    else
        capture(c, bh_menu_keys_motion);
}
void bh_menu_action_primary(BHMenuEdits *c, long row)
{
    (void)row;
    capture(c, bh_menu_keys_weapons);
}
void bh_menu_action_secondary(BHMenuEdits *c, long row)
{
    (void)row;
    capture(c, bh_menu_keys_camera);
}
void bh_menu_action_camera(BHMenuEdits *c, long row)
{
    BHInputProfile *input_profile = profile(c);
    unsigned char *bytes;
    unsigned long offset;
    if (!input_profile)
        return;
    if (row >= 11 && row <= 14)
    {
        bytes = row <= 12 ? input_profile->camera_joystick : input_profile->camera_mouse;
        offset = (row == 11 || row == 13) ? 1 : 0;
        bytes[offset] = (unsigned char)((bytes[offset] + 1) % (row <= 12 ? 17 : 5));
        refresh(c, row);
    }
    else
        capture(c, bh_menu_keys_extra);
}
void bh_menu_action_mission(BHMenuEdits *menu_edits, long row)
{
    if (!menu_edits || !menu_edits->mode || !menu_edits->mission)
        return;
    *menu_edits->mode = 1;
    *menu_edits->mission = row;
}
void bh_menu_action_ship(BHMenuEdits *menu_edits, long row)
{
    short unlocked;
    unsigned long i;
    if (!menu_edits || !menu_edits->config || !menu_edits->ship || !menu_edits->mission_menu)
        return;
    for (i = 0; i < 9; ++i)
        if (!menu_edits->mission_rows[i])
        {
            menu_edits->error = 1;
            return;
        }
    *menu_edits->ship = row;
    memcpy(&unlocked, &menu_edits->config->values.unlocked_mission, 2);
    for (i = 0; i < 9; ++i)
        menu_edits->mission_rows[i]->action = menu_edits->unlock_all_levels || (long)i <= unlocked ? bh_menu_action_mission : NULL;
    menu_edits->mission_menu->selection = unlocked;
    menu_edits->mission_menu->display_selection = (float)unlocked;
}
void bh_menu_edit_row_callback(BHMenuSettingsMenu *menu_settings_menu, long row)
{
    BHMenuEditBridge *menu_edit_bridge = (BHMenuEditBridge *)menu_settings_menu->user;
    static void (*const edits[11])(BHMenuEdits *, long) = {
        bh_menu_edit_video,   bh_menu_edit_audio,     bh_menu_edit_profile, bh_menu_edit_motion,
        bh_menu_edit_primary, bh_menu_edit_secondary, bh_menu_edit_camera};
    if (menu_edit_bridge && menu_edit_bridge->kind >= 0 && menu_edit_bridge->kind <= BH_MENU_EDIT_CAMERA)
        edits[menu_edit_bridge->kind](menu_edit_bridge->context, row);
}
