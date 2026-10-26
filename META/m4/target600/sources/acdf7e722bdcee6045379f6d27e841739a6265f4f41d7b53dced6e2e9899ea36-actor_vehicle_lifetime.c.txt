#include "../src/bh_actor_vehicle_lifetime.h"
#include "../src/bh_script_actor_query.h"
#include "../src/bh_fp.h"
#include <string.h>
#include <stdio.h>
static unsigned long checks,failures;
#define CHECK(c) do{++checks;if(!(c)){++failures;printf("failure %d\n",__LINE__);}}while(0)
static void put(BHEntity *e,unsigned long offset,BHEntity *p){memcpy((char*)e+offset,&p,4);}
static BHEntity *get(BHEntity *e,unsigned long offset){BHEntity *p;memcpy(&p,(char*)e+offset,4);return p;}
int bh_actor_vehicle_lifetime_checkpoint(unsigned long *count){
 BHActorVehicleLifetimeContext c;BHActorRouteContext route;BHScriptActorContext watcher;BHPickupRoom room;BHWorldGeometry world;BHEntityPool pool;BHSoundQueue sound;BHEntity *a,*b,*d,*w;BHActorRouteState s;BHSoundResource *r;unsigned long handle;BHMathEnvironment env;
 checks=failures=0;CHECK(bh_math_environment_enter(&env));CHECK(bh_entities_init(&pool,8,0));CHECK(bh_sound_queue_init(&sound,0,0,0));memset(&world,0,sizeof(world));memset(&room,0,sizeof(room));room.world=&world;memset(&route,0,sizeof(route));route.rooms=&room;route.room_count=1;route.sound=&sound;
 a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);d=bh_entity_acquire(&pool);w=bh_entity_acquire(&pool);CHECK(a&&b&&d&&w);memset(&s,0,sizeof(s));s.current=&world;s.sound=BH_SOUND_INVALID;bh_actor_route_write(a,&s);bh_actor_route_write(b,&s);bh_actor_route_write(d,&s);
 /* Global a,b,d; room d,a,b exercises independent list ownership. */
 c.pool=&pool;c.route=&route;c.head=a;c.count=3;put(a,0xec,0);put(a,0xf0,b);put(b,0xec,a);put(b,0xf0,d);put(d,0xec,b);put(d,0xf0,0);
 room.actors=d;put(d,0xf8,0);put(d,0xf4,a);put(a,0xf8,d);put(a,0xf4,b);put(b,0xf8,a);put(b,0xf4,0);
 memset(&watcher,0,sizeof(watcher));watcher.pool=&pool;watcher.route=&route;CHECK(bh_script_actor_bind(&watcher,w));put(w,0x1b8,a);put(w,0x1ac,a);
 r=bh_sound_resource_allocate(&sound);CHECK(r!=0);r->streaming=1;r->base_frequency=22050;r->base_volume=0;handle=bh_sound_request(&sound,r->index,1,1,0,0);CHECK(handle!=BH_SOUND_INVALID);memcpy((char*)a+0x104,&handle,4);
 CHECK(bh_actor_vehicle_release(&c,a));CHECK(c.head==b&&c.count==2&&room.actors==d&&get(d,0xf4)==b&&get(b,0xf8)==d&&get(b,0xec)==0&&get(d,0xec)==b);CHECK(get(w,0x1b8)==0&&get(w,0x1ac)==0);CHECK(!bh_sound_voice(&sound,handle)&&pool.used==3);CHECK(!bh_actor_vehicle_release(&c,a)&&c.count==2);
 put(w,0x1b8,d);CHECK(bh_actor_vehicle_release(&c,d));CHECK(c.head==b&&c.count==1&&room.actors==b&&get(b,0xf0)==0&&get(b,0xf8)==0&&get(b,0xf4)==0&&get(w,0x1b8)==0);
 CHECK(bh_actor_vehicle_release(&c,b));CHECK(!c.head&&!c.count&&!room.actors&&pool.used==1);CHECK(!bh_actor_vehicle_release(&c,b));CHECK(bh_entity_release(&pool,w));CHECK(bh_entities_dispose(&pool));CHECK(bh_sound_queue_dispose(&sound));bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"actor-vehicle-lifetime\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ACTOR_VEHICLE_LIFETIME_STANDALONE
int main(void){return bh_actor_vehicle_lifetime_checkpoint(0);}
#endif
