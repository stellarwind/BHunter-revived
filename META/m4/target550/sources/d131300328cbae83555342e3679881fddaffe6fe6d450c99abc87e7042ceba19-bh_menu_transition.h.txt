#ifndef BH_MENU_TRANSITION_H
#define BH_MENU_TRANSITION_H
#include "bh_media_control.h"
#include "bh_messages.h"
#include "bh_fade.h"
#include "bh_config.h"
typedef struct BHMenuTransition {
 BHEntityPool *pool;BHDrawQueue *draw;BHMediaControl *media;BHMessageState *messages;
 BHConfig *config;const char *config_path;unsigned short hardware_flags;
 BHEntity *music;long cd_ready,range_ready;DWORD window;float saved_volume;long *camera_index,*quit;
 unsigned long warnings,fades,releases,saves;int error;
} BHMenuTransition;
/* Count-zero source binding, actual pool and all borrowed owners outlive entity.
   time is original+1f8 double, attempts+1c0. Known CRT empty-RET sentinel
   is represented by a disabled NULL callback, never counted as game code. */
int __cdecl bh_menu_transition_bind(BHMenuTransition *,BHEntity *,double,long);
double __cdecl bh_menu_transition_time(const BHEntity *);
void __cdecl bh_menu_music_fade(BHEntity *); /*42cb40*/
void __cdecl bh_menu_music_tick(BHEntity *); /*42cbb0*/
void __cdecl bh_menu_fade_expiry(BHEntity *); /*42cec0*/
void __cdecl bh_menu_quit_fade(BHEntity *); /*42cf00*/
void __cdecl bh_menu_quit_tick(BHEntity *); /*42cfb0*/
BHEntity *__cdecl bh_menu_quit_begin(BHMenuTransition *); /*42d070*/
#endif
