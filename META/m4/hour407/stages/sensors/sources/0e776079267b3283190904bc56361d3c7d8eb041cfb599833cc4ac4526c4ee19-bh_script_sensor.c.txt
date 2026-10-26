#include "bh_script_sensor.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
typedef char sensor_layout[sizeof(void*)==4&&offsetof(BHEntity,payload)==0xe4?1:-1];
typedef struct SensorPacket {long event;BHEntity *actor;long expected;unsigned char unused[28];} SensorPacket;
typedef char packet_layout[sizeof(SensorPacket)==40&&offsetof(SensorPacket,expected)==8?1:-1];
static void get(const BHEntity *e,unsigned long at,void *out){memcpy(out,(const unsigned char*)e+at,4);}
static void put(BHEntity *e,unsigned long at,const void *value){memcpy((unsigned char*)e+at,value,4);}
static BHScriptSensorContext *context(BHEntity *e){BHScriptSensorContext *c;memcpy(&c,e->payload+416,4);return c;}
static unsigned long generation(BHEntity *e){unsigned long n;memcpy(&n,e->payload+420,4);return n;}
static int live(BHEntityPool *pool,const BHEntity *e,unsigned long generation){BHEntity *p;unsigned long n=0;
 for(p=pool->active;p&&n++<pool->capacity;p=p->next)if(p==e)return p->generation==generation;return 0;
}
static int present(BHEntityPool *pool,const BHEntity *e){BHEntity *p;unsigned long n=0;
 for(p=pool->active;p&&n++<pool->capacity;p=p->next)if(p==e)return 1;return 0;
}
static const BHScriptTimerActor *record(BHScriptSensorContext *c,BHEntity *e){unsigned long i;
 for(i=0;i<c->lifetime->timers.actor_count;++i)if(c->lifetime->timers.actors[i].entity==e)return c->lifetime->timers.actors+i;return NULL;
}
/* VC5 may reverse a comparison to a bare C0 test and thereby accept unordered.
   Explicit NaN exclusions preserve the reviewed ordered x87 branches. */
static int greater(double a,double b){return !_isnan(a)&&!_isnan(b)&&a>b;}
static int greater_equal(double a,double b){return !_isnan(a)&&!_isnan(b)&&a>=b;}
int __cdecl bh_script_sensor_read(BHEntity *e,BHScriptSensorState *s){if(!e||!s)return 0;
 get(e,0xf0,&s->actor);get(e,0xf4,&s->script_loaded);get(e,0xfc,&s->remaining);
 get(e,0x100,&s->threshold);get(e,0x104,&s->broadcast_radius);get(e,0x108,&s->expected);
 get(e,0x128,&s->countdown);get(e,0x12c,&s->latch);return 1;
}
float __cdecl bh_script_sensor_damage(const BHEntity *e){float value=0;if(e)get(e,0x148,&value);return value;}
void __cdecl bh_script_sensor_set_damage(BHEntity *e,float value){if(e)put(e,0x148,&value);}
static int valid(BHScriptSensorContext *c,BHEntity *e,const BHScriptSensorState *s,int bound){unsigned long g;BHEntityPool *pool;
 if(!c||!c->lifetime||!c->lifetime->timers.pool||!e||!s||(c->radar_count&&!c->radar)||c->radar_count>65535){if(c)c->error=BH_SENSOR_ARGUMENT;return 0;}
 pool=c->lifetime->timers.pool;g=bound?generation(e):e->generation;
 if(!live(pool,e,g)||!record(c,s->actor)||!present(pool,s->actor)){c->error=BH_SENSOR_ACTOR;return 0;}
 if(!bh_math_environment_supported()){c->error=BH_SENSOR_NUMERICAL;return 0;}
 c->error=BH_SENSOR_OK;return 1;
}
int __cdecl bh_script_sensor_bind(BHScriptSensorContext *c,BHEntity *e,const BHScriptSensorState *s,int damage){BHScriptLifetimeState base;
 if(!valid(c,e,s,0))return 0;
 bh_script_lifetime_read(e,&base);base.actor=s->actor;base.script_loaded=s->script_loaded;
 memcpy(&base.sound,&s->remaining,4);base.countdown=s->countdown;
 if(!bh_script_lifetime_bind(c->lifetime,e,&base,bh_script_sensor_query)){c->error=BH_SENSOR_ARGUMENT;return 0;}
 put(e,0x100,&s->threshold);put(e,0x104,&s->broadcast_radius);put(e,0x108,&s->expected);put(e,0x12c,&s->latch);
 memcpy(e->payload+416,&c,4);memcpy(e->payload+420,&e->generation,4);
 e->step=damage?bh_script_damage_tick:bh_script_proximity_tick;return 1;
}
/* FST of the subtraction precedes the branch but the add-period uses the
   retained, unrounded x87 result. Unordered follows the due branch. */
static int due(BHEntity *e,float old,double period){double value=(double)old-(1.0/30.0);float next;
 bh_round_float(&next,value);put(e,0x128,&next);if(greater(value,0))return 0;
 bh_round_float(&next,value+period);put(e,0x128,&next);return 1;
}
static int broadcast(BHScriptSensorContext *c,BHEntity *e,BHEntity *actor,long radius,long expected,long event){SensorPacket packet;float r;unsigned long g=generation(e);int result;BHEntity *after;
 memset(&packet,0,sizeof(packet));packet.event=event;packet.actor=actor;packet.expected=expected;
 bh_round_float(&r,(double)radius);
 result=bh_entities_query(c->lifetime->timers.pool,0,0,event==17?NULL:&actor->position,event==17?0:r,&packet);
 if(!live(c->lifetime->timers.pool,e,g)){c->error=BH_SENSOR_MUTATION;return 0;}
 get(e,0xf0,&after);if(after!=actor){c->error=BH_SENSOR_MUTATION;return 0;}
 if(!result){c->error=BH_SENSOR_QUERY;return 0;}return 1;
}
static int decrease(BHScriptSensorContext *c,BHEntity *e,int final_event){long remaining;unsigned long bits;
 get(e,0xfc,&remaining);if(!remaining)return 0;bits=(unsigned long)remaining-1UL;memcpy(&remaining,&bits,4);put(e,0xfc,&remaining);
 if(remaining)return 0;
 if(final_event){BHEntity *actor;get(e,0xf0,&actor);if(!broadcast(c,e,actor,0,0,17))return 1;}
 bh_entity_release(c->lifetime->timers.pool,e);return 1;
}
static int in_box(const BHEntity *e,const BHEntity *p){BHVec3 low,high;
 memcpy(&low,(const unsigned char*)e+0x1b0,12);memcpy(&high,(const unsigned char*)e+0x1bc,12);
 /* Retain the original unequal C0 versus C0|C3 tests, including asymmetric
    unordered bound behavior. Ordinary six ordered <= comparisons differ. */
 return greater_equal(p->position.x,low.x)&&!greater(p->position.x,high.x)&&!greater(low.y,p->position.y)&&greater_equal(high.y,p->position.y)&&!greater(low.z,p->position.z)&&greater_equal(high.z,p->position.z);
}
static int nearby(BHEntity *a,BHEntity *b,long threshold){double x=(double)a->position.x-b->position.x,y=(double)a->position.y-b->position.y,z=(double)a->position.z-b->position.z;float rx,rz;
 bh_round_float(&rx,x);bh_round_float(&rz,z);return greater((double)threshold,(x*rx+y*y)+z*rz);
}
void __cdecl bh_script_proximity_tick(BHEntity *e){BHScriptSensorContext *c=context(e);BHScriptSensorState s;unsigned long i;const BHScriptTimerActor *owner;BHEntity *bounds;float latch=0;
 bh_script_sensor_read(e,&s);if(!valid(c,e,&s,1))return;
 if(!due(e,s.countdown,.25))return;
 for(i=0;i<c->radar_count;++i){BHEntity *p=c->radar[i];int inside;
  if(p!=c->player)continue;
  if(!p||!present(c->lifetime->timers.pool,p)){c->error=BH_SENSOR_ACTOR;return;}
  get(e,0x100,&s.threshold);get(e,0x104,&s.broadcast_radius);get(e,0x108,&s.expected);
  if(s.threshold>0)inside=nearby(p,s.actor,s.threshold);
  else{owner=record(c,s.actor);if(owner->owner)owner=owner->owner;bounds=owner->entity;
   if(!bounds||!present(c->lifetime->timers.pool,bounds)){c->error=BH_SENSOR_ACTOR;return;}inside=in_box(bounds,p);}
  if(inside){get(e,0x12c,&latch);if(greater(latch,0))return;latch=1;put(e,0x12c,&latch);
   if(!broadcast(c,e,s.actor,s.broadcast_radius,s.expected,14))return;
   if(decrease(c,e,0))return;
  }
 }
 latch=0;put(e,0x12c,&latch);
}
void __cdecl bh_script_damage_tick(BHEntity *e){BHScriptSensorContext *c=context(e);BHScriptSensorState s;double scaled;float damage,countdown,latch=0;
 bh_script_sensor_read(e,&s);if(!valid(c,e,&s,1))return;
 if(greater(s.latch,0))bh_script_sensor_set_damage(s.actor,0);
 if(!due(e,s.countdown,1))return;
 put(e,0x12c,&latch);scaled=(double)bh_script_sensor_damage(s.actor)*.98;
 bh_round_float(&damage,scaled);bh_script_sensor_set_damage(s.actor,damage);
 if(!greater(scaled,(double)s.threshold))return;
 if(!broadcast(c,e,s.actor,s.broadcast_radius,s.expected,14))return;
 get(e,0x128,&countdown);bh_round_float(&countdown,(double)countdown+4);put(e,0x128,&countdown);
 latch=1;put(e,0x12c,&latch);bh_script_sensor_set_damage(s.actor,0);
 decrease(c,e,1);
}
