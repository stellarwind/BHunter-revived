#include "actor_construct_fixture.h"
#include "../src/bh_script_vm.h"
typedef struct VMFixture {
 ResponseFixture response;BHScriptVMContext vm;BHScriptTimerActor states[2];BHScriptVMActor actors[2];
 BHScriptMotionContext motion;BHScriptSensorContext sensors;BHScriptEmitterContext emitters;
 BHActorConstructContext constructors;BHActorConstructRoom constructor_room;BHActorVehicleDrawContext vehicle_draw;
 BHActorLaunchContext launch;BHRadarQueryContext radar;BHScriptVMWaypoint waypoints[8];
 BHRailWagonContext rails;BHRailPaths paths;BHScriptActorConstructorContext script_actors;BHScriptActorDrawContext script_draw;
 BHGeometryModel model;unsigned long models[25],sounds[25],cycle[4];BHEntity *host;
} VMFixture;
static BHVec3 vec(float x,float y,float z){BHVec3 v;v.x=x;v.y=y;v.z=z;return v;}
static int vm_fixture(VMFixture *v){Fixture *f=&v->response.base;BHScriptMotionActor movable;BHVec3 zero={0,0,0},a,b;unsigned long i;memset(v,0,sizeof(*v));if(!response_fixture(&v->response))return 0;
 v->host=bh_entity_acquire(&f->pool);if(!v->host)return 0;v->host->position=zero;v->states[0].entity=v->host;v->states[0].active=1;v->states[1].entity=f->owner;v->states[1].active=1;
 memset(&movable,0,sizeof(movable));identity(&movable.local_to_world);identity(&movable.world_to_local);movable.minimum=vec(-10,-10,-10);movable.maximum=vec(10,10,10);bh_script_motion_actor_write(v->host,&movable);
 v->response.mission.timers.actors=v->states;v->response.mission.timers.actor_count=2;v->vm.lifetime=&v->response.mission;v->motion.lifetime=v->vm.lifetime;v->sensors.lifetime=v->vm.lifetime;v->sensors.player=f->owner;v->emitters.lifetime=v->vm.lifetime;v->emitters.particles=&f->particles;v->vm.motion=&v->motion;v->vm.sensors=&v->sensors;v->vm.emitters=&v->emitters;
 v->vm.explosions=&f->explosions;v->vm.contacts=&f->contact;v->vm.flash=&f->flash;v->vm.targets=&f->construct;v->vm.pickups=&f->pickups;v->vm.actors=v->actors;v->vm.actor_count=2;v->vm.waypoints=v->waypoints;v->vm.waypoint_count=8;
 for(i=0;i<2;++i){v->actors[i].state=v->states+i;v->actors[i].world=&f->world;v->actors[i].resource=1;}
 f->world.minimum=vec(-100,-100,-100);f->world.maximum=vec(100,100,100);f->world.navigation.next=&f->world.navigation;f->world.previous=&f->world;f->world.model=&v->model;v->model.portal_count=2;f->world.navigation.count=2;
 for(i=0;i<2;++i){BHPortal *p=f->world.navigation.portals+i;p->neighbor=&f->world.navigation;p->direction=i?4:0;p->plane=i?100:-100;p->scratch[0]=p->scratch[2]=p->overlap[0]=p->overlap[2]=-100;p->scratch[1]=p->scratch[3]=p->overlap[1]=p->overlap[3]=100;}
 v->constructor_room.room=&f->room;v->constructors.rooms=&v->constructor_room;v->constructors.room_count=1;v->constructors.response=&v->response.response;v->constructors.draw=&v->vehicle_draw;v->constructors.models=v->models;v->constructors.sounds=v->sounds;v->constructors.resource_count=25;v->constructors.cycle_models=v->cycle;v->constructors.cycle_count=4;v->constructors.first=&f->world;v->constructors.world_count=1;v->vehicle_draw.queue=&f->queue;v->vehicle_draw.sound=&f->sound;for(i=0;i<25;++i)v->models[i]=100+i;
 v->launch.actors=&v->constructors;v->launch.mode=0;v->vm.launch=&v->launch;v->radar.pool=&f->pool;v->radar.lifetime=v->vm.lifetime;v->vm.radar=&v->radar;
 v->script_draw.queue=&f->queue;v->script_draw.particles=&f->particles;v->script_actors.callbacks.pool=&f->pool;v->script_actors.callbacks.route=&v->response.route;v->script_actors.callbacks.draw=&v->script_draw;v->script_actors.first_world=&f->world;v->script_actors.world_count=1;v->vm.script_actors=&v->script_actors;
 if(!bh_rail_paths_init(&v->paths,0))return 0;for(i=0;i<12;++i){if(i<3){a=vec(0,0,(float)i*10);b=vec(0,0,(float)(i+1)*10);}else if(i<6){a=vec((float)(i-3)*10,0,30);b=vec((float)(i-2)*10,0,30);}else if(i<9){a=vec(30,0,(float)(9-i)*10);b=vec(30,0,(float)(8-i)*10);}else {a=vec((float)(12-i)*10,0,0);b=vec((float)(11-i)*10,0,0);}if(!bh_rail_add(&v->paths,&a,&b,&zero,&zero))return 0;}if(!bh_rail_link(&v->paths))return 0;
 v->rails.rails=&v->paths;v->rails.pool=&f->pool;v->rails.first_world=&f->world;v->rails.world_count=1;v->rails.rooms=&f->room;v->rails.room_count=1;v->rails.queue=&f->queue;v->rails.sound=&f->sound;v->rails.model=1;v->vm.rails=&v->rails;return 1;
}
static void vm_finish(VMFixture *v){finish(&v->response.base);bh_rail_paths_dispose(&v->paths);CHECK(!v->paths.segments,"real rail source disposal");}
static BHEntity *create(VMFixture *v,long opcode,long a,long b,long c,long d,long e){long args[5],selectors[5]={-1,-1,-1,-1,-1};args[0]=a;args[1]=b;args[2]=c;args[3]=d;args[4]=e;return bh_script_vm_create(&v->vm,0,opcode,args,selectors);}
