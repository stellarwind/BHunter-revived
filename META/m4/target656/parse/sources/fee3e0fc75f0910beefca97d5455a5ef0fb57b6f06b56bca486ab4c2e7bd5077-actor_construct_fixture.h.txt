#include "actor_response_fixture.h"
#include "../src/bh_actor_response.h"
static float field(BHEntity *e,unsigned long o){float f;memcpy(&f,(char*)e+o,4);return f;}
static void real(BHEntity *e,unsigned long o,float f){memcpy((char*)e+o,&f,4);}
static void setword(BHEntity *e,unsigned long o,unsigned long w){memcpy((char*)e+o,&w,4);}
static unsigned long word(BHEntity *e,unsigned long o){unsigned long w;memcpy(&w,(char*)e+o,4);return w;}
static unsigned long rows[4][32];
typedef struct ResponseFixture {
 Fixture base;BHActorResponseContext response;BHActorDamageContext damage;
 BHActorAttackContext attack;BHActorPursuitRouteContext pursuit;
 BHActorRouteContext route;BHActorVehicleLifetimeContext vehicles;
 BHScriptLifetimeContext mission;BHEffectProjectileContext effects;
 BHHomingProjectileContext homing;BHProjectileCreateContext projectiles;
 BHAreaProjectileContext area;BHVec3 angles;
} ResponseFixture;
static int response_fixture(ResponseFixture *r){Fixture *f=&r->base;memset(r,0,sizeof(*r));if(!setup(f))return 0;
 r->route.rooms=&f->room;r->route.room_count=1;r->route.sound=&f->sound;r->route.actor_limit=128;
 r->damage.pool=&f->pool;r->damage.route=&r->route;r->damage.particles=&f->particles;r->damage.fragments=&f->fragments;r->damage.targets=&f->construct;r->damage.loot_rows=rows;r->damage.loot_row_count=4;r->damage.smoke_resource=15;r->damage.legacy_death_angles=&r->angles;
 r->pursuit.route=&r->route;r->pursuit.player=f->owner;r->effects.effects=&r->homing;r->homing.pool=&f->pool;r->attack.pursuit=&r->pursuit;r->attack.effects=&r->effects;r->attack.projectiles=&r->projectiles;r->attack.area=&r->area;
 r->vehicles.pool=&f->pool;r->vehicles.route=&r->route;r->mission.timers.pool=&f->pool;r->mission.sounds=&f->sound;r->response.damage=&r->damage;r->response.attack=&r->attack;r->response.vehicles=&r->vehicles;r->response.mission=&r->mission;r->response.player=f->owner;setword(f->owner,0x238,1);setword(f->owner,0x234,100);return 1;
}
static BHEntity *actor(ResponseFixture *r,unsigned long type){Fixture *f=&r->base;BHActorRouteState s;BHEntity *e=bh_entity_acquire(&f->pool),*head=f->room.actors;unsigned long zero=0;
 memset(&s,0,sizeof(s));s.current=s.next=&f->world;s.duration=1;s.duration_ticks=30;s.phase=.5f;s.speed=10;s.speed_scale=1;s.sound=BH_SOUND_INVALID;s.controls[1].x=1;s.controls[2].x=1;s.controls[3].x=2;bh_actor_route_write(e,&s);
 memset((char*)e+0x15c,0,24);real(e,0x194,0);real(e,0x19c,1);setword(e,0xfc,type);setword(e,0x100,100);setword(e,0x118,0);setword(e,0x11c,0);setword(e,0x1b8,0);setword(e,0x1ac,0);setword(e,0x1d4,zero);setword(e,0x1dc,0xffffffffUL);
 setword(e,0xf4,(unsigned long)head);setword(e,0xf8,0);if(head)setword(head,0xf8,(unsigned long)e);f->room.actors=e;
 setword(e,0xf0,(unsigned long)r->vehicles.head);setword(e,0xec,0);if(r->vehicles.head)setword(r->vehicles.head,0xec,(unsigned long)e);r->vehicles.head=e;++r->vehicles.count;
 bh_actor_vehicle_tick_bind(&r->attack,e);bh_actor_response_bind(&r->response,e);return e;
}
