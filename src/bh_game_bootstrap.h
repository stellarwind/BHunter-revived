#ifndef BH_GAME_BOOTSTRAP_H
#define BH_GAME_BOOTSTRAP_H
#include "bh_player_create.h"
#include "bh_collision_resource.h"
#include "bh_script_blocks.h"
#include "bh_world_load.h"
#include "bh_editor_input.h"
#include "bh_flyby_construct.h"
#include "bh_mission_story.h"
#include "bh_menu_object.h"
#include "bh_sound_load.h"
#include "bh_provider_startup.h"
#include "bh_render_material.h"
#define BH_BOOTSTRAP_MENUS 18
#define BH_BOOTSTRAP_ROWS 32
#define BH_BOOT_PLAYER_NAME ((const char*)1)
typedef struct BHBootstrapBinding {unsigned long identity;unsigned long *destination;} BHBootstrapBinding;
typedef struct BHBootstrapValue {unsigned long identity,value,kind;} BHBootstrapValue;
typedef struct BHGameBootstrap BHGameBootstrap;
typedef struct BHBootstrapMenu {
 BHMenuActions actions;BHMenuEdits edits;BHGameBootstrap *owner;
 BHMenuObject object;BHMenuRuntime runtime;BHMenuActionsRoot root;
 BHMenuSettingsMenu settings;BHMenuPresent present;BHMenuPresentRow present_rows[BH_BOOTSTRAP_ROWS];
 BHMenuRow *rows[BH_BOOTSTRAP_ROWS];BHMenuSettingsRow settings_rows[BH_BOOTSTRAP_ROWS];
 BHMenuSettingsRow *row_pointers[BH_BOOTSTRAP_ROWS];BHMenuAction action[BH_BOOTSTRAP_ROWS];
 BHMenuMissionAction edit_action[BH_BOOTSTRAP_ROWS];BHMenuRuntime *destinations[BH_BOOTSTRAP_ROWS];
 unsigned long refresh[BH_BOOTSTRAP_ROWS],identity;
} BHBootstrapMenu;
struct BHGameBootstrap {
 BHCollisionResourcePath collision_path;
 BHTextureContext *textures;BHRenderResources *resources;BHSoundLoad *sounds;
 BHAtlasContext *atlas;BHTextQueue *text;BHScriptBlocks *blocks;BHWorldLoad *world;
 BHEditorInput *editor;BHNavigation *navigation;BHPlayerCreate *player;
 BHMissionStory *story;unsigned long (*availability)[32];
 BHPlayerInputOwner *inputs;BHEntityPool *pool;BHInputSnapshot *snapshot;
 BHConfig *config;BHProviderCatalog *providers;BHModemNames *modems;
 BHFlybyConstruct *flyby;BHMenuRuntimeContext *runtime;BHMenuActions *actions;BHMenuEdits *edits;
 BHMessagePanel *panel;BHMenuTransition *transition;BHMenuPresent *present;
 /* Count-zero composition phases:1world,2player,3menus,4all resources,
    5font pages ready before flyby construction.
    Callback must initialize actual source registries, never supply substitutes. */
 int (__cdecl *compose)(BHGameBootstrap *,unsigned long,void *);void *compose_user;
 const BHBootstrapBinding *bindings;unsigned long binding_count;
 BHBootstrapValue values[256];unsigned long value_count;
 BHBootstrapMenu menus[BH_BOOTSTRAP_MENUS];
 BHAtlasPage *confirmation,*menu_font,*graffiti;BHPlayerInputList *all_keys;
 float remote_age[8];char computer_name[256],player_name[21];
 unsigned long menus_created,rows_created,resources_loaded,aliases,profile_copies;
 int error;
 int diagnostic_box; /* count-zero isolated level boot; omit flyby/fades */
};
/* Whole004474c0. Declarative literal data retains all original resource loads,
   aliases, sound arguments, eighteen menus and115static rows. Dynamic provider
   rows retain original switch/destinations. All calls use real source owners;
   original identities are data lookup keys, never callable/mapped addresses.
   Context/prototypes and owned menu storage survive every installed callback.
   No tick, rendering drain, native peer or device input is executed here.
   Sources bound before call may still invoke platform/resource constructors;
   synthetic checks use controlled source resource failures. A successful full
   asset/native bootstrap is an explicit outstanding integration question. */
int __cdecl bh_game_bootstrap(BHGameBootstrap *);
unsigned long __cdecl bh_bootstrap_value(const BHGameBootstrap *,unsigned long identity);
int __cdecl bh_bootstrap_menus(BHGameBootstrap *); /* count-zero family composition */
int __cdecl bh_bootstrap_resources(BHGameBootstrap *,unsigned long stage);
#endif

