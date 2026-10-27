#include "../src/bh_area_projectile.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include "../src/bh_debris_response.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL area-projectile %lu %s\n",checks,s);}}while(0)
static float number(BHEntity *e,unsigned long a){float f;memcpy(&f,(unsigned char*)e+a,4);return f;}
static unsigned long word(BHEntity *e,unsigned long a){unsigned long n;memcpy(&n,(unsigned char*)e+a,4);return n;}
static void scalar(BHEntity *e,unsigned long a,float f){memcpy((unsigned char*)e+a,&f,4);}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
typedef struct Mesh{BHBsp bsp;BHBspVertex vertices[4];BHBspEdge edges[4];BHBspFace face;BHBspNode node;BHBspLeaf leaf;BHBspMaterial material;unsigned char storage;BHGeometry geometry;BHSegmentQuery query;}Mesh;
static int mesh(Mesh*m){unsigned long i;memset(m,0,sizeof(*m));m->bsp.vertices=m->vertices;m->bsp.edges=m->edges;m->bsp.faces=&m->face;m->bsp.nodes=&m->node;m->bsp.leaves=&m->leaf;m->bsp.materials=&m->material;m->bsp.storage=&m->storage;
 m->bsp.counts[0]=m->bsp.counts[3]=m->bsp.counts[4]=m->bsp.counts[5]=1;m->bsp.counts[1]=m->bsp.counts[2]=4;
 for(i=0;i<4;++i){m->vertices[i].y=(i==0||i==3)?-10:10;m->vertices[i].z=i<2?-10:10;m->edges[i].vertex=i;m->edges[i].next=i<3?i+1:-1;}
 m->node.normal[0]=1;m->node.front_node=m->node.back_node=-1;m->node.front_leaf=0;m->node.back_leaf=-1;m->leaf.face=0;m->leaf.next=-1;
 return bh_geometry_init(&m->geometry,&m->bsp,NULL)&&bh_segment_query_init(&m->query,&m->geometry);
}
int __cdecl bh_area_projectile_checkpoint(unsigned long *count){BHMathEnvironment fp;BHEntityPool pool;BHDrawQueue queue;BHSoundQueue sound;BHHomingProjectileContext effects;BHProjectileDrawContext draw;BHAreaProjectileContext c;BHWorldGeometry world;BHWorldSegmentRoom room;BHWorldSegmentContext segment;BHWorldSegmentActor registry;BHEntity *source,*e,*actor,*slot,*head;BHSoundResource *resource;BHSoundVoice *voice;Mesh m;BHDebrisContext response_context;BHDebrisBody body;BHPhysicsState physics;BHEntity *response_registry[2];BHHomingWorldResponse response;BHVec3 inertia={1,1,1};BHVec3 p={0,0,0},v={1,0,0};unsigned long token=27,n,generation;long packet[2];
 memset(&effects,0,sizeof(effects));memset(&draw,0,sizeof(draw));memset(&c,0,sizeof(c));memset(&world,0,sizeof(world));memset(&room,0,sizeof(room));memset(&body,0,sizeof(body));
 CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_entities_init(&pool,12,NULL),"actual entity owner");CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"actual source voice owner");CHECK(bh_draw_queue_init(&queue,50,NULL),"actual queue");source=bh_entity_acquire(&pool);actor=bh_entity_acquire(&pool);CHECK(bh_entity_set_query(&pool,actor,bh_marker_query),"real position receiver");
 world.navigation.next=&world.navigation;world.previous=&world;world.minimum.x=world.minimum.y=world.minimum.z=-1000;world.maximum.x=world.maximum.y=world.maximum.z=1000;world.query_flags=2;identity(&world.transform);identity(&world.inverse);room.world=&world;room.kind=1;
 CHECK(bh_world_segment_init(&segment,&world,1,&room,1,NULL,0,12),"actual empty room collision closure");resource=bh_sound_resource_allocate(&sound);resource->streaming=1;resource->base_frequency=8000;resource=bh_sound_resource_allocate(&sound);resource->streaming=1;resource->base_frequency=8000;
 effects.pool=&pool;effects.sound=&sound;effects.segment=&segment;draw.queue=&queue;draw.pulse_image=31;c.effects=&effects;c.draw=&draw;c.identity=&token;c.loop_sound=0;c.hit_sound=1;
 slot=bh_entity_acquire(&pool);scalar(slot,0x100,123);bh_entity_release(&pool,slot);e=bh_area_projectile_create(&c,&world,&p,&v,source,1);
 CHECK(e==slot&&token==0&&word(e,0xf4)==27,"real recycled allocation and shared identity consumption");CHECK(e->step==bh_area_projectile_tick&&e->query==bh_pickup_target_removed&&e->refresh,"actual installed callback closure");CHECK(number(e,0xe8)==1&&number(e,0xec)==20&&e->origin.x==300&&number(e,0x100)==123,"initial power radius scaled velocity unused preservation");CHECK(word(e,0xfc)==(unsigned long)source&&word(e,0x104)==(unsigned long)source&&word(e,0xf8)==(unsigned long)&world,"actual borrowed source/cache fields");voice=bh_sound_voice(&sound,word(e,0xf0));CHECK(voice&&voice->frequency==8000&&voice->velocity.x==300,"actual loop request");
 e->refresh(e,NULL);CHECK(draw.queued==1,"installed actual pulse render producer");e->step(e);CHECK(c.error==0&&fabs(e->position.x-10)<.0001&&fabs(number(e,0xe8)-.99)<.00001&&fabs(number(e,0xec)-20.6666667)<.00001,"whole tick growth attenuation and actual segment movement");voice=bh_sound_voice(&sound,word(e,0xf0));CHECK(voice&&fabs(voice->volume-(log(.99)*(-1000)/log(.5)))<.01&&voice->position.x==10,"actual loop update");
 packet[0]=17;packet[1]=(long)source;e->query(e,packet);CHECK(word(e,0x104)==0&&word(e,0xfc)==(unsigned long)source,"actual target removal only clears original104");scalar(e,0xec,24.333334f);e->step(e);CHECK(word(e,0xfc)==0,"radius threshold clears collision ignore");
 e->origin.x=0;e->position.x=0;actor->position.x=1;room.actors=actor;memset((unsigned char*)actor+0xf4,0,4);scalar(e,0xf4,1);n=c.room_hits;e->step(e);CHECK(c.room_hits==n+1&&c.error==0,"actual room event3 contact traversal");CHECK(number(e,0xe8)<.9f,"room hit attenuates power");room.actors=NULL;registry.entity=actor;registry.query=NULL;segment.actors=&registry;segment.actor_count=1;n=c.global_hits;e->step(e);CHECK(c.global_hits==n+1&&c.error==0,"ordered actual global actor registry proximity contact");
 segment.actor_count=0;room.actors=actor;head=NULL;room.actors_owner=&head;n=c.room_hits;e->step(e);CHECK(c.room_hits==n,"empty live room overrides stale copied head");head=actor;e->step(e);CHECK(c.room_hits==n+1,"post-binding room insertion reaches actual area damage traversal");head=NULL;e->step(e);CHECK(c.room_hits==n+1,"room removal does not retain or fall back to stale head");room.actors_owner=NULL;room.actors=NULL;
 scalar(e,0xe8,.1010101f);generation=e->generation;n=pool.used;e->step(e);CHECK(pool.used==n-1&&e->generation==generation+1,"attenuation crosses expiry then retires actual slot");CHECK(!bh_sound_voice(&sound,word(e,0xf0)),"expiry stops own loop");
 CHECK(mesh(&m),"actual geometry wall");room.query=&m.query;segment.actors=NULL;segment.actor_count=0;response_registry[0]=source;response_registry[1]=NULL;CHECK(bh_debris_context_init(&response_context,&pool,&sound,BH_SOUND_INVALID,response_registry,2),"actual physics receiver owner");memset(&physics,0,sizeof(physics));identity(&physics.pose);physics.last_clear=physics.pose;CHECK(bh_debris_bind(&response_context,&body,source,&physics,1,.5f,&inertia),"actual physics contact receiver");response.world=&world;response.receiver=source;effects.responses=&response;effects.response_count=1;
 token=1;p.x=3;v.x=-.6f;e=bh_area_projectile_create(&c,&world,&p,&v,NULL,2);n=c.collisions;e->step(e);CHECK(c.error==0&&c.collisions==n+1&&e->origin.x>0&&fabs(e->position.x-.05)<.0001,"whole actual wall hit reflects velocity and publishes contact point");CHECK(fabs(number(e,0xe8)-1.782)<.00001,"collision power attenuation follows lifetime attenuation");CHECK(physics.acceleration.x!=0,"contact packet delivered through actual debris receiver");
 token=0;p.x=3;e=bh_area_projectile_create(&c,&world,&p,&v,NULL,2);e->step(e);CHECK(c.error==0&&c.collisions==n+2,"unsuppressed damage path invokes actual receiver after contact");CHECK(bh_debris_unbind(&body)&&bh_debris_context_dispose(&response_context),"actual receiver disposed");CHECK(bh_segment_query_dispose(&m.query)&&bh_geometry_dispose(&m.geometry),"actual mesh owners disposed");room.query=NULL;
 token=33;n=pool.used;c.draw=NULL;CHECK(!bh_area_projectile_create(&c,&world,&p,&v,source,1)&&token==33&&pool.used==n,"missing renderer rejects before side effects");c.draw=&draw;
 CHECK(bh_entities_valid(&pool),"entity rings valid");CHECK(bh_entities_dispose(&pool)&&bh_sound_queue_dispose(&sound)&&bh_draw_queue_dispose(&queue),"actual disposal");bh_math_environment_leave(&fp);if(count)*count+=checks;return !failures;
}
#ifdef BH_AREA_PROJECTILE_STANDALONE
int main(void){unsigned long n=0;int ok=bh_area_projectile_checkpoint(&n);printf("area-projectile: %lu checks, %lu failures\n",n,failures);return !ok;}
#endif

