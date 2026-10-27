#include "../src/bh_script_actor_route.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
static BHEntity *linked(BHEntity *a,unsigned long off){BHEntity *p;memcpy(&p,(char*)a+off,4);return p;}
static void bindlink(BHEntity *a,unsigned long off,BHEntity *p){memcpy((char*)a+off,&p,4);}
static void fixture(BHWorldGeometry *w,BHGeometryModel *m,BHPickupRoom *r,BHActorRouteContext *c,BHEntity *a,unsigned long axis){
 unsigned long i;BHActorRouteState s;BHPortal *p;
 memset(w,0,sizeof(*w)*3);memset(m,0,sizeof(*m));memset(r,0,sizeof(*r)*3);memset(c,0,sizeof(*c));memset(a,0,sizeof(*a));memset(&s,0,sizeof(s));
 m->portal_count=2;for(i=0;i<3;++i){w[i].model=m;r[i].world=w+i;}
 w[1].navigation.count=2;p=w[1].navigation.portals;
 p[0].neighbor=&w[0].navigation;p[0].direction=axis;p[0].plane=0;
 p[1].neighbor=&w[2].navigation;p[1].direction=axis^4;p[1].plane=100;
 for(i=0;i<2;++i){p[i].scratch[0]=-10;p[i].scratch[1]=10;p[i].scratch[2]=-20;p[i].scratch[3]=20;}
 c->rooms=r;c->room_count=3;c->actor_limit=4;c->tick=65530;r[0].actors=a;
 s.current=w;s.next=w+1;s.speed=25;s.speed_scale=2;s.sound=BH_SOUND_INVALID;s.controls[3].x=-5;bh_actor_route_write(a,&s);
}
int bh_script_actor_route_checkpoint(unsigned long *count){
 BHWorldGeometry w[3];BHGeometryModel m;BHPickupRoom r[3];BHActorRouteContext c;BHActorRouteState s;
 BHEntity a,tail,head,before;BHMathEnvironment env;unsigned long axis;BHVec3 point;BHScriptActorPath path;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));
 for(axis=0;axis<3;++axis){fixture(w,&m,r,&c,&a,axis);
  CHECK(bh_script_actor_route_advance(&c,&a));CHECK(c.error==BH_ACTOR_ROUTE_OK);CHECK(!r[0].actors&&r[1].actors==&a);
  bh_actor_route_read(&a,&s);CHECK(s.current==w+1&&s.next==w+2);CHECK(s.duration==3&&s.duration_ticks==90&&s.tick==65530);
  path=bh_script_actor_route_path(&a);CHECK(path==bh_script_actor_linear);path(.5f,&point,s.controls,s.controls+1,s.controls+2,s.controls+3);
  CHECK(((float*)&point)[axis]==50);CHECK(linked(&a,0xf4)==0&&linked(&a,0xf8)==0);
 }
 fixture(w,&m,r,&c,&a,0);w[1].navigation.portals[1].direction=5;
 CHECK(bh_script_actor_route_advance(&c,&a));bh_actor_route_read(&a,&s);CHECK(bh_script_actor_route_path(&a)==bh_script_actor_curve);
 CHECK(s.duration>2&&s.duration<3);CHECK(s.controls[1].x==7&&s.controls[2].y==30);
 fixture(w,&m,r,&c,&a,0);m.portal_count=3;w[1].navigation.count=3;
 w[1].navigation.portals[2]=w[1].navigation.portals[1];w[1].navigation.portals[2].plane=200;
 w[1].navigation.portals[1].scratch[0]=0;w[1].navigation.portals[1].scratch[1]=20;
 CHECK(bh_script_actor_route_advance(&c,&a));bh_actor_route_read(&a,&s);
 CHECK(s.controls[3].x==200&&s.controls[3].y==0&&s.duration==6);
 fixture(w,&m,r,&c,&a,0);m.portal_count=3;w[1].navigation.count=3;
 w[1].navigation.portals[2]=w[1].navigation.portals[1];w[1].navigation.portals[2].plane=200;
 CHECK(bh_script_actor_route_advance(&c,&a));bh_actor_route_read(&a,&s);CHECK(s.controls[3].x==100);
 fixture(w,&m,r,&c,&a,0);memset(&head,0,sizeof(head));memset(&tail,0,sizeof(tail));r[0].actors=&head;
 bindlink(&head,0xf4,&a);bindlink(&a,0xf8,&head);bindlink(&a,0xf4,&tail);bindlink(&tail,0xf8,&a);
 CHECK(bh_script_actor_route_advance(&c,&a));CHECK(linked(&head,0xf4)==&tail&&linked(&tail,0xf8)==&head);
 fixture(w,&m,r,&c,&a,0);s.sound=123;s.sound_frames=1;bh_actor_route_read(&a,&s);s.sound=123;s.sound_frames=1;bh_actor_route_write(&a,&s);before=a;
 CHECK(!bh_script_actor_route_advance(&c,&a));CHECK(c.error==BH_ACTOR_ROUTE_ARGUMENT&&!memcmp(&a,&before,sizeof(a))&&r[0].actors==&a);
 fixture(w,&m,r,&c,&a,0);w[1].navigation.portals[0].neighbor=0;w[1].navigation.portals[1].neighbor=0;before=a;
 CHECK(!bh_script_actor_route_advance(&c,&a));CHECK(c.error==BH_ACTOR_ROUTE_TOPOLOGY&&!memcmp(&a,&before,sizeof(a)));
 bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"script-actor-route\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCRIPT_ACTOR_ROUTE_STANDALONE
int main(void){return bh_script_actor_route_checkpoint(0);}
#endif
