#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include "../src/bh_target_construct.h"
#include "../src/bh_actor_damage.h"
#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks, failed;
#define CHECK(x, s)                                                            \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x)) {                                                                \
      ++failed;                                                                \
      if (failed < 20)                                                         \
        printf("FAIL actor-damage %d: %s\n", __LINE__, s);                 \
    }                                                                          \
  } while (0)
static void identity(BHMat34 *m) {
  memset(m, 0, sizeof(*m));
  m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}
typedef struct Fixture {
  BHEntityPool pool;
  BHEntity *e, *owner, *registry[4];
  BHDrawQueue queue;
  BHSoundQueue sound;
  BHDebrisContext debris;
  BHDebrisBody body;
  BHPhysicsState state;
  BHPhysicsStepContext physics;
  BHPhysicsStepBody pb;
  BHPhysicsEffects effects;
  BHContactEffects contact;
  BHGeometry *models[1];
  BHMat34 inverse;
  BHScriptDebrisContext fragments;
  BHFlashFlareContext flash;
  BHScriptExplosionContext explosions;
  BHScriptParticleContext particles;
  BHPickupContext pickups;
  BHPickupRenderBinding render;
  BHPickupDrawContext draw;
  BHEntityResourceDrawContext special;
  BHWorldGeometry world;
  BHPickupRoom room;
  unsigned long resources[20];
  BHTargetConstructContext construct;
  BHTargetConstructSlot slots[4];
  BHPhysicsStepBody roster[4];
  BHGeometry *catalog[4];
  BHTargetDrawContext target_draw;
  BHMainCamera camera;
  BHViewConfig view;
  long mode;
} Fixture;
static int setup(Fixture *f) {
  BHVec3 inertia = {1, 1, 1};
  BHSoundResource *r;
  unsigned long i;
  memset(f, 0, sizeof(*f));
  if (!bh_entities_init(&f->pool, 32, NULL) ||
      !bh_draw_queue_init(&f->queue, 64, NULL) ||
      !bh_sound_queue_init(&f->sound, NULL, NULL, NULL))
    return 0;
  r = bh_sound_resource_allocate(&f->sound);
  if (!r)
    return 0;
  r->streaming = 1;
  r->base_frequency = 100;
  f->e = bh_entity_acquire(&f->pool);
  f->owner = bh_entity_acquire(&f->pool);
  f->owner->position.x = 1000;
  bh_entity_set_query(&f->pool, f->owner, bh_actor_target_removed);
  f->registry[0] = f->e;
  f->registry[1] = f->owner;
  identity(&f->state.pose);
  f->state.last_clear = f->state.pose;
  identity(&f->inverse);
  if (!bh_debris_context_init(&f->debris, &f->pool, &f->sound, r->index,
                              f->registry, 4) ||
      !bh_debris_bind(&f->debris, &f->body, f->e, &f->state, 2, 0, &inertia))
    return 0;
  f->body.owner = f->owner;
  f->pb.entity = f->e;
  f->pb.generation = f->e->generation;
  f->pb.physics = &f->state;
  f->pb.target_world = &f->state.pose;
  f->pb.target_local = &f->inverse;
  f->pb.mass = 2;
  if (!bh_contact_effects_init(&f->contact, &f->pool, &f->queue, 1, 2, 3, 4))
    return 0;
  f->effects.effects = &f->contact;
  f->effects.sound = &f->sound;
  if (!bh_physics_step_init(&f->physics, &f->pool, &f->pb, 1, NULL, 0,
                            f->models, 1, &f->effects, 1, NULL))
    return 0;
  f->fragments.pool = &f->pool;
  f->fragments.queue = &f->queue;
  f->fragments.image = 9;
  f->flash.pool = &f->pool;
  f->flash.queue = &f->queue;
  f->flash.textures[0] = 10;
  f->explosions.pool = &f->pool;
  f->explosions.queue = &f->queue;
  f->explosions.sound = &f->sound;
  f->explosions.debris = &f->fragments;
  f->explosions.flash = &f->flash;
  f->explosions.sound_resource = r->index;
  f->explosions.image = 11;
  if (!bh_script_particles_init(&f->particles, &f->pool, &f->queue))
    return 0;
  f->world.navigation.next = &f->world.navigation;
  f->world.minimum.x = f->world.minimum.y = f->world.minimum.z = -100;
  f->world.maximum.x = f->world.maximum.y = f->world.maximum.z = 100;
  identity(&f->world.transform);
  f->room.world = &f->world;
  for (i = 0; i < 20; ++i)
    f->resources[i] = i + 1;
  f->pickups.pool = &f->pool;
  f->pickups.first_world = &f->world;
  f->pickups.world_count = 1;
  f->pickups.rooms = &f->room;
  f->pickups.room_count = 1;
  f->pickups.resources = f->resources;
  f->pickups.resource_count = 20;
  f->pickups.outside_actors = f->registry;
  f->pickups.sound = &f->sound;
  f->draw.queue = &f->queue;
  f->special.queue = &f->queue;
  f->render.default_draw = &f->draw;
  f->render.special_draw = &f->special;
  if (!bh_pickup_render_bind(&f->pickups, &f->render))
    return 0;
  f->target_draw.queue = &f->queue;
  f->target_draw.camera = &f->camera;
  f->target_draw.view = &f->view;
  f->target_draw.screen_width = 640;
  f->target_draw.player = f->owner;
  f->construct.debris = &f->debris;
  f->construct.physics = &f->physics;
  f->construct.effects = &f->effects;
  f->construct.explosions = &f->explosions;
  f->construct.particles = &f->particles;
  f->construct.pickups = &f->pickups;
  f->construct.draw = &f->target_draw;
  f->construct.mode = &f->mode;
  f->construct.slots = f->slots;
  f->construct.capacity = 4;
  f->construct.roster = f->roster;
  f->construct.models = f->catalog;
  f->construct.resources = f->resources;
  f->construct.resource_count = 20;
  f->construct.prefix = "actor-damage-fixture\\";
  f->construct.search = BH_BSP_PREFIX_ONLY;
  if (!bh_physics_step_dispose(&f->physics) || !bh_debris_unbind(&f->body) ||
      !bh_entity_release(&f->pool, f->e))
    return 0;
  f->registry[0] = NULL;
  f->e = NULL;
  return 1;
}
static int model_file(void) {
  BHCollisionModel model;
  BHVec3 extent = {2, 4, 6}, padding = {1, 2, 3};
  FILE *f;
  unsigned long i;
  const void *arrays[6];
  static const unsigned long sizes[6] = {32, 60, 16, 12, 56, 8};
  _mkdir("actor-damage-fixture");
  f = fopen("actor-damage-fixture/HUN_LOD1.BSP", "rb");
  if (f) {
    fclose(f);
    return 0;
  }
  if (!bh_collision_model_init(&model, NULL) ||
      !bh_collision_box_build(&model, &extent, &padding))
    return 0;
  arrays[0] = model.bsp.materials;
  arrays[1] = model.bsp.vertices;
  arrays[2] = model.bsp.edges;
  arrays[3] = model.bsp.faces;
  arrays[4] = model.bsp.nodes;
  arrays[5] = model.bsp.leaves;
  f = fopen("actor-damage-fixture/HUN_LOD1.BSP", "wb");
  if (!f)
    return 0;
  for (i = 0; i < 6; ++i) {
    fwrite(model.bsp.counts + i, 4, 1, f);
    fwrite(arrays[i], sizes[i], model.bsp.counts[i], f);
  }
  fclose(f);
  return bh_collision_model_dispose(&model);
}
static void finish(Fixture *f) {
  unsigned long i;
  for (i = 0; i < 4; ++i)
    if (f->slots[i].body.entity)
      bh_debris_remove(f->slots[i].body.entity);
  CHECK(bh_target_construct_dispose(&f->construct),
        "real collision and dynamic physics owners disposed");
  while (f->pickups.head)
    bh_pickup_cleanup(f->pickups.head);
  while (f->pool.active)
    bh_entity_release(&f->pool, f->pool.active);
  CHECK(bh_debris_context_dispose(&f->debris) &&
            bh_contact_effects_dispose(&f->contact) &&
            bh_sound_queue_dispose(&f->sound) &&
            bh_draw_queue_dispose(&f->queue) && bh_entities_dispose(&f->pool),
        "actual dependencies release all ownership");
}
static float field(BHEntity *e,unsigned long a){float f;memcpy(&f,(char*)e+a,4);return f;}
static void setfield(BHEntity *e,unsigned long a,float f){memcpy((char*)e+a,&f,4);}
static void setptr(BHEntity *e,unsigned long a,BHEntity *p){memcpy((char*)e+a,&p,4);}
static unsigned long rows[1][32];
static BHEntity *actor(Fixture *f,BHActorDamageContext *c){
 BHActorRouteState s;BHEntity *e=bh_entity_acquire(&f->pool);BHEntity *head=f->room.actors;memset(&s,0,sizeof(s));s.current=s.next=&f->world;s.duration_ticks=30;s.controls[1].x=1;s.controls[2].x=1;s.controls[3].x=2;s.sound=BH_SOUND_INVALID;s.speed_scale=1;bh_actor_route_write(e,&s);setfield(e,0x19c,1);setfield(e,0x194,0);setptr(e,0xf4,head);setptr(e,0xf8,0);if(head)setptr(head,0xf8,e);f->room.actors=e;bh_actor_damage_bind(c,e);return e;
}
static void damage_fixture(Fixture *f,BHActorDamageContext *c,BHActorRouteContext *route,const BHVec3 *angles){
 memset(c,0,sizeof(*c));memset(route,0,sizeof(*route));route->rooms=&f->room;route->room_count=1;route->sound=&f->sound;route->actor_limit=128;c->pool=&f->pool;c->route=route;c->particles=&f->particles;c->fragments=&f->fragments;c->targets=&f->construct;c->loot_rows=rows;c->loot_row_count=1;c->smoke_resource=15;c->death_sound=0;c->legacy_death_angles=angles;
}
int bh_actor_damage_checkpoint(unsigned long *count){
 Fixture f;BHActorDamageContext c;BHActorRouteContext route;BHMathEnvironment env;BHEntity *e,*target,before;BHWorldGeometry foreign;BHVec3 angles={.1f,.2f,.3f};BHActorRouteState s;float packet[10];unsigned long *words=(unsigned long*)packet;unsigned long used,seed,expected,n,i,gen;BHTargetStepState ts;
 checks=failed=0;CHECK(bh_math_environment_enter(&env),"PC53");CHECK(model_file(),"real fixture collision BSP");CHECK(setup(&f),"real dependencies");damage_fixture(&f,&c,&route,&angles);e=actor(&f,&c);CHECK(e&&e->query==bh_actor_damage_query,"actual query installed");
 memset(&foreign,0,sizeof(foreign));bh_actor_route_read(e,&s);s.current=&foreign;bh_actor_route_write(e,&s);before=*e;used=f.pool.used;words[0]=4;bh_actor_damage_query(e,packet);
 CHECK(c.error==BH_ACTOR_ROUTE_TOPOLOGY&&!memcmp(e,&before,sizeof(before))&&f.pool.used==used&&f.room.actors==e,"unregistered-room release rejects without actor/list/pool/query mutation");
 s.current=f.room.world;bh_actor_route_write(e,&s);c.error=BH_ACTOR_ROUTE_OK;
 memset(packet,0,sizeof(packet));words[0]=3;bh_actor_damage_query(e,packet);CHECK(packet[1]==0&&packet[2]==0&&packet[3]==0,"query position at start");route.tick=15;words[0]=16;bh_actor_damage_query(e,packet);CHECK(fabs(packet[1]-1)<.000001&&packet[4]>2.49&&packet[4]<2.5&&fabs(packet[7])<.0001,"sampled velocity and acceleration");
 words[0]=13;words[3]=0x12345678;bh_actor_damage_query(e,packet);CHECK(words[5]==1&&words[6]==0x12345678,"ray metadata event13");
 memset(packet,0,sizeof(packet));words[0]=1;packet[1]=-1;packet[4]=100;packet[5]=4;bh_actor_damage_query(e,packet);CHECK(fabs(field(e,0x168)-.15)<.000001&&e->step==bh_actor_damage_tick,"blast attenuated impulse");
 memset(packet,0,sizeof(packet));words[0]=2;packet[4]=2;packet[5]=3;packet[6]=4;packet[7]=.5f;bh_actor_damage_query(e,packet);CHECK(fabs(field(e,0x168)-1.15)<.000001&&field(e,0x16c)==1.5f&&field(e,0x170)==2,"direct contact impulse");
 words[0]=11;bh_actor_damage_query(e,packet);CHECK(fabs(field(e,0x19c)-.9)<.000001,"fade event");for(i=0;i<12;++i)bh_actor_damage_query(e,packet);CHECK(field(e,0x19c)==.15f,"fade clamp");
 words[0]=0;packet[1]=1.25f;words[2]=(unsigned long)f.owner;bh_actor_damage_query(e,packet);CHECK(field(e,0x194)==1.25f,"damage accumulation");setptr(e,0x1b8,f.owner);words[0]=17;words[1]=(unsigned long)f.owner;bh_actor_damage_query(e,packet);CHECK(!memcmp((char*)e+0x1b8,"\0\0\0\0",4)&&!memcmp((char*)e+0x1ac,"\0\0\0\0",4),"reference removal");
 c.legacy_death_angles=NULL;
 /* Healthy recovery consumes one probability draw and reinstalls real route tick. */
 setfield(e,0x194,0);setfield(e,0x198,1);setfield(e,0x19c,1);setfield(e,0x15c,0);setfield(e,0x160,0);setfield(e,0x164,0);setfield(e,0x168,0);setfield(e,0x16c,0);setfield(e,0x170,0);used=f.pool.used;srand(99);rand();expected=rand();srand(99);bh_actor_damage_tick(e);CHECK(f.pool.used==used&&rand()==expected&&e->step!=bh_actor_damage_tick,"healthy recovery/RNG");CHECK(bh_entities_step(&f.pool),"restored normal route callback executes");
 /* A surviving damaged actor emits smoke only below the original probability cutoff. */
 for(seed=0;seed<65536;++seed){srand(seed);if(rand()<2000)break;}setfield(e,0x194,.5f);used=f.pool.used;srand(seed);for(i=0;i<7;++i)rand();expected=rand();srand(seed);bh_actor_damage_tick(e);CHECK(f.pool.used==used+1&&rand()==expected,"probability smoke consumes threshold plus six draws");CHECK(c.error==BH_ACTOR_ROUTE_OK&&fabs(field(e,0x194)-.5*.9975)<.000001,"nonlethal damage decays without wreck orientation binding");{BHScriptParticleState *ps=bh_script_particle_state(&f.particles,f.pool.active);CHECK(ps&&ps->resource==15&&ps->lifetime==2&&ps->size==.5f&&ps->alpha==.3f&&ps->acceleration.y==-.002f,"real smoke state contract");}
 /* Strong displacement clamps the nonlinear damping factor and preserves impulse decay. */
 setfield(e,0x15c,100);setfield(e,0x168,30);setfield(e,0x194,0);bh_actor_damage_tick(e);CHECK(field(e,0x15c)==0&&field(e,0x168)==21,"nonlinear clamp and impulse damping");setfield(e,0x168,0);
 /* Mode3 leaves dead actor alive after real smoke; six draws including particle constructor. */
 setfield(e,0x194,1.5f);c.mode=3;used=f.pool.used;srand(71);for(i=0;i<6;++i)rand();expected=rand();srand(71);bh_actor_damage_tick(e);CHECK(f.pool.used==used+1&&field(e,0x194)>1&&rand()==expected,"mode3 suppresses removal while retaining smoke");
 c.mode=0;setfield(e,0x194,1.5f);gen=e->generation;bh_actor_damage_tick(e);CHECK(c.error==BH_ACTOR_ROUTE_ARGUMENT&&e->generation==gen&&e->query==bh_actor_damage_query&&f.room.actors==e&&!f.slots[0].body.entity,"missing death adapter preserves query and room membership");c.legacy_death_angles=&angles;
 c.mode=0;setfield(e,0x194,1.5f);used=f.pool.used;gen=e->generation;seed=71;srand(seed);for(i=0;i<6;++i)rand();n=(rand()&7)+1;for(i=0;i<n*9+2;++i)rand();expected=rand();srand(seed);bh_actor_damage_tick(e);CHECK(rand()==expected,"whole death random ordering");CHECK(e->generation!=gen&&!f.room.actors&&c.error==BH_ACTOR_ROUTE_OK,"original actor removed from room and pool");target=f.slots[0].body.entity;CHECK(target&&f.construct.roster_count==1&&f.pool.used==used+n+1,"real target and fragments replace original");if(target){bh_target_step_read(target,&ts);CHECK(ts.countdown>=1&&ts.countdown<3&&ts.loot==0&&f.slots[0].state.euler.x==angles.x,"whole moving target receives countdown/loot/explicit legacy angles");}
 finish(&f);CHECK(setup(&f),"second fixture");damage_fixture(&f,&c,&route,&angles);e=actor(&f,&c);words[0]=20;bh_actor_damage_query(e,packet);CHECK(!e->query&&e->step&&e->refresh==0,"deferred cleanup installs real owner");CHECK(bh_entities_step(&f.pool)&&!f.room.actors,"real deferred removal");e=actor(&f,&c);words[0]=4;bh_actor_damage_query(e,packet);CHECK(!f.room.actors,"immediate removal owner");finish(&f);remove("actor-damage-fixture/HUN_LOD1.BSP");_rmdir("actor-damage-fixture");bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"actor-damage\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failed);return failed!=0;
}
#ifdef BH_ACTOR_DAMAGE_STANDALONE
int main(void){return bh_actor_damage_checkpoint(0);}
#endif
