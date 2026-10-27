#include "bh_mission_story.h"
#include <string.h>
typedef enum StoryAction {STORY_MESSAGE,STORY_COMPLETE,STORY_UNLOCK_1,STORY_UNLOCK_2,STORY_UNLOCK_3,STORY_UNLOCK_4,STORY_UNLOCK_5,STORY_UNLOCK_6,STORY_PROGRESS_ZERO,STORY_PROGRESS_SEVEN,STORY_PROGRESS_EIGHT,STORY_CLEAR,STORY_END_ONE,STORY_FINALE} StoryAction;
typedef struct StoryMessage {long id;StoryAction action;const char *text;} StoryMessage;
#include "bh_mission_story_data.h"
static void setword(BHEntity *e,unsigned long offset,unsigned long v){memcpy((char*)e+offset,&v,4);}
static void cue(BHMissionStory *c,long progression){if(!c->progress){c->error=1;return;}++c->cues;bh_mission_progress(c->progress,progression);if(c->progress->error)c->error=3;}
static void clear(BHMissionStory *c){unsigned long visited;if(!c->player||!c->actors||!c->actor_limit){c->error=1;return;}setword(c->player,0x238,0);setword(c->player,0x234,0);++c->broadcasts;if(!bh_player_input_broadcast(*c->actors,c->actor_limit,&visited))c->error=4;}
static void enable(BHMissionStory *c,unsigned long row,unsigned long index){if(!c->availability[row][index]){c->availability[row][index]=1;++c->availability[row][31];}}
int __cdecl bh_mission_story_dispatch(BHMissionStory *c,const long packet[3]){const StoryMessage *m=0;unsigned long i;BHEntity *weapon,*actual;BHWeaponObjectState *s;unsigned long handle;
 if(!c||!packet){if(c)c->error=1;return 0;}c->error=0;++c->calls;if(packet[0]!=14)return 1;
 for(i=0;i<sizeof(messages)/sizeof(messages[0]);++i)if(messages[i].id==packet[2]){m=messages+i;break;}
 if(m){if(!c->messages){c->error=1;return 0;}++c->queued;if(!bh_message_enqueue(c->messages,m->text))c->error=2;
  switch(m->action){case STORY_MESSAGE:break;case STORY_COMPLETE:case STORY_PROGRESS_ZERO:cue(c,0);break;
  case STORY_UNLOCK_1:case STORY_UNLOCK_2:case STORY_UNLOCK_3:case STORY_UNLOCK_4:case STORY_UNLOCK_5:case STORY_UNLOCK_6:cue(c,(long)m->action-STORY_UNLOCK_1+1);break;
  case STORY_PROGRESS_SEVEN:cue(c,7);break;case STORY_PROGRESS_EIGHT:cue(c,8);break;case STORY_CLEAR:clear(c);break;
  case STORY_END_ONE:clear(c);cue(c,0);if(c->ending)*c->ending=1;else c->error=1;break;
  case STORY_FINALE:clear(c);if(!c->player||!c->camera||c->camera!=(BHCameraState*)((char*)c->player+0xe8)||!c->force_control||!c->force_gate){c->error=1;break;}if(!bh_camera_state_set_mode(c->camera,8))c->error=5;*c->force_control=bh_player_force_control_a;*c->force_gate=0;break;
  }return !c->error;
 }
 switch(packet[2]){case 0x960:if(!c->ending){c->error=1;break;}*c->ending=2;break;case 3000:cue(c,0);break;
 case 3001:case 3002:case 3003:case 3004:case 3005:
  if(!c->availability){c->error=1;break;}switch(packet[2]){
   case 3001:enable(c,0,0);enable(c,0,10);enable(c,1,10);enable(c,1,3);enable(c,2,10);enable(c,2,3);enable(c,3,5);break;
   case 3002:enable(c,0,3);enable(c,1,11);enable(c,1,5);enable(c,2,11);enable(c,2,5);enable(c,3,7);break;
   case 3003:enable(c,0,1);enable(c,1,7);enable(c,2,6);break;
   case 3004:enable(c,0,2);enable(c,1,12);enable(c,2,12);enable(c,3,12);enable(c,3,8);break;
   case 3005:enable(c,0,6);break;
  }break;
 case 3006:
  if(!c->sound){c->error=1;return 0;}handle=bh_sound_request(c->sound,c->pickup_sound,1,.7f,0,0);if(handle==BH_SOUND_INVALID)c->error=6;
  if(!c->player||!c->weapons){c->error=1;break;}for(i=5;i<10;++i){if(!bh_weapon_registry_resolve(c->weapons,i,&weapon,&s)){c->error=1;break;}memcpy(&actual,(char*)c->player+0x258+i*4,4);if(actual!=weapon){c->error=1;break;}s->common.ammo=0;if(s->common.ammo_limit<0)s->common.ammo=s->common.ammo_limit;}break;
 }
 return !c->error;
}
void __cdecl bh_mission_story_query(BHEntity *e,void *packet){BHMissionStory *c;memcpy(&c,e->payload+548,4);if(c)bh_mission_story_dispatch(c,(const long*)packet);}
int __cdecl bh_mission_story_bind(BHMissionStory *c,BHEntity *e){BHEntity *at;BHEntityPool *pool;unsigned long n=0;if(!c||!e||!c->progress||!c->progress->lifetime||!(pool=c->progress->lifetime->timers.pool))return 0;for(at=pool->active;at&&n++<pool->capacity;at=at->next)if(at==e){memcpy(e->payload+548,&c,4);return bh_entity_set_query(pool,e,bh_mission_story_query);}return 0;}
