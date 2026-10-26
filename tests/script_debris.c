#include "../src/bh_script_debris.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL script-debris %lu: %s\n",checks,s);}}while(0)
typedef struct Memory{unsigned long calls,fail_at,live;}Memory;
static void *__cdecl allocate(unsigned long n,void*u){Memory*m=(Memory*)u;void*p;if(++m->calls==m->fail_at)return NULL;p=malloc(n);if(p)++m->live;return p;}
static void __cdecl release(void*p,void*u){--((Memory*)u)->live;free(p);}
typedef struct Fixture{BHEntityPool pool;BHDrawQueue queue;BHScriptDebrisContext c;Memory mem;}Fixture;
static int fixture(Fixture*f){BHSceneAllocator a;memset(f,0,sizeof(*f));a.allocate=allocate;a.release=release;a.user=&f->mem;if(!bh_entities_init(&f->pool,4,&a)||!bh_draw_queue_init(&f->queue,16,&a))return 0;f->c.pool=&f->pool;f->c.queue=&f->queue;f->c.image=19;return 1;}
static void finish(Fixture*f){CHECK(bh_draw_queue_dispose(&f->queue)&&bh_entities_dispose(&f->pool)&&!f->mem.live,"all actual queue/pool/allocator ownership released");}
static float number(unsigned long*p){float x;memcpy(&x,p,4);return x;}
static int vec(BHVec3 a,BHVec3 b){return !memcmp(&a,&b,12);}
static void constructor(void){Fixture f;BHVec3 point={1,2,3},expected;BHEntity*e;BHScriptDebrisState s;unsigned long seed,i,next,created,used;int r[9];double factor;volatile double intermediate,reference_speed=30,dt=1.0/30.0;volatile float length,component[3],width,height,angle,spin;
 CHECK(fixture(&f),"actual debris pool and queue");
 for(seed=1;seed<=24;++seed){srand(seed);for(i=0;i<9;++i)r[i]=rand();next=rand();srand(seed);f.c.origin_tag=0x12345678UL;
  CHECK(bh_script_debris_create(&f.c,1,&point,4,30,3)==1&&!f.c.origin_tag,"whole constructor succeeds and clears source attribution");e=f.pool.active;bh_script_debris_read(e,&s);
  CHECK(e->step==bh_script_debris_tick&&e->refresh==bh_script_debris_refresh&&s.origin_tag==0x12345678UL&&s.lifetime==4&&vec(e->position,point),"actual installed step/refresh and retained original fields");
  CHECK((unsigned long)rand()==next&&s.frame==((unsigned long)r[0]&4),"exact9 shared CRT draws and bit4 frame selector");
  length=(float)sqrt((double)(r[1]-16384)*(r[1]-16384)+(double)(r[2]-16384)*(r[2]-16384)+(double)(r[3]-16384)*(r[3]-16384));
  intermediate=.9-(double)r[4]*(-.000006103515625);intermediate=intermediate*reference_speed;intermediate=intermediate*dt;factor=intermediate/length;
  component[0]=(float)(factor*(r[1]-16384));component[1]=(float)(factor*(r[2]-16384));component[2]=(float)(factor*(r[3]-16384));expected.x=component[0];expected.y=component[1];expected.z=component[2];
  CHECK(vec(e->origin,expected),"float-spilled sqrt length and unrounded common normalization factor");
  width=(float)((double)r[5]*3/16384);height=(float)((double)r[6]*3/16384);angle=(float)((double)r[7]/5120);spin=(float)(((double)r[8]-16384)/32768);
  CHECK(s.width==width&&s.height==height&&s.angle==angle&&s.spin==spin,"original width/height/angle/spin argument and store order");bh_entity_release(&f.pool,e);
 }
 f.c.origin_tag=37;CHECK(!bh_script_debris_create(&f.c,0,NULL,1,1,1)&&!f.c.origin_tag,"zero-count resets attribution without dereferencing point");
 f.c.origin_tag=91;used=f.pool.used;CHECK(bh_script_debris_create(&f.c,3,&point,2,5,1)==3&&f.pool.used==used+3&&!f.c.origin_tag,"whole multi-entity loop links actual acquired entities");
 for(e=f.pool.active;e;e=e->next){bh_script_debris_read(e,&s);CHECK(s.origin_tag==91,"all batch entries capture shared unchanged attribution before final reset");}
 while(f.pool.free_slots)CHECK(bh_entity_acquire(&f.pool)!=NULL,"fill real current allocation chunk");
 f.c.origin_tag=73;used=f.pool.used;f.mem.fail_at=f.mem.calls+1;srand(44);next=rand();srand(44);created=bh_script_debris_create(&f.c,2,&point,1,1,1);
 CHECK(!created&&f.pool.used==used&&f.c.origin_tag==73&&f.c.error==BH_DEBRIS_POOL&&(unsigned long)rand()==next,"actual growth failure leaves attribution, prior entities and random state intact");f.mem.fail_at=0;
 CHECK(bh_entity_release(&f.pool,f.pool.active),"release one actual slot to test partially successful constructor batch");used=f.pool.used;f.mem.fail_at=f.mem.calls+1;srand(45);for(i=0;i<9;++i)rand();next=rand();srand(45);
 CHECK(bh_script_debris_create(&f.c,3,&point,1,1,1)==1&&f.pool.used==used+1&&f.c.origin_tag==73&&(unsigned long)rand()==next,"partial batch keeps successful entity and attribution on following real allocation failure");f.mem.fail_at=0;finish(&f);
}
static void physics(void){Fixture f;BHVec3 point={10,20,30},old,expected;BHEntity*e;BHScriptDebrisState s;double y,drag;volatile float ry,xyz[3],angle;unsigned long used,bits=0x7fc01234UL;float nan;BHDrawCommand*q;
 CHECK(fixture(&f),"physics/refresh fixture");CHECK(bh_script_debris_create(&f.c,1,&point,4,1,1)==1,"real constructor supplies persistent tick context");e=f.pool.active;bh_script_debris_read(e,&s);s.angle=1;s.spin=.25f;s.width=8;s.height=12;s.frame=4;bh_script_debris_write(e,&s);e->origin.x=2;e->origin.y=-3;e->origin.z=4;old=e->origin;
 y=(double)old.y+.010911111111111113;ry=(float)y;drag=1.0-sqrt(((double)old.x*old.x+y*ry)+(double)old.z*old.z)*.0025;
 xyz[0]=(float)(drag*old.x);xyz[1]=(float)(drag*y);xyz[2]=(float)(drag*old.z);expected.x=xyz[0];expected.y=xyz[1];expected.z=xyz[2];angle=1.25f;
 e->step(e);bh_script_debris_read(e,&s);CHECK(e->position.x==12&&e->position.y==17&&e->position.z==34,"position uses previous velocity before gravity/drag");CHECK(vec(e->origin,expected)&&s.angle==angle&&s.lifetime<4,"gravity unroundedY times roundedY magnitude; retainedY damped then angle store");
 e->refresh(e,NULL);q=f.queue.commands+9;CHECK(q->kind==BH_DRAW_BILLBOARD&&q->resource==19&&!memcmp(q->words,&e->position,12)&&number(q->words+3)==8&&number(q->words+4)==12&&number(q->words+5)==1.25f&&q->words[7]==4&&number(q->words+6)==1,"whole refresh forwards actual state to real billboard producer");CHECK(f.queue.commands[1].next==9,"original bucket0 producer linkage");
 e->origin.x=1000;e->origin.y=0;e->origin.z=0;e->step(e);CHECK(e->origin.x==500,"drag is floored to.5 for large velocity");
 bh_script_debris_read(e,&s);s.lifetime=.03125f;bh_script_debris_write(e,&s);point=e->position;used=f.pool.used;e->step(e);CHECK(f.pool.used==used-1&&vec(point,e->position),"negative pre-store expiry releases before position/angle changes");
 CHECK(bh_script_debris_create(&f.c,1,&point,1,1,1)==1,"second real lifetime fixture");e=f.pool.active;memcpy(&nan,&bits,4);bh_script_debris_read(e,&s);s.lifetime=nan;bh_script_debris_write(e,&s);used=f.pool.used;e->step(e);CHECK(f.pool.used==used-1,"unordered lifetime follows actual C0 release branch");finish(&f);
}
int __cdecl bh_script_debris_checkpoint(unsigned long*count){BHMathEnvironment env;checks=failures=0;if(!bh_math_environment_enter(&env))return 2;constructor();physics();bh_math_environment_leave(&env);if(count)*count=checks;printf("{\"scenario\":\"script-debris\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);return failures!=0;}
#ifdef BH_SCRIPT_DEBRIS_STANDALONE
int main(void){return bh_script_debris_checkpoint(NULL);}
#endif
