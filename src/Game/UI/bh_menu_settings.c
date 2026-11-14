#include "bh_menu_settings.h"
#include <stddef.h>
static_assert(sizeof(BHInputProfile) == 76, "BHInputProfile size");

static void selection(BHMenuSettingsMenu *menu, long index)
{
    menu->selection = index;
    menu->display_selection = (float)index;
}
static int row_valid(BHMenuSettingsMenu *menu, long index)
{
    return menu && index >= 0 && (unsigned long)index < menu->row_count && menu->rows && menu->rows[index];
}
static void refresh(BHMenuSettingsMenu *menu, long index)
{
    BHMenuSettingsRefresh method = menu->rows[index]->refresh;
    if (method)
        method(menu, index);
}
void bh_menu_submenu_first(BHMenuSettings *menu_settings)
{
    if (!menu_settings || !menu_settings->ship_menu)
        return;
    selection(menu_settings->ship_menu, menu_settings->current_index);
    menu_settings->action = (long)((unsigned long)menu_settings->current_index + 14UL);
}
static void assign(BHMenuSettings *menu_settings, BHMenuSettingsMenu *menu, const unsigned char *input, long index,
                   unsigned char *keys, long count)
{
    if (!menu_settings || !menu_settings->input_profile || !row_valid(menu, index))
        return;
    if (index >= 1 && index <= count)
    {
        if (!input)
            return;
        keys[index - 1] = *input;
    }
    refresh(menu, index);
}
void bh_menu_keys_motion(BHMenuSettings *menu_settings, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(menu_settings, m, p, i, menu_settings && menu_settings->input_profile ? menu_settings->input_profile->motion_keys : NULL, 6);
}
void bh_menu_keys_weapons(BHMenuSettings *menu_settings, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(menu_settings, m, p, i, menu_settings && menu_settings->input_profile ? menu_settings->input_profile->primary_keys : NULL, 9);
}
void bh_menu_keys_camera(BHMenuSettings *menu_settings, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(menu_settings, m, p, i, menu_settings && menu_settings->input_profile ? menu_settings->input_profile->secondary_keys : NULL, 9);
}
void bh_menu_keys_extra(BHMenuSettings *menu_settings, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(menu_settings, m, p, i, menu_settings && menu_settings->input_profile ? menu_settings->input_profile->camera_keys : NULL, 10);
}
