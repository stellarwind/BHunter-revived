#include "../src/bh_homing_projectile.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_debris_response.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<15)printf("FAIL homing-projectile %lu: %s\n",checks,s);}}while(0)
static int nearf(float a,double b){return fabs(a-b)<.0001;}
static void put(BHEntity*e,unsigned long a,const void*p,unsigned long n){memcpy((unsigned char*)e+a,p,n);}
static float number(BHEntity*e,unsigned long a){float f;memcpy(&f,(unsigned char*)e+a,4);return f;}
static void identity(BHMat34*m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
typedef struct Mesh{BHBsp bsp;BHBspVertex vertices[4];BHBspEdge edges[4];BHBspFace face;BHBspNode node;BHBspLeaf leaf;BHBspMaterial material;unsigned char storage;BHGeometry geometry;BHSegmentQuery query;}Mesh;
static int mesh(Mesh*m){unsigned long i;memset(m,0,sizeof(*m));m->bsp.vertices=m->vertices;m->bsp.edges=m->edges;m->bsp.faces=&m->face;m->bsp.nodes=&m->node;m->bsp.leaves=&m->leaf;m->bsp.materials=&m->material;m->bsp.storage=&m->storage;
 m->bsp.counts[0]=m->bsp.counts[3]=m->bsp.counts[4]=m->bsp.counts[5]=1;m->bsp.counts[1]=m->bsp.counts[2]=4;
 for(i=0;i<4;++i){m->vertices[i].y=(i==0||i==3)?-10:10;m->vertices[i].z=i<2?-10:10;m->edges[i].vertex=i;m->edges[i].next=i<3?i+1:-1;}
 m->node.normal[0]=1;m->node.front_node=m->node.back_node=-1;m->node.front_leaf=0;m->node.back_leaf=-1;m->leaf.face=0;m->leaf.next=-1;
 return bh_geometry_init(&m->geometry,&m->bsp,NULL)&&bh_segment_query_init(&m->query,&m->geometry);
}
typedef struct Fixture{BHEntityPool pool;BHDrawQueue queue;BHSoundQueue sound;BHScriptParticleContext particles;BHContactEffects contacts;BHFlashFlareContext flash;BHHitMarkerContext markers;BHWorldGeometry world;BHWorldSegmentRoom room;BHWorldSegmentContext segment;BHHomingWorldResponse response;BHHomingProjectileContext context;BHEntity*projectile,*target,*receiver;BHDebrisContext debris;BHDebrisBody body;BHPhysicsState physics;BHEntity*registry[2];Mesh mesh;}Fixture;
static int fixture(Fixture*f,int geometry){BHVec3 inertia={1,1,1};float life=1;unsigned long i,identity_word=0;BHSoundResource*r;
 memset(f,0,sizeof(*f));if(!bh_entities_init(&f->pool,32,NULL)||!bh_draw_queue_init(&f->queue,128,NULL)||!bh_sound_queue_init(&f->sound,NULL,NULL,NULL))return 0;
 f->projectile=bh_entity_acquire(&f->pool);f->target=bh_entity_acquire(&f->pool);f->receiver=bh_entity_acquire(&f->pool);if(!f->projectile||!f->target||!f->receiver)return 0;
 bh_entity_set_query(&f->pool,f->target,bh_marker_query);f->target->position.x=10;f->target->position.y=10;f->projectile->origin.z=30;put(f->projectile,0xe8,&life,4);put(f->projectile,0x11c,&identity_word,4);put(f->projectile,0x100,&f->target->position,12);
 f->world.navigation.next=&f->world.navigation;f->world.previous=&f->world;f->world.minimum.x=f->world.minimum.y=f->world.minimum.z=-100;f->world.maximum.x=f->world.maximum.y=f->world.maximum.z=100;f->world.query_flags=2;identity(&f->world.transform);identity(&f->world.inverse);f->room.world=&f->world;f->room.kind=1;
 if(geometry){if(!mesh(&f->mesh))return 0;f->room.query=&f->mesh.query;}
 if(!bh_world_segment_init(&f->segment,&f->world,1,&f->room,1,NULL,0,32))return 0;
 if(!bh_script_particles_init(&f->particles,&f->pool,&f->queue)||!bh_contact_effects_init(&f->contacts,&f->pool,&f->queue,7,8,9,10))return 0;
 f->flash.pool=&f->pool;f->flash.queue=&f->queue;for(i=0;i<4;++i)f->flash.textures[i]=12+i;f->markers.pool=&f->pool;f->markers.queue=&f->queue;f->markers.image=19;
 f->registry[0]=f->receiver;if(!bh_debris_context_init(&f->debris,&f->pool,&f->sound,BH_SOUND_INVALID,f->registry,2))return 0;identity(&f->physics.pose);f->physics.last_clear=f->physics.pose;
 if(!bh_debris_bind(&f->debris,&f->body,f->receiver,&f->physics,1,.5f,&inertia))return 0;
 for(i=0;i<2;++i){r=bh_sound_resource_allocate(&f->sound);if(!r)return 0;r->streaming=1;r->base_frequency=8000;}
 f->response.world=&f->world;f->response.receiver=f->receiver;f->context.pool=&f->pool;f->context.sound=&f->sound;f->context.segment=&f->segment;f->context.particles=&f->particles;f->context.contacts=&f->contacts;f->context.flash=&f->flash;f->context.markers=&f->markers;f->context.responses=&f->response;f->context.response_count=1;f->context.hit_sound=0;f->context.liquid_sound=1;f->context.first_image=21;f->context.second_image=22;
 return bh_homing_projectile_bind(&f->context,f->projectile);
}
static void finish(Fixture*f){CHECK(bh_hit_marker_dispose(&f->markers),"real marker dual-list disposal");CHECK(bh_debris_unbind(&f->body)&&bh_debris_context_dispose(&f->debris),"real physics response bindings released");if(f->mesh.geometry.bsp){bh_segment_query_dispose(&f->mesh.query);bh_geometry_dispose(&f->mesh.geometry);}CHECK(bh_sound_queue_dispose(&f->sound)&&bh_draw_queue_dispose(&f->queue)&&bh_entities_dispose(&f->pool),"real source owners disposed");}
static void guidance_and_expiry(void){Fixture f;BHEntity*target;float life;unsigned long generation,nan=0x7fc00000UL;
 CHECK(fixture(&f,0),"guidance actual owner fixture");target=f.target;put(f.projectile,0x12c,&target,4);f.projectile->step(f.projectile);
 CHECK(f.context.error==0&&f.context.guidance==1&&f.pool.used==3,"whole predictive guidance plus real segment miss");CHECK(nearf(f.projectile->origin.x,15/sqrt(2.0))&&nearf(f.projectile->origin.y,15/sqrt(2.0))&&f.projectile->origin.z==30,"orthogonal pursuit correction has original gain15");CHECK(nearf(f.projectile->position.x,.5/sqrt(2.0))&&nearf(f.projectile->position.y,.5/sqrt(2.0))&&f.projectile->position.z==1,"actual motion uses1/30 after new steering");CHECK(nearf(number(f.projectile,0xe8),1-1.0/30)&&!memcmp((unsigned char*)f.projectile+0x100,&target->position,12),"whole tick stores age and actual target sample");finish(&f);
 CHECK(fixture(&f,0),"opposite guidance fixture");target=f.target;target->position.x=target->position.y=0;target->position.z=-10;put(f.projectile,0x12c,&target,4);put(f.projectile,0x100,&target->position,12);f.projectile->step(f.projectile);CHECK(f.projectile->origin.z==30&&f.projectile->origin.x==0&&f.projectile->origin.y==0,"dot=-1 fails strict-.4 steering gate");finish(&f);
 CHECK(fixture(&f,0),"expiry fixture");life=0;put(f.projectile,0xe8,&life,4);generation=f.projectile->generation;f.projectile->step(f.projectile);CHECK(f.pool.used==2&&f.projectile->generation==generation+1&&f.context.guidance==0,"expired projectile releases before guidance/segment/effects");finish(&f);
 CHECK(fixture(&f,0),"unordered expiry fixture");put(f.projectile,0xe8,&nan,4);f.projectile->step(f.projectile);CHECK(f.pool.used==2,"unordered retained lifetime follows x87 release branch");finish(&f);
}
static void proximity(void){Fixture f;BHEntity *e;unsigned long i,particles=0,flashes=0,generation;int next,expected;BHScriptParticleState*s;
 CHECK(fixture(&f,0),"proximity fixture with actual event3 marker query");f.room.actors=f.target;f.target->position.x=2;f.target->position.y=0;generation=f.projectile->generation;
 srand(31);for(i=0;i<8;++i)rand();expected=rand();srand(31);f.projectile->step(f.projectile);next=rand();
 CHECK(f.context.proximity==1&&f.pool.used==6&&f.projectile->generation==generation+1,"whole room-proximity path retires projectile and constructs3 particles/flash");CHECK(next==expected,"actual shared CRT population:8 draws including real particle constructors");
 for(e=f.pool.active;e;e=e->next){if(e->step==bh_script_particle_tick){++particles;s=bh_script_particle_state(&f.particles,e);CHECK(s&&((s->resource==21&&s->size==1.6f)||(s->resource==22&&s->size==.8f)||(s->resource==21&&s->size==.8f)),"actual particle constructor receives original images and sizes");}if(e->step==bh_script_flash_tick){++flashes;CHECK(number(e,0xe8)==.1f&&number(e,0xec)==50&&number(e,0xf0)==2,"actual flash owner receives original fields");}}
 CHECK(particles==3&&flashes==1&&f.sound.voice_count==1,"whole impact creates real sound queue state without playback");CHECK(f.context.damage_packets==1,"damage delivered through actual existing receiver identity");finish(&f);
 CHECK(fixture(&f,0),"strict proximity boundary fixture");f.room.actors=f.target;f.target->position.x=3;f.target->position.y=0;f.projectile->step(f.projectile);CHECK(!f.context.proximity&&f.pool.used==3&&f.projectile->position.z==1,"distance equality3 does not trigger strict squared9 test");finish(&f);
 CHECK(fixture(&f,0),"ignored actor fixture");f.room.actors=f.target;f.target->position.x=1;f.target->position.y=0;put(f.projectile,0x128,&f.target,4);f.projectile->step(f.projectile);CHECK(!f.context.proximity&&f.pool.used==3,"actual stored self exclusion applies to proximity list");finish(&f);
}
static void geometry_hits(void){Fixture f;BHEntity*e;BHImpactState*s;unsigned long generation,i;float life=1;int expected,next;
 CHECK(fixture(&f,1),"real geometric wall fixture");f.projectile->position.x=3;f.projectile->origin.x=-180;f.projectile->origin.z=0;generation=f.projectile->generation;
 bh_geometry_set_flags(&f.mesh.geometry,0,0);f.projectile->step(f.projectile);
 CHECK(f.context.world_hits==1&&f.markers.count==1&&f.pool.used==7&&f.projectile->generation==generation+1,"whole solid hit uses actual segment query, marker, particles,flash,release");CHECK(f.context.damage_packets==1&&nearf(f.physics.acceleration.x,-45),"real contact packet reaches actual debris physics receiver before damage");
 e=f.markers.newest;CHECK(e&&number(e,0x108)==1.5f&&number(e,0x10c)==0&&number(e,0x110)==.5f,"solid hit actual marker gets1.5 size and quadrant2");CHECK(e&&nearf(e->position.x,.15)&&nearf(number(e,0x114),1),"actual hit point0.05 plus original-.1 normal offset");finish(&f);
 CHECK(fixture(&f,1),"flag2 no marker fixture");f.projectile->position.x=3;f.projectile->origin.x=-180;f.projectile->origin.z=0;bh_geometry_set_flags(&f.mesh.geometry,0,2);f.projectile->step(f.projectile);CHECK(f.context.world_hits==1&&!f.markers.count&&f.pool.used==6,"flag2 suppresses only marker, retains ordinary impact");finish(&f);
 CHECK(fixture(&f,1),"liquid surface fixture");f.projectile->position.x=3;f.projectile->origin.x=-180;f.projectile->origin.z=0;bh_geometry_set_flags(&f.mesh.geometry,0,4);srand(19);rand();expected=rand();srand(19);f.projectile->step(f.projectile);next=rand();
 CHECK(f.context.world_hits==1&&!f.markers.count&&f.pool.used==3&&next==expected,"liquid branch consumes single shared random draw and emits real contact effect only");s=NULL;for(e=f.pool.active;e;e=e->next)if((s=bh_contact_impact_state(&f.contacts,e)))break;CHECK(s&&s->size==6&&s->duration>.6f&&s->duration<.65f&&nearf(e->position.x,.55),"liquid effect has exact randomized duration,6-unit size,half-normal offset");CHECK(!f.context.damage_packets&&f.sound.voice_count==1,"liquid does not deliver damage or normal particle burst");finish(&f);
 CHECK(fixture(&f,1),"damage-suppressed projectile fixture");f.projectile->position.x=3;f.projectile->origin.x=-180;f.projectile->origin.z=0;i=1;put(f.projectile,0x11c,&i,4);bh_geometry_set_flags(&f.mesh.geometry,0,2);f.projectile->step(f.projectile);CHECK(!f.context.damage_packets&&f.context.world_hits==1&&f.pool.used==6,"nonzero identity suppresses damage after preserving contact and effects");finish(&f);
}
static void*__cdecl fail_alloc(unsigned long size,void*user){(void)size;(void)user;return NULL;}
static void actor_and_allocation(void){Fixture f;BHWorldSegmentActor actor;float basis[9]={1,0,0,0,1,0,0,0,1};unsigned long i;int expected,next;
 CHECK(fixture(&f,1),"actual global actor mesh hit fixture");f.room.query=NULL;f.receiver->collision_radius=2;memcpy(f.receiver->opaque0,basis,36);actor.entity=f.receiver;actor.query=&f.mesh.query;f.segment.actors=&actor;f.segment.actor_count=1;f.projectile->position.x=3;f.projectile->origin.x=-180;f.projectile->origin.z=0;f.projectile->step(f.projectile);
 CHECK(f.context.world_hits==1&&!f.markers.count&&f.pool.used==6&&f.context.damage_packets==1,"whole actor-mesh branch skips world marker and invokes actual response");CHECK(nearf(f.physics.acceleration.x,-45),"actor collision impulse delivered to real shared physics component");finish(&f);
 CHECK(fixture(&f,1),"real pool allocation-failure impact fixture");f.projectile->position.x=3;f.projectile->origin.x=-180;f.projectile->origin.z=0;while(f.pool.used<f.pool.capacity)CHECK(bh_entity_acquire(&f.pool)!=NULL,"fill actual entity pool before failure control");f.pool.allocator.allocate=fail_alloc;
 srand(13);for(i=0;i<5;++i)rand();expected=rand();srand(13);f.projectile->step(f.projectile);next=rand();
 CHECK(f.context.world_hits==1&&!f.markers.count&&f.pool.used==31&&f.context.damage_packets==1,"marker/particle/flash allocation failures retain contact and retirement/damage prefix");CHECK(next==expected,"failed real particle acquisitions consume no constructor random draws");finish(&f);
}
int __cdecl bh_homing_projectile_checkpoint(unsigned long*count){BHMathEnvironment fp;setvbuf(stdout,NULL,_IONBF,0);checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;guidance_and_expiry();proximity();geometry_hits();actor_and_allocation();bh_math_environment_leave(&fp);if(count)*count=checks;printf("{\"scenario\":\"homing-projectile\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_HOMING_PROJECTILE_STANDALONE
int main(void){unsigned long count;return bh_homing_projectile_checkpoint(&count);}
#endif


