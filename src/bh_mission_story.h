#ifndef BH_MISSION_STORY_H
#define BH_MISSION_STORY_H
#include "bh_mission_progress.h"
#include "bh_messages.h"
#include "bh_player_input_events.h"
#include "bh_player_force_control.h"
#include "bh_camera_state.h"
#include "bh_weapon_factory.h"
typedef int (__cdecl *BHStoryForceControl)(BHPlayerForceControl *,float,int);
typedef struct BHMissionStory {
 BHMissionProgressContext *progress;BHMessageState *messages;BHEntity *player;
 BHWeaponRegistry *weapons;BHEntity **actors;unsigned long actor_limit;
 unsigned long (*availability)[32];BHSoundQueue *sound;unsigned long pickup_sound;
 BHCameraState *camera;BHStoryForceControl *force_control;long *force_gate,*ending;
 unsigned long calls,queued,broadcasts,cues;int error;
} BHMissionStory;
/* Whole004225f0: event14's third signed word routes95 exact original messages,
   progression/save/cues, player-state broadcasts, finale camera/control switch,
   availability rows and five weapon refill effects. Borrowed live owners only;
   ring/arena, configuration and sound failures retain earlier effects and do
   not suppress subsequent original actions. Tables are fixed4x32 original loot
   rows; counter updates wrap32 and count once per previously zero slot.
   Source rejects absent bindings at their branch of use. Camera is the actual
   player+e8 subobject; weapons5..9 must equal actual player table entries.
   No gameplay/input execution or original-backed callbacks. */
int __cdecl bh_mission_story_dispatch(BHMissionStory *,const long packet[3]);
int __cdecl bh_mission_story_bind(BHMissionStory *,BHEntity *);
void __cdecl bh_mission_story_query(BHEntity *,void *);
#endif
