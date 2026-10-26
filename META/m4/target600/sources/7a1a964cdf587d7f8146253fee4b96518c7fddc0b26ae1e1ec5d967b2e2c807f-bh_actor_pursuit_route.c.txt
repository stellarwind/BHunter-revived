#include "bh_actor_pursuit_route.h"
#include "bh_target_selection.h"
#include "bh_target_steering.h"
#include "bh_route_choice.h"
#include "bh_route_guided.h"
#include "bh_route_curve_length.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
static float stored(double d){float f;bh_round_float(&f,d);return f;}
static void read(BHEntity *e,unsigned long o,void *p,unsigned long n){memcpy(p,(char*)e+o,n);}
static void write(BHEntity *e,unsigned long o,const void *p,unsigned long n){memcpy((char*)e+o,p,n);}
static long value(BHEntity *e,unsigned long o){long v;read(e,o,&v,4);return v;}
static float number(BHEntity *e,unsigned long o){float f;read(e,o,&f,4);return f;}
static int fail(BHActorPursuitRouteContext *c,BHActorRouteError err){if(c&&c->route)c->route->error=err;return 0;}
static long elapsed32(long now,long before){unsigned long v=(unsigned long)now-(unsigned long)before;long result;memcpy(&result,&v,4);return result;}
typedef struct TargetMotion{long event;BHVec3 position,velocity,acceleration;}TargetMotion;
static int query(BHEntity *target,TargetMotion *p){if(!target||!target->query)return 0;p->event=16;target->query(target,p);return 1;}
static int reset_duration(BHActorPursuitRouteContext *c,BHEntity *e,BHActorRouteState *s){double ticks;float length=bh_route_curve_length(s->controls);s->phase=0;s->duration=stored(((double)length+length)/((double)s->speed*s->speed_scale));ticks=(double)s->duration*30;if(!_finite(ticks)||ticks< -9223372036854775808.0||ticks>=9223372036854775808.0)return fail(c,BH_ACTOR_ROUTE_NUMERICAL);s->duration_ticks=(unsigned short)(__int64)ticks;s->tick=c->route->tick;return bh_actor_route_write(e,s);}
int __cdecl bh_actor_pursuit_route(BHActorPursuitRouteContext *c,BHEntity *e){
 BHActorRouteState s;BHWorldGeometry *room,*target_world=0,*preferred=0;BHEntity *target;BHTargetSelection selected;TargetMotion motion;BHVec3 saved_end,saved_tangent;
 long incoming=-1,outgoing,index,target_index,junction,old_city,new_city,forced;unsigned long i,first;float pursuit;double factor;char text[256];
 static const char *const report[4]={"1Attention,\ncrime fighters!\n\nA fugitive car\nhas been spotted\nentering\n%s","1Attention!\n\nWe have reports\non a fugitive car\nentering\n%s","1Calling all\nHunters!\n\nA fugitive car\njust entered\n%s","1Incoming\nSurveillance\nReport:\n\nA fugitive car\njust passed through\na gate at\n%s"};
 if(!c||!c->route||!c->navigation||!e||!bh_math_environment_supported())return fail(c,BH_ACTOR_ROUTE_ARGUMENT);bh_actor_route_read(e,&s);room=s.next;target=(BHEntity*)value(e,0x1b8);
 if(s.sound_frames){--s.sound_frames;write(e,0x108,&s.sound_frames,4);if(!s.sound_frames&&s.sound!=BH_SOUND_INVALID){if(!c->route->sound)return fail(c,BH_ACTOR_ROUTE_ARGUMENT);bh_sound_stop(c->route->sound,s.sound);s.sound=BH_SOUND_INVALID;write(e,0x104,&s.sound,4);}}
 if(value(e,0xfc)==1&&(!target||rand()<0x1000)){if(!bh_target_select(c->player,c->actor_head,e,s.controls+3,c->actor_limit,&selected))return fail(c,BH_ACTOR_ROUTE_ARGUMENT);target=selected.target;write(e,0x1b8,&target,4);}
 forced=value(e,0x1d4);
 if(forced){
  s.controls[0]=s.controls[3];for(i=0;i<3;++i)((float*)&s.controls[1])[i]=stored(((double)((float*)&s.controls[3])[i]-((float*)&s.controls[2])[i])+((float*)&s.controls[0])[i]);
  bh_world_clamp_point(s.current,s.controls);bh_world_clamp_point(s.current,s.controls+1);read(e,0x1bc,s.controls+3,12);read(e,0x1c8,s.controls+2,12);
  if(!reset_duration(c,e,&s))return 0;forced=0;write(e,0x1d4,&forced,4);c->route->error=BH_ACTOR_ROUTE_OK;return 1;
 }
 if(!room||!room->model||room->navigation.count!=room->model->portal_count||room->navigation.count>6)return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);
 index=bh_navigation_find(c->navigation,room);if(index<0)return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);
 if(value(e,0xfc)==6&&c->city_ids){new_city=c->city_ids[index];old_city=value(e,0x110);if(new_city!=-1&&new_city!=old_city){write(e,0x110,&new_city,4);if(old_city!=-1&&elapsed32(c->clock,c->last_report)>1800){if(new_city<0||(unsigned long)new_city>=c->city_count||!c->city_names||!c->messages||!c->city_names[new_city]||strlen(c->city_names[new_city])>160)return fail(c,BH_ACTOR_ROUTE_ARGUMENT);first=(unsigned long)rand()&3;c->last_report=c->clock;sprintf(text,report[first],c->city_names[new_city]);bh_message_enqueue(c->messages,text);}}}
 for(i=0;i<room->navigation.count;++i)if((BHWorldGeometry*)room->navigation.portals[i].neighbor==s.current||incoming<0)incoming=(long)i;
 pursuit=number(e,0x1b4);
 if(room->query_flags&8){if(!bh_route_guided_exit(e,room,incoming,&outgoing))return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);}
 else if(!target||(unsigned long)index>=c->navigation->junction_count){if(!bh_route_choose_exit(e,room,0,incoming,&outgoing))return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);}
 else {
  read(target,0xc4,&target_world,4);
  if(rand()>0xccc){
   if(target_world)for(i=0;i<room->navigation.count&&!preferred;++i)if((pursuit>=.5||(long)i!=incoming)&&(BHWorldGeometry*)room->navigation.portals[i].neighbor==target_world)preferred=target_world;
   if(!preferred){target_index=target_world?bh_navigation_find(c->navigation,target_world):index;if(target_index<0)return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);
    if((unsigned long)target_index>=c->navigation->junction_count){const BHNavigationNode *node=c->navigation->nodes+target_index;junction=node->junction_first_port;if(junction==index)junction=node->junction_second_port;else if(node->junction_second_port!=index){if(rand()<0x4000&&node->junction_second_port>=0)junction=node->junction_second_port;else junction=node->junction_first_port;}if(junction<0)junction=index;target_index=junction;}
    if((unsigned long)target_index>=c->navigation->junction_count)return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);
    if(!c->navigation->routes.cells||c->navigation->routes.node_count!=c->navigation->junction_count)return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);
    outgoing=c->navigation->routes.cells[target_index*c->navigation->junction_count+index].exit_slot;
    if(outgoing>=0&&(pursuit>=.5||outgoing!=incoming)&&(unsigned long)outgoing<room->navigation.count)preferred=(BHWorldGeometry*)room->navigation.portals[outgoing].neighbor;
   }
  }
  if(!bh_route_choose_exit(e,room,preferred,incoming,&outgoing))return fail(c,BH_ACTOR_ROUTE_TOPOLOGY);
 }
 /* CRT random bias here is distinct from the actor-local generator used by route selection. */
 factor=1.0-s.jitter;s.u=stored(factor*s.u-((double)s.jitter*rand())*(-1.0/32768.0));s.v=stored(factor*s.v-((double)rand()*s.jitter)*(-1.0/32768.0));write(e,0x18c,&s.u,4);write(e,0x190,&s.v,4);
 if(!bh_actor_route_promote(c->route,e,incoming,outgoing))return 0;bh_actor_route_read(e,&s);
 if(target){short protected_player=0;read(target,0xc4,&target_world,4);if(target==c->player)read(target,0x2b4,&protected_player,2);
  if(target_world==s.current&&(!protected_player||target!=c->player)){
   for(i=0;i<3;++i){((float*)&s.controls[1])[i]=stored((double)((float*)&s.controls[1])[i]*.4-((float*)&s.controls[0])[i]*(-.6));((float*)&s.controls[2])[i]=stored((double)((float*)&s.controls[2])[i]*.4-((float*)&s.controls[3])[i]*(-.6));}
   if(!query(target,&motion))return fail(c,BH_ACTOR_ROUTE_ARGUMENT);saved_end=s.controls[3];saved_tangent=s.controls[2];write(e,0x1bc,&saved_end,12);write(e,0x1c8,&saved_tangent,12);
   for(i=0;i<3;++i){float predicted=stored((double)((float*)&motion.position)[i]-(((double)((float*)&motion.velocity)[i]-((float*)&motion.acceleration)[i]*(-1.5))*(-1.5)));((float*)&s.controls[3])[i]=predicted;((float*)&s.controls[2])[i]=stored((double)predicted-(((double)((float*)&s.controls[0])[i]-((float*)&saved_end)[i])*(-.25)));}
   bh_world_clamp_point(s.current,s.controls+3);bh_world_clamp_point(s.current,s.controls+2);if(!reset_duration(c,e,&s))return 0;forced=1;write(e,0x1d4,&forced,4);
  }
 }
 c->route->error=BH_ACTOR_ROUTE_OK;return 1;
}
