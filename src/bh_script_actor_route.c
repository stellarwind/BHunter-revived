#include "bh_script_actor_route.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <float.h>
static float rounded(double v){float f;bh_round_float(&f,v);return f;}
static BHEntity *link(const BHEntity *a,unsigned long off){BHEntity *p;memcpy(&p,(const char*)a+off,4);return p;}
static void setlink(BHEntity *a,unsigned long off,BHEntity *p){memcpy((char*)a+off,&p,4);}
static BHPickupRoom *binding(BHActorRouteContext *c,BHWorldGeometry *w){unsigned long i;for(i=0;i<c->room_count;++i)if(c->rooms[i].world==w)return c->rooms+i;return 0;}
BHScriptActorPath __cdecl bh_script_actor_route_path(const BHEntity *a){BHScriptActorPath p;memcpy(&p,(const char*)a+0x1a0,4);return p;}
static int error(BHActorRouteContext *c,BHActorRouteError e){if(c)c->error=e;return 0;}
int __cdecl bh_script_actor_route_advance(BHActorRouteContext *c,BHEntity *a){
 BHActorRouteState s;BHPickupRoom *old,*room;BHWorldGeometry *w;BHEntity *next,*prev,*at;
 BHVec3 start,end,n1,n2,side,delta,up={0,1,0};unsigned long i,n,count=0;long incoming=-1,outgoing=-1;
 double best=9e37,test,vertical,lateral,dot1,dot2,duration,length;BHScriptActorPath path;
 if(!c||!a||!c->rooms||!c->room_count||!c->actor_limit)return error(c,BH_ACTOR_ROUTE_ARGUMENT);
 if(!bh_math_environment_supported())return error(c,BH_ACTOR_ROUTE_NUMERICAL);
 bh_actor_route_read(a,&s);old=binding(c,s.current);room=binding(c,s.next);w=s.next;
 if(!old||!room||!w->model||w->navigation.count!=w->model->portal_count||w->navigation.count>6)return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
 next=link(a,0xf4);prev=link(a,0xf8);
 if((prev?link(prev,0xf4)!=a:old->actors!=a)||(next&&link(next,0xf8)!=a))return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
 at=old->actors;for(n=0;at&&at!=a&&n<c->actor_limit;++n)at=link(at,0xf4);
 if(at!=a||n>=c->actor_limit)return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
 for(i=0;i<w->navigation.count;++i)if(w->navigation.portals[i].neighbor){
  ++count;if(incoming<0)incoming=(long)i;if(outgoing<0)outgoing=(long)i;
  if((BHWorldGeometry*)w->navigation.portals[i].neighbor==s.current)incoming=(long)i;else outgoing=(long)i;
 }
 if(incoming<0||outgoing<0||!_finite(s.speed)||!_finite(s.speed_scale)||s.speed<=0||s.speed_scale<=0)return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
 if(!bh_portal_center(w->navigation.portals+incoming,&start)||!bh_portal_normal(w->navigation.portals+incoming,&n1))return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
 bh_vec3_cross(&up,&n1,&side);
 if(count>2){outgoing=incoming;
  for(i=0;i<w->navigation.count;++i){const BHPortal *p=w->navigation.portals+i;
   if((long)i==incoming||!p->neighbor||p->direction!=(w->navigation.portals[incoming].direction^4UL))continue;
   if(!bh_portal_center(p,&end))return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
   delta.x=rounded((double)end.x-start.x);delta.y=rounded((double)end.y-start.y);delta.z=rounded((double)end.z-start.z);
   vertical=delta.y;lateral=(double)delta.z*side.z+(double)delta.y*side.y+(double)delta.x*side.x;
   test=rounded(lateral*lateral+vertical*vertical);
   if(test<best){best=test;outgoing=(long)i;}
  }
 }
 if(!bh_portal_center(w->navigation.portals+outgoing,&end)||!bh_portal_normal(w->navigation.portals+outgoing,&n2)||!binding(c,(BHWorldGeometry*)w->navigation.portals[outgoing].neighbor))return error(c,BH_ACTOR_ROUTE_TOPOLOGY);
 delta.x=rounded((double)end.x-start.x);delta.y=rounded((double)end.y-start.y);delta.z=rounded((double)end.z-start.z);
 dot1=rounded((double)n1.z*delta.z+(double)n1.y*delta.y+(double)n1.x*delta.x);
 dot2=rounded(-((double)n2.z*delta.z+(double)n2.y*delta.y+(double)n2.x*delta.x));
 if(dot1==0)dot1=10;if(dot2==0)dot2=10;length=(dot1+dot2)*.75;
 if(w->navigation.portals[outgoing].direction==(w->navigation.portals[incoming].direction^4UL))path=bh_script_actor_linear;
 else{length=length*1.5707963267948966;dot1=dot1*.7;dot2=rounded(dot2*.7);path=bh_script_actor_curve;}
 s.controls[0]=start;s.controls[3]=end;s.portal_center=end;
 s.controls[1].x=rounded((double)n1.x*dot1+start.x);s.controls[1].y=rounded((double)n1.y*dot1+start.y);s.controls[1].z=rounded((double)n1.z*dot1+start.z);
 s.controls[2].x=rounded((double)n2.x*dot2+end.x);s.controls[2].y=rounded((double)n2.y*dot2+end.y);s.controls[2].z=rounded((double)n2.z*dot2+end.z);
 s.phase=0;s.duration=rounded((double)length/((double)s.speed*s.speed_scale));duration=(double)s.duration*30;
 if(!_finite(duration)||duration<0||duration>=9223372036854775808.0)return error(c,BH_ACTOR_ROUTE_NUMERICAL);
 s.duration_ticks=(unsigned short)(__int64)duration;s.tick=c->tick;
 if(s.sound_frames==1&&s.sound!=BH_SOUND_INVALID&&!c->sound)return error(c,BH_ACTOR_ROUTE_ARGUMENT);
 if(s.sound_frames){--s.sound_frames;if(!s.sound_frames&&s.sound!=BH_SOUND_INVALID){bh_sound_stop(c->sound,s.sound);s.sound=BH_SOUND_INVALID;}}
 if(prev)setlink(prev,0xf4,next);else old->actors=next;if(next)setlink(next,0xf8,prev);
 next=room->actors;setlink(a,0xf4,next);setlink(a,0xf8,0);if(next)setlink(next,0xf8,a);room->actors=a;
 s.current=w;s.next=(BHWorldGeometry*)w->navigation.portals[outgoing].neighbor;bh_actor_route_write(a,&s);memcpy((char*)a+0xc4,&w,4);memcpy((char*)a+0x1a0,&path,4);
 c->error=BH_ACTOR_ROUTE_OK;return 1;
}
