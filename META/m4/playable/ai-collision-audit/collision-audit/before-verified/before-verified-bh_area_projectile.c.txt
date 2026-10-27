#include "bh_area_projectile.h"
#include "bh_callback_helpers.h"
#include "bh_physics_response.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
static const BHVec3 zero={0,0,0};
static void get(const BHEntity *e,unsigned long a,void *p,unsigned long n){memcpy(p,(const unsigned char*)e+a,n);}
static void put(BHEntity *e,unsigned long a,const void *p,unsigned long n){memcpy((unsigned char*)e+a,p,n);}
static float number(const BHEntity *e,unsigned long a){float f;get(e,a,&f,4);return f;}
static float stored(double x){float f;bh_round_float(&f,x);return f;}
static BHAreaProjectileContext *context(const BHEntity *e){BHAreaProjectileContext *c;memcpy(&c,e->payload+600,4);return c;}
static int live(BHAreaProjectileContext *c,BHEntity *e,unsigned long gen){BHEntity *p;unsigned long i=0;for(p=c->effects->pool->active;p&&i++<c->effects->pool->capacity;p=p->next)if(p==e)return e->generation==gen;return 0;}
static void __cdecl draw(BHEntity *e,void *packet){BHAreaProjectileContext *c=context(e);(void)packet;bh_projectile_pulse_draw(c->draw,e);}
static BHEntity *receiver(BHAreaProjectileContext *c,const BHWorldSegmentHit *hit){unsigned long i;if(hit->object.kind!=BH_WORLD_HIT_GEOMETRY)return hit->object.object.actor;for(i=0;i<c->effects->response_count;i++)if(c->effects->responses[i].world==hit->object.object.geometry)return c->effects->responses[i].receiver;return NULL;}
static int packets(BHAreaProjectileContext *c,BHEntity *e,BHEntity *target,const BHVec3 *point,float impulse,double damage){BHPhysicsContactPacket packet;struct{long event;float amount;BHEntity *source;}direct;unsigned long generation,target_generation,suppress;
 if(!target||!target->query){c->error=2;return 0;}generation=e->generation;target_generation=target->generation;memset(&packet,0,sizeof(packet));packet.event=2;packet.point=*point;bh_vec3_normalize_to(&e->origin,&packet.normal);packet.impulse=impulse;packet.severity=0;target->query(target,&packet);
 if(!live(c,e,generation)){c->error=3;return 0;}get(e,0xf4,&suppress,4);if(!suppress){if(!live(c,target,target_generation)||!target->query){c->error=3;return 0;}direct.event=0;direct.amount=stored((double)number(e,0xe8)*damage);get(e,0x104,&direct.source,4);target->query(target,&direct);if(!live(c,e,generation)){c->error=3;return 0;}}return 1;
}
void __cdecl bh_area_projectile_tick(BHEntity *e){BHAreaProjectileContext *c=context(e);BHHomingProjectileContext *b;BHWorldGeometry *cached;BHEntity *ignore,*actor,*target;BHWorldHit self;BHWorldSegmentHit hit;BHEntityPositionPacket sample;BHVec3 end,delta,point;float power,radius;double range,dot,raw_radius,raw_power;unsigned long sound,i,n,generation;int result;
 if(!c||!bh_math_environment_supported())return;b=c->effects;c->error=0;generation=e->generation;
 raw_radius=(double)number(e,0xec)+2.0/3.0;raw_power=(double)number(e,0xe8)*.99;radius=stored(raw_radius);power=stored(raw_power);put(e,0xec,&radius,4);put(e,0xe8,&power,4);if(raw_radius>25){ignore=NULL;put(e,0xfc,&ignore,4);}
 get(e,0xf0,&sound,4);if(raw_power<.1){bh_sound_stop(b->sound,sound);bh_entity_release(b->pool,e);return;}
 end.x=stored((double)e->position.x+(double)e->origin.x/30);end.y=stored((double)e->position.y+(double)e->origin.y/30);end.z=stored((double)e->position.z+(double)e->origin.z/30);
 get(e,0xf8,&cached,4);get(e,0xfc,&ignore,4);self.kind=BH_WORLD_HIT_ACTOR;self.object.actor=ignore;result=bh_world_segment_run(b->segment,ignore?&self:NULL,&cached,&e->position,&end,-1,&hit);if(result<0){c->error=2;return;}put(e,0xf8,&cached,4);
 if(!result)e->position=end;else{dot=(((double)e->origin.y*hit.normal.y+(double)e->origin.z*hit.normal.z)+(double)e->origin.x*hit.normal.x)*(-2);power=stored((double)power*.9);put(e,0xe8,&power,4);e->origin.x=stored((double)hit.normal.x*dot+e->origin.x);e->origin.y=stored((double)hit.normal.y*dot+e->origin.y);e->origin.z=stored((double)hit.normal.z*dot+e->origin.z);e->position=hit.point;target=receiver(c,&hit);if(!packets(c,e,target,&e->position,stored((double)power*(-.4)),.2))return;++c->collisions;}
 range=stored((double)radius*radius*.2);
 if(cached){for(i=0;i<b->segment->room_count;i++)if(b->segment->rooms[i].world==cached)break;if(i==b->segment->room_count){c->error=2;return;}
  actor=b->segment->rooms[i].actors;n=0;while(actor){if(++n>b->segment->max_room_actors||!actor->query){c->error=2;return;}sample.event=3;actor->query(actor,&sample);if(!live(c,e,generation)){c->error=3;return;}delta.x=stored((double)sample.position.x-e->position.x);delta.y=stored((double)sample.position.y-e->position.y);delta.z=stored((double)sample.position.z-e->position.z);
   if(((double)delta.z*delta.z+(double)delta.y*delta.y)+(double)delta.x*delta.x<range){power=number(e,0xe8);bh_sound_request(b->sound,c->hit_sound,power,3,&e->position,&zero);power=stored((double)power*.9);put(e,0xe8,&power,4);if(!packets(c,e,actor,&e->position,16,.15))return;++c->room_hits;}get(actor,0xf4,&actor,4);
  }
 }
 for(i=0;i<b->segment->actor_count;i++){actor=b->segment->actors[i].entity;get(e,0xfc,&ignore,4);if(actor==ignore)continue;delta.x=stored((double)actor->position.x-e->position.x);delta.y=stored((double)actor->position.y-e->position.y);delta.z=stored((double)actor->position.z-e->position.z);
  if(((double)delta.z*delta.z+(double)delta.y*delta.y)+(double)delta.x*delta.x<range){power=number(e,0xe8);bh_sound_request(b->sound,c->hit_sound,stored((double)power+power),3,&e->position,&zero);power=stored((double)power*.9);put(e,0xe8,&power,4);delta.x=-delta.x;delta.y=-delta.y;delta.z=-delta.z;bh_vec3_normalize(&delta);point.x=stored((double)actor->position.x+delta.x);point.y=stored((double)actor->position.y+delta.y);point.z=stored((double)actor->position.z+delta.z);if(!packets(c,e,actor,&point,stored((double)power*(-.4)),.2))return;++c->global_hits;}
 }
 bh_sound_update(b->sound,sound,number(e,0xe8),1,&e->position,&e->origin);
}
BHEntity *__cdecl bh_area_projectile_create(BHAreaProjectileContext *c,BHWorldGeometry *world,const BHVec3 *position,const BHVec3 *velocity,BHEntity *source,float strength){BHEntity *e;float radius=20;unsigned long sound,identity;BHHomingProjectileContext *b;
 if(!c||!position||!velocity||!c->identity||!c->effects||!c->draw||!c->draw->queue||!bh_math_environment_supported())return NULL;b=c->effects;if(!b->pool||!b->sound||!b->segment||(b->response_count&&!b->responses))return NULL;
 e=bh_entity_acquire(b->pool);if(!e)return NULL;identity=*c->identity;*c->identity=0;put(e,0xe8,&strength,4);put(e,0xec,&radius,4);put(e,0xf4,&identity,4);e->position=*position;e->origin.x=stored((double)velocity->x*300);e->origin.y=stored((double)velocity->y*300);e->origin.z=stored((double)velocity->z*300);memcpy(e->payload+600,&c,4);e->step=bh_area_projectile_tick;e->refresh=draw;bh_entity_set_query(b->pool,e,bh_pickup_target_removed);sound=bh_sound_request(b->sound,c->loop_sound,1,1,position,&e->origin);put(e,0xf0,&sound,4);put(e,0xfc,&source,4);put(e,0xf8,&world,4);put(e,0x104,&source,4);return e;
}
