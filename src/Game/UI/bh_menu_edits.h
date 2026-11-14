#ifndef BH_MENU_EDITS_H
#define BH_MENU_EDITS_H
#include "../../Audio/bh_cd_audio.h"
#include "../../Input/bh_input_mode.h"
#include "bh_menu_row.h"
typedef struct BHMenuEdits BHMenuEdits;
typedef void (*BHMenuMissionAction)(BHMenuEdits *, long);
typedef struct BHMenuMissionRow
{
    BHMenuMissionAction action;
} BHMenuMissionRow;
/* Borrowed actual config/profile/rows/input state. Original global
   config base496a38 maps directly to config.settings. No physical collection
   occurs: capture actions consume only the cached real input state. The9
   mission rows carry actual source callback identities or disabled NULL. */
struct BHMenuEdits
{
    BHConfig *config;
    BHMenuSettings *settings;
    BHMenuRows *rows;
    BHCdAudio *audio;
    BHInputKeyCapture *capture;
    BHMenuSettingsMenu *menu, *mission_menu;
    BHMenuMissionRow *mission_rows[9];
    long *mode, *mission, *ship;
    unsigned long refreshes;
    int error, unlock_all_levels;
};
void bh_menu_edit_video(BHMenuEdits *, long);       /*42d1b0*/
void bh_menu_edit_audio(BHMenuEdits *, long);       /*42d290*/
void bh_menu_edit_profile(BHMenuEdits *, long);     /*42e300*/
void bh_menu_edit_motion(BHMenuEdits *, long);      /*42d380*/
void bh_menu_edit_primary(BHMenuEdits *, long);     /*42d550*/
void bh_menu_edit_secondary(BHMenuEdits *, long);   /*42d620*/
void bh_menu_edit_camera(BHMenuEdits *, long);      /*42d6f0*/
void bh_menu_action_motion(BHMenuEdits *, long);    /*42e410*/
void bh_menu_action_primary(BHMenuEdits *, long);   /*42e640*/
void bh_menu_action_secondary(BHMenuEdits *, long); /*42e790*/
void bh_menu_action_camera(BHMenuEdits *, long);    /*42e8f0*/
void bh_menu_action_mission(BHMenuEdits *, long);   /*42ca10*/
void bh_menu_action_ship(BHMenuEdits *, long);      /*42ca30*/
/* Count-zero row callback bridge for the current actual menu. Install as
   BHMenuSettingsRow.refresh and set BHMenuSettingsMenu.user=context; dispatch
   mode identifies one of the seven refresh owners. */
typedef enum BHMenuEditKind
{
    BH_MENU_EDIT_VIDEO,
    BH_MENU_EDIT_AUDIO,
    BH_MENU_EDIT_PROFILE,
    BH_MENU_EDIT_MOTION,
    BH_MENU_EDIT_PRIMARY,
    BH_MENU_EDIT_SECONDARY,
    BH_MENU_EDIT_CAMERA
} BHMenuEditKind;
typedef struct BHMenuEditBridge
{
    BHMenuEdits *context;
    BHMenuEditKind kind;
} BHMenuEditBridge;
void bh_menu_edit_row_callback(BHMenuSettingsMenu *, long);
#endif
