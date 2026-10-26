#include "../src/bh_script_actor_query.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
static void pointer(BHEntity *e,unsigned long at,BHEntity *v){memcpy((char*)e+at,&v,4);}
static BHEntity *getpointer(BHEntity *e,unsigned long at){BHEntity *v;memcpy(&v,(char*)e+at,4);return v;}
int bh_script_actor_query_checkpoint(unsigned long *count){
 BHEntityPool pool;BHActorRouteContext route;BHScriptActorContext context;BHActorRouteState s;BHPickupRoom room;BHWorldGeometry world;
 BHScriptActorPath path=bh_script_actor_linear;BHEntity *e,*target;BHScriptActorQueryPacket p,before;BHVec3 offset={1,2,3};BHMathEnvironment env;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));CHECK(bh_entities_init(&pool,2,0));e=bh_entity_acquire(&pool);target=bh_entity_acquire(&pool);CHECK(e&&target);
 memset(&route,0,sizeof(route));memset(&context,0,sizeof(context));memset(&s,0,sizeof(s));memset(&room,0,sizeof(room));memset(&world,0,sizeof(world));
 route.tick=15;route.rooms=&room;route.room_count=1;route.actor_limit=2;room.world=&world;room.actors=e;
 context.pool=&pool;context.route=&route;s.current=&world;s.duration_ticks=30;s.controls[1].x=100;s.sound=BH_SOUND_INVALID;bh_actor_route_write(e,&s);
 memcpy((char*)e+0x1a0,&path,4);memcpy((char*)e+0x15c,&offset,12);CHECK(bh_script_actor_bind(&context,e));CHECK(e->query==bh_script_actor_query);
 memset(&p,0xa5,sizeof(p));p.event=3;bh_script_actor_query(e,&p);CHECK(p.position.x==51&&p.position.y==4&&p.position.z==3);
 memset(&p,0,sizeof(p));p.event=16;bh_script_actor_query(e,&p);CHECK(p.position.x==51&&p.position.y==2&&p.position.z==3);CHECK(fabs(p.velocity.x-100)<.001&&p.velocity.y==0&&p.velocity.z==0);CHECK(fabs(p.acceleration.x)<.01&&p.acceleration.y==0&&p.acceleration.z==0);
 path=bh_script_actor_curve;s.controls[1].x=0;s.controls[3].x=100;bh_actor_route_write(e,&s);memcpy((char*)e+0x1a0,&path,4);
 p.event=16;bh_script_actor_query(e,&p);CHECK(p.velocity.x>80&&p.velocity.x<100&&p.acceleration.x< -200&&p.acceleration.x> -300);
 CHECK(p.position.x>21&&p.position.x<22&&p.position.y==2);
 p.event=99;before=p;bh_script_actor_query(e,&p);CHECK(!memcmp(&p,&before,sizeof(p)));
 pointer(e,0x1b8,target);pointer(e,0x1ac,target);p.event=17;memcpy((char*)&p+4,&target,4);bh_script_actor_query(e,&p);CHECK(!getpointer(e,0x1b8)&&!getpointer(e,0x1ac));
 pointer(e,0x1b8,target);pointer(e,0x1ac,target);p.event=17;memcpy((char*)&p+4,&e,4);bh_script_actor_query(e,&p);CHECK(getpointer(e,0x1b8)==target&&getpointer(e,0x1ac)==target);
 p.event=20;bh_script_actor_query(e,&p);CHECK(e->query==NULL&&e->refresh==NULL&&e->step==bh_script_actor_cleanup&&pool.used==2);
 e->step(e);CHECK(pool.used==1&&!room.actors);e=bh_entity_acquire(&pool);CHECK(e!=NULL);room.actors=e;bh_actor_route_write(e,&s);CHECK(bh_script_actor_bind(&context,e));p.event=4;bh_script_actor_query(e,&p);CHECK(pool.used==1&&!room.actors);
 CHECK(bh_entities_dispose(&pool));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"script-actor-query\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCRIPT_ACTOR_QUERY_STANDALONE
int main(void){return bh_script_actor_query_checkpoint(0);}
#endif
