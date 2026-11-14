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
void bh_menu_submenu_first(BHMenuSettings *s)
{
    if (!s || !s->ship_menu)
        return;
    selection(s->ship_menu, s->current_index);
    s->action = (long)((unsigned long)s->current_index + 14UL);
}
static void assign(BHMenuSettings *s, BHMenuSettingsMenu *menu, const unsigned char *input, long index,
                   unsigned char *keys, long count)
{
    if (!s || !s->input_profile || !row_valid(menu, index))
        return;
    if (index >= 1 && index <= count)
    {
        if (!input)
            return;
        keys[index - 1] = *input;
    }
    refresh(menu, index);
}
void bh_menu_keys_motion(BHMenuSettings *s, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(s, m, p, i, s && s->input_profile ? s->input_profile->motion_keys : NULL, 6);
}
void bh_menu_keys_weapons(BHMenuSettings *s, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(s, m, p, i, s && s->input_profile ? s->input_profile->primary_keys : NULL, 9);
}
void bh_menu_keys_camera(BHMenuSettings *s, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(s, m, p, i, s && s->input_profile ? s->input_profile->secondary_keys : NULL, 9);
}
void bh_menu_keys_extra(BHMenuSettings *s, BHMenuSettingsMenu *m, const unsigned char *p, long i)
{
    assign(s, m, p, i, s && s->input_profile ? s->input_profile->camera_keys : NULL, 10);
}
