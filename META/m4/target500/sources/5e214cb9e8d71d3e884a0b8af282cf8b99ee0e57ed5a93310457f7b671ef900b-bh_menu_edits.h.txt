#ifndef BH_MENU_EDITS_H
#define BH_MENU_EDITS_H
#include "bh_cd_audio.h"
#include "bh_input_mode.h"
#include "bh_menu_row.h"
#include "bh_network_sessions.h"
typedef struct BHMenuEdits BHMenuEdits;
typedef void(__cdecl *BHMenuMissionAction)(BHMenuEdits *, long);
typedef struct BHMenuMissionRow {
  BHMenuMissionAction action;
} BHMenuMissionRow;
/* Borrowed actual config/profile/network/rows/input state. Original global
   config base496a38 maps directly to config.settings. No physical collection
   occurs: capture actions consume only the cached real input state. The9
   mission rows carry actual source callback identities or disabled NULL. */
struct BHMenuEdits {
  BHConfig *config;
  BHMenuSettings *settings;
  BHMenuRows *rows;
  BHCdAudio *audio;
  BHNetworkSessions *sessions;
  BHInputKeyCapture *capture;
  BHMenuSettingsMenu *menu, *mission_menu;
  BHMenuMissionRow *mission_rows[9];
  long *mode, *mission, *ship;
  unsigned long refreshes;
  int error;
};
void __cdecl bh_menu_edit_video(BHMenuEdits *, long);       /*42d1b0*/
void __cdecl bh_menu_edit_audio(BHMenuEdits *, long);       /*42d290*/
void __cdecl bh_menu_edit_profile(BHMenuEdits *, long);     /*42e300*/
void __cdecl bh_menu_edit_motion(BHMenuEdits *, long);      /*42d380*/
void __cdecl bh_menu_edit_primary(BHMenuEdits *, long);     /*42d550*/
void __cdecl bh_menu_edit_secondary(BHMenuEdits *, long);   /*42d620*/
void __cdecl bh_menu_edit_camera(BHMenuEdits *, long);      /*42d6f0*/
void __cdecl bh_menu_edit_sessions(BHMenuEdits *, long);    /*42d920*/
void __cdecl bh_menu_edit_modem(BHMenuEdits *, long);       /*42d990*/
void __cdecl bh_menu_edit_tcpip(BHMenuEdits *, long);       /*42da40*/
void __cdecl bh_menu_edit_serial(BHMenuEdits *, long);      /*42daa0*/
void __cdecl bh_menu_action_motion(BHMenuEdits *, long);    /*42e410*/
void __cdecl bh_menu_action_primary(BHMenuEdits *, long);   /*42e640*/
void __cdecl bh_menu_action_secondary(BHMenuEdits *, long); /*42e790*/
void __cdecl bh_menu_action_camera(BHMenuEdits *, long);    /*42e8f0*/
void __cdecl bh_menu_action_mission(BHMenuEdits *, long);   /*42ca10*/
void __cdecl bh_menu_action_ship(BHMenuEdits *, long);      /*42ca30*/
/* Count-zero row callback bridge for the current actual menu. Install as
   BHMenuSettingsRow.refresh and set BHMenuSettingsMenu.user=context; dispatch
   mode identifies one of the11 actual refresh owners. */
typedef enum BHMenuEditKind {
  BH_MENU_EDIT_VIDEO,
  BH_MENU_EDIT_AUDIO,
  BH_MENU_EDIT_PROFILE,
  BH_MENU_EDIT_MOTION,
  BH_MENU_EDIT_PRIMARY,
  BH_MENU_EDIT_SECONDARY,
  BH_MENU_EDIT_CAMERA,
  BH_MENU_EDIT_SESSIONS,
  BH_MENU_EDIT_MODEM,
  BH_MENU_EDIT_TCPIP,
  BH_MENU_EDIT_SERIAL
} BHMenuEditKind;
typedef struct BHMenuEditBridge {
  BHMenuEdits *context;
  BHMenuEditKind kind;
} BHMenuEditBridge;
void __cdecl bh_menu_edit_row_callback(BHMenuSettingsMenu *, long);
#endif
