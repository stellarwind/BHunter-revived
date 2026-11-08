#ifndef BH_MENU_SETTINGS_H
#define BH_MENU_SETTINGS_H
#include "bh_config.h"

typedef struct BHMenuSettingsMenu BHMenuSettingsMenu;
typedef void (*BHMenuSettingsRefresh)(BHMenuSettingsMenu *, long);
/* Explicit borrowed source representation of the verified menu+1d0 integer,
   +1e0 float and +200 row pointers, whose row+18 method is optional. It is
   not an invented binary overlay or an implementation of menu construction. */
typedef struct BHMenuSettingsRow
{
    BHMenuSettingsRefresh refresh;
} BHMenuSettingsRow;
struct BHMenuSettingsMenu
{
    long selection;
    float display_selection;
    BHMenuSettingsRow **rows;
    unsigned long row_count;
    void *user;
};
typedef struct BHMenuSettings
{
    BHInputProfile *input_profile;
    BHMenuSettingsMenu *ship_menu;
    long current_index, action;
} BHMenuSettings;

/* Whole0042cae0. All bindings must outlive calls;
   menu and profile creation/row-label refresh remain external owners. */
void bh_menu_submenu_first(BHMenuSettings *);
/* Whole0042e360/0042e540/0042e690/0042e7e0. Copy first input byte to
   profile offsets1c..21/28..30/31..39/3a..43 for indices1..6/9/9/10,
   then call the supplied row's real optional refresh method. Even a valid
   row index outside the assignment range still refreshes. */
void bh_menu_keys_motion(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
void bh_menu_keys_weapons(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
void bh_menu_keys_camera(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
void bh_menu_keys_extra(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
#endif
