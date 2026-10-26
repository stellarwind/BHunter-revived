#include "../src/bh_script_actor_construct.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure line %d\n",__LINE__);}}while(0)
int bh_script_actor_construct_checkpoint(unsigned long *count){
 BHScriptActorConstructorContext c;BHActorRouteContext route;BHScriptActorDrawContext draw;BHScriptParticleContext particles;
 BHEntityPool pool;BHDrawQueue queue;BHWorldGeometry w[2];BHGeometryModel model;BHPickupRoom rooms[2];BHActorRouteState s;
 BHEntity *e;BHVec3 point={0,0,0},outside={1000,0,0};BHScriptActorQueryPacket p;BHMathEnvironment env;BHSoundQueue sound;BHSoundResource *resource;
 unsigned long i,type,draw_seed,expected;float offsets[3];
 checks=failures=0;CHECK(bh_math_environment_enter(&env));CHECK(bh_entities_init(&pool,8,0));CHECK(bh_draw_queue_init(&queue,64,0));CHECK(bh_script_particles_init(&particles,&pool,&queue));CHECK(bh_sound_queue_init(&sound,0,0,0));resource=bh_sound_resource_allocate(&sound);CHECK(resource!=NULL);
 memset(&c,0,sizeof(c));memset(&route,0,sizeof(route));memset(&draw,0,sizeof(draw));memset(w,0,sizeof(w));memset(&model,0,sizeof(model));memset(rooms,0,sizeof(rooms));
 model.portal_count=2;for(i=0;i<2;++i){w[i].model=&model;w[i].navigation.next=&w[1-i].navigation;w[i].navigation.count=2;rooms[i].world=w+i;w[i].minimum.x=i?200:-100;w[i].maximum.x=i?400:100;w[i].minimum.y=w[i].minimum.z=-100;w[i].maximum.y=w[i].maximum.z=100;
  w[i].navigation.portals[0].neighbor=&w[1-i].navigation;w[i].navigation.portals[0].direction=0;w[i].navigation.portals[0].plane=-100;
  w[i].navigation.portals[1].neighbor=&w[1-i].navigation;w[i].navigation.portals[1].direction=4;w[i].navigation.portals[1].plane=100;}
 if(resource){resource->streaming=1;resource->base_frequency=8000;}
 route.rooms=rooms;route.room_count=2;route.actor_limit=16;route.sound=&sound;draw.queue=&queue;draw.particles=&particles;draw.glowing_model=9999;
 c.callbacks.pool=&pool;c.callbacks.route=&route;c.callbacks.draw=&draw;c.first_world=w;c.world_count=2;c.sound_resource=resource?resource->index:BH_SOUND_INVALID;c.callbacks.pulse_resource=c.sound_resource;
 for(i=0;i<4;++i)c.model_resources[i]=100+i;
 for(draw_seed=0;draw_seed<65536;++draw_seed){srand(draw_seed);if(rand()>=0x800)break;}
 srand(12);expected=rand();srand(12);CHECK(!bh_script_actor_construct(&c,&outside,0));CHECK(rand()==expected&&pool.used==0);
 for(type=0;type<4;++type){srand(12);rand();rand();expected=rand();srand(12);e=bh_script_actor_construct(&c,&point,type);CHECK(e!=NULL);CHECK(rand()==expected);
  if(e){bh_actor_route_read(e,&s);CHECK(s.current==w&&s.next==w+1&&rooms[0].actors==e&&pool.used==1);CHECK(s.speed==80&&s.u==.5f&&s.v==.5f&&s.phase==0&&s.duration_ticks==112);
   CHECK(e->query==bh_script_actor_query&&e->step!=NULL&&e->flags&0x100);memcpy(offsets,(char*)e+0x174,12);CHECK(type<2?(offsets[0]==5&&offsets[1]==5&&offsets[2]==10):(!offsets[0]&&!offsets[1]&&!offsets[2]));
   memset(&p,0,sizeof(p));p.event=3;e->query(e,&p);CHECK(p.position.x== -100&&p.position.y==2&&p.position.z==0);
   srand(draw_seed);bh_script_actor_present(e);CHECK(queue.commands[queue.count-1].resource==100+type);e->step(e);CHECK(route.error==BH_ACTOR_ROUTE_OK);
   p.event=4;e->query(e,&p);CHECK(pool.used==0&&!rooms[0].actors);CHECK(bh_draw_queue_begin(&queue));}
 }
 CHECK(!bh_script_actor_construct(&c,&point,4)&&pool.used==0);CHECK(bh_sound_queue_dispose(&sound));CHECK(bh_entities_dispose(&pool));CHECK(bh_draw_queue_dispose(&queue));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"script-actor-construct\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCRIPT_ACTOR_CONSTRUCT_STANDALONE
int main(void){return bh_script_actor_construct_checkpoint(0);}
#endif
