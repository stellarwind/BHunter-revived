#ifndef BH_MENU_SETTINGS_H
#define BH_MENU_SETTINGS_H
#include "bh_config.h"

typedef struct BHMenuSettingsMenu BHMenuSettingsMenu;
typedef void(__cdecl *BHMenuSettingsRefresh)(BHMenuSettingsMenu *, long);
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
/* Exactly188 bytes per original network profile, borrowed from its real
   owner. The selected+1c text spans96 bytes; selected+7c spans64 bytes. */
typedef struct BHMenuNetworkRecord
{
    unsigned char bytes[188];
} BHMenuNetworkRecord;
typedef struct BHMenuSettings
{
    BHInputProfile *input_profile;
    BHMenuSettingsMenu *network_menu, *submenus[2], *network_choices[3];
    BHMenuNetworkRecord *network_records, *selected_network;
    unsigned long network_record_count;
    long current_index, action, last_nonzero, launch_state;
} BHMenuSettings;

/* Whole0042caa0/0042cae0/0042cb10/0042ea70. All bindings must outlive calls;
   menu and profile creation/row-label refresh remain external owners. */
void __cdecl bh_menu_network_open(BHMenuSettings *, long index);
void __cdecl bh_menu_submenu_first(BHMenuSettings *);
void __cdecl bh_menu_submenu_second(BHMenuSettings *);
void __cdecl bh_menu_network_select(BHMenuSettings *, long index);
/* Whole0042e360/0042e540/0042e690/0042e7e0. Copy first input byte to
   profile offsets1c..21/28..30/31..39/3a..43 for indices1..6/9/9/10,
   then call the supplied row's real optional refresh method. Even a valid
   row index outside the assignment range still refreshes. */
void __cdecl bh_menu_keys_motion(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
void __cdecl bh_menu_keys_weapons(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
void __cdecl bh_menu_keys_camera(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
void __cdecl bh_menu_keys_extra(BHMenuSettings *, BHMenuSettingsMenu *, const unsigned char *, long);
/* Whole0042eae0/0042ee50 for valid strings fitting their owning record.
   Bad source bindings/indices/overlong text are rejected without writes;
   original adjacent-memory overflow is not reproduced. */
void __cdecl bh_menu_network_address(BHMenuSettings *, BHMenuSettingsMenu *, const char *, long);
void __cdecl bh_menu_network_phone(BHMenuSettings *, BHMenuSettingsMenu *, const char *, long);
#endif
