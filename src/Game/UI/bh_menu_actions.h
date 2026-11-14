#ifndef BH_MENU_ACTIONS_H
#define BH_MENU_ACTIONS_H
#include "bh_menu_edits.h"
#include "../../Input/bh_input_text.h"
#include "../../Audio/bh_media_control.h"
#include "../bh_messages.h"
typedef struct BHMenuActions BHMenuActions;
typedef void (*BHMenuAction)(BHMenuActions *, long);
/* Borrowed selection and row actions for the active menu. */
typedef struct BHMenuActionsRoot
{
    BHMenuSettingsMenu *menu;
    BHMenuAction *row_actions;
    unsigned long action_count;
} BHMenuActionsRoot;
struct BHMenuActions
{
    BHConfig *config;
    const char *save_path;
    BHDeviceName *devices;
    unsigned long device_count;
    BHMenuSettings *settings;
    BHMenuSettingsMenu *menu;
    BHMenuRows *rows;
    BHMenuProfileContext *profiles;
    BHInputTextCapture *text;
    BHMediaControl *media;
    BHMessageState *messages;
    BHSoundQueue *sound;
    unsigned long denial_sound;
    const char *player_name;
    const char *network_address;
    long *network_ship;
    void (*network_start)(void *user, const char *address);
    void *network_user;
    unsigned long refreshes, paints, saves;
    int error;
};
void bh_menu_action_video(BHMenuActions *, long);           /*42d0d0*/
void bh_menu_action_audio(BHMenuActions *, long);           /*42db60*/
void bh_menu_action_profile(BHMenuActions *, long);         /*42df90*/
void bh_menu_action_profile_promote(BHMenuActions *, long); /*42e150*/
void bh_menu_action_graphics_notice(BHMenuActions *, long); /*42f2b0*/
void bh_menu_action_network_host(BHMenuActions *, long);
void bh_menu_action_network_join(BHMenuActions *, long);
void bh_menu_network_address(BHMenuActions *, long);
/* Callers supply real initialized owners, current/root menus, safe terminated
   original profile text and writable private CFG path. Original rows
   may have absent refresh/action methods; non-NULL methods must be actual
   source owners.
   Errors retain already-performed original effects; no rollback is added. */
#endif
