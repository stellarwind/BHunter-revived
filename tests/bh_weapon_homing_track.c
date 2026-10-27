#include "../src/bh_weapon_homing_track.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include "../src/bh_actor_vehicle_lifetime.h"
#include "../src/bh_weapon_arc_lifecycle.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL homing-track %lu %s\n",checks,s);}}while(0)
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
/* Original00430d80/00431180/00432c60 load004dfb28 at acquisition.
   Remove the head through004397e0's actual source owner between callbacks;
   the obsolete frame snapshot must never be queried or selected. */
static void live_head(BHEntityPool *pool,BHEntity *w,BHEntity *p,BHEntity *a,BHEntity *b,BHWeaponTargetContext *c,BHWorldGeometry *world){
 BHActorVehicleLifetimeContext life;BHActorRouteContext route;BHPickupRoom room;
 BHActorRouteState state;BHWeaponArcLifecycleContext arc;BHSoundQueue sound;
 BHWeaponCommonState *s=bh_weapon_common_state(w);BHEntity *nil=NULL;
 memset(&life,0,sizeof(life));memset(&route,0,sizeof(route));memset(&room,0,sizeof(room));memset(&state,0,sizeof(state));memset(&arc,0,sizeof(arc));
 CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"live-head sound owner");
 route.rooms=&room;route.room_count=1;route.actor_limit=pool->capacity;route.sound=&sound;
 room.world=world;room.actors=a;life.pool=pool;life.route=&route;life.head=a;life.count=2;
 state.current=world;state.sound=BH_SOUND_INVALID;bh_actor_route_write(a,&state);bh_actor_route_write(b,&state);
 memcpy((char*)a+0xec,&nil,4);memcpy((char*)a+0xf0,&b,4);memcpy((char*)a+0xf4,&b,4);memcpy((char*)a+0xf8,&nil,4);
 memcpy((char*)b+0xec,&a,4);memcpy((char*)b+0xf0,&nil,4);memcpy((char*)b+0xf4,&nil,4);memcpy((char*)b+0xf8,&a,4);
 c->actors=a;c->actors_owner=&life.head;c->max_actors=pool->capacity;c->sound=&sound;
 arc.head=a;arc.head_owner=&life.head;arc.world=c->world;arc.sound=&sound;arc.limit=pool->capacity;
 a->position.x=10;a->position.y=0;b->position.x=20;b->position.y=0;
 CHECK(bh_actor_vehicle_release(&life,a)&&life.head==b&&room.actors==b,"real head retirement updates global and room lists");
 memset(s,0,sizeof(*s));s->ammo=1;s->word_100=-1;c->queries=0;
 CHECK(bh_weapon_homing_track(c,w,p)&&s->hud.target==b&&c->queries==1,"homing reads surviving live head after retirement");
 s->hud.target=NULL;c->queries=0;
 CHECK(bh_weapon_target_update(c,w,p)&&s->hud.target==b&&c->queries==1,"charged targeting reads surviving live head");
 s->hud.target=NULL;s->word_108=1;
 CHECK(bh_weapon_arc_track(&arc,w,p)&&s->hud.target==b&&arc.queries==1,"arc reads surviving live head");
 CHECK(bh_actor_vehicle_release(&life,b)&&!life.head&&!room.actors,"real final retirement empties both lists");
 s->hud.target=NULL;c->queries=0;CHECK(bh_weapon_homing_track(c,w,p)&&!s->hud.target&&!c->queries,"homing honors empty live owner over stale snapshot");
 CHECK(bh_weapon_target_update(c,w,p)&&!s->hud.target&&!c->queries,"charged targeting honors empty live owner");
 s->word_108=1;CHECK(bh_weapon_arc_track(&arc,w,p)&&!s->hud.target&&!arc.queries,"arc honors empty live owner");
 c->actors_owner=NULL;CHECK(bh_sound_queue_dispose(&sound),"live-head sound disposal");
}
int __cdecl bh_weapon_homing_track_checkpoint(unsigned long *count){BHMathEnvironment fp;BHEntityPool pool;BHEntity *w,*p,*a,*b,*nil=NULL;BHWeaponCommonState *s;BHWeaponTargetContext c;BHWorldGeometry world;BHWorldSegmentRoom room;BHWorldSegmentContext segment;BHVec3 aim={1,0,0};float f=1;unsigned long bits;
 memset(&c,0,sizeof(c));memset(&world,0,sizeof(world));memset(&room,0,sizeof(room));CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_entities_init(&pool,5,NULL),"actual pool");w=bh_entity_acquire(&pool);p=bh_entity_acquire(&pool);a=bh_entity_acquire(&pool);b=bh_entity_acquire(&pool);s=bh_weapon_common_state(w);memset(s,0,sizeof(*s));CHECK(bh_entity_set_query(&pool,a,bh_marker_query)&&bh_entity_set_query(&pool,b,bh_marker_query),"actual position callbacks");memcpy((unsigned char*)p+0x280,&aim,12);a->position.x=30;b->position.x=20;memcpy((unsigned char*)a+0xf0,&b,4);memcpy((unsigned char*)b+0xf0,&nil,4);
 world.navigation.next=&world.navigation;world.previous=&world;world.minimum.x=world.minimum.y=world.minimum.z=-1000;world.maximum.x=world.maximum.y=world.maximum.z=1000;world.query_flags=2;identity(&world.transform);identity(&world.inverse);room.world=&world;room.kind=1;CHECK(bh_world_segment_init(&segment,&world,1,&room,1,NULL,0,5),"actual unobstructed trace owner");c.world=&segment;c.actors=a;c.max_actors=5;s->delay=-1;memcpy(&s->word_f0,&f,4);memcpy(&s->word_f4,&f,4);
 CHECK(bh_weapon_homing_track(&c,w,p)&&s->hud.target==b&&c.queries==2&&c.traces==2,"actual target chain selects nearest forward candidate");CHECK(s->delay==-2&&s->hud.charge==0,"signed delay and reset charge");memcpy(&f,&s->word_f0,4);CHECK(fabs(f-(1-1.0/30))<.000001&&s->word_f0==s->word_f4,"both positive cooldowns decrement");CHECK(bh_weapon_homing_track(&c,w,p)&&s->hud.charge==1&&s->hud.target==b,"retained real query target increments charge");b->position.x=-1;CHECK(bh_weapon_homing_track(&c,w,p)&&!s->hud.target,"behind-player retained target drops");b->position.x=151;s->hud.target=b;CHECK(bh_weapon_homing_track(&c,w,p)&&!s->hud.target,"beyond range target drops");b->position.x=10;b->position.y=11;s->hud.target=b;s->hud.charge=0;CHECK(bh_weapon_homing_track(&c,w,p)&&!s->hud.target,"initial lock uses tighter angular gate");s->hud.target=b;s->hud.charge=2;CHECK(bh_weapon_homing_track(&c,w,p)&&s->hud.target==b,"charged lock retains wider angular target");b->position.y=20;CHECK(bh_weapon_homing_track(&c,w,p)&&!s->hud.target,"wide angular gate drops target");
 a->position.x=150;a->position.y=0;b->position.x=-10;b->position.y=0;CHECK(bh_weapon_homing_track(&c,w,p)&&!s->hud.target,"strict150 acquisition and behind candidate gates");s->hud.target=b;s->hud.charge=0x7fffffff;b->position.x=10;CHECK(bh_weapon_homing_track(&c,w,p)&&s->hud.charge==(long)0x80000000,"original32bit signed charge wraps");s->hud.target=NULL;c.max_actors=1;CHECK(!bh_weapon_homing_track(&c,w,p)&&c.error==BH_WEAPON_TARGET_LIMIT,"malformed over-budget actor traversal rejects");live_head(&pool,w,p,a,b,&c,&world);CHECK(bh_entities_dispose(&pool),"actual disposal");bh_math_environment_leave(&fp);if(count)*count+=checks;return !failures;}
#ifdef BH_WEAPON_HOMING_TRACK_STANDALONE
int main(void){unsigned long n=0;int ok=bh_weapon_homing_track_checkpoint(&n);printf("homing-track: %lu checks, %lu failures\n",n,failures);return !ok;}
#endif
