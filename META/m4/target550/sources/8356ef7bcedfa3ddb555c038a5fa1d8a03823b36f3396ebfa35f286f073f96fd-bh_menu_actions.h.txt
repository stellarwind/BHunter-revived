#ifndef BH_MENU_ACTIONS_H
#define BH_MENU_ACTIONS_H
#include "bh_menu_edits.h"
#include "bh_input_text.h"
#include "bh_media_control.h"
#include "bh_network_game.h"
#include "bh_messages.h"
#include "bh_serial_options.h"
typedef struct BHMenuActions BHMenuActions;
typedef void (__cdecl *BHMenuAction)(BHMenuActions *,long);
/* Explicit borrowed original menu state: timeout+1ec,page+1cc, current menu
   selection+1d0/display+1e0, row actions+14. No binary overlay is invented. */
typedef struct BHMenuActionsRoot {
 float timeout;long page;BHMenuSettingsMenu *menu;
 BHMenuAction *row_actions;unsigned long action_count;
} BHMenuActionsRoot;
struct BHMenuActions {
 BHConfig *config;const char *save_path;BHDeviceName *devices;unsigned long device_count;
 BHMenuSettings *settings;BHMenuSettingsMenu *menu;BHMenuRows *rows;
 BHMenuProfileContext *profiles;BHInputTextCapture *text;
 BHMediaControl *media;BHModemNames *modems;BHProviderCatalog *providers;
 BHNetworkSessions *sessions;BHNetworkGame *game;BHMenuActionsRoot **root;
 BHMessageState *messages;BHSoundQueue *sound;unsigned long denial_sound;
 const char *player_name;long *mode,*network_failed,*frame_count;
 unsigned long refreshes,paints,saves;int error;
};
void __cdecl bh_menu_action_video(BHMenuActions *,long); /*42d0d0*/
void __cdecl bh_menu_action_audio(BHMenuActions *,long); /*42db60*/
void __cdecl bh_menu_action_profile(BHMenuActions *,long); /*42df90*/
void __cdecl bh_menu_action_profile_promote(BHMenuActions *,long); /*42e150*/
void __cdecl bh_menu_action_connect(BHMenuActions *,long); /*42e9e0*/
void __cdecl bh_menu_action_modem(BHMenuActions *,long); /*42eb30*/
void __cdecl bh_menu_action_tcpip(BHMenuActions *,long); /*42eea0*/
void __cdecl bh_menu_action_serial(BHMenuActions *,long); /*42f0c0*/
void __cdecl bh_menu_action_disconnect(BHMenuActions *,long); /*42f280*/
void __cdecl bh_menu_action_graphics_notice(BHMenuActions *,long); /*42f2b0*/
void __cdecl bh_menu_action_host(BHMenuActions *,long); /*42f360*/
void __cdecl bh_menu_action_join(BHMenuActions *,long); /*42f390*/
/* Callers supply real initialized owners, current/root menus, safe terminated
   original profile/provider text and writable private CFG path. Original rows
   may have absent refresh/action methods; non-NULL methods must be actual
   source owners. Native positive audio/network paths remain real platform
   calls; checks use absent endpoints/NULL peer and never create sessions.
   Errors retain already-performed original effects; no rollback is added. */
#endif
