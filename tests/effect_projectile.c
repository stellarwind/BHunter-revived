#include "../src/bh_effect_projectile.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL effect-projectile %s\n",n);}}while(0)
typedef struct Fixture{BHEntityPool pool;BHDrawQueue queue;BHSoundQueue sound;BHScriptParticleContext particles;BHFlashFlareContext flash;BHScriptDebrisContext debris;BHHomingProjectileContext effects;BHEffectProjectileContext core;BHEntity*actor;}Fixture;
static void put(BHEntity*e,unsigned long a,const void*p,unsigned long n){memcpy((char*)e+a,p,n);}
static float scalar(BHEntity*e,unsigned long a){float f;memcpy(&f,(char*)e+a,4);return f;}
static unsigned long number(BHEntity*e,unsigned long a){unsigned long n;memcpy(&n,(char*)e+a,4);return n;}
static void word(BHEntity*e,unsigned long a,unsigned long n){put(e,a,&n,4);}
static int fixture(Fixture*f,int orbit){BHSoundResource*r;unsigned long i;memset(f,0,sizeof(*f));if(!bh_entities_init(&f->pool,64,NULL)||!bh_draw_queue_init(&f->queue,128,NULL)||!bh_sound_queue_init(&f->sound,NULL,NULL,NULL)||!bh_script_particles_init(&f->particles,&f->pool,&f->queue))return 0;f->flash.pool=&f->pool;f->flash.queue=&f->queue;f->debris.pool=&f->pool;f->debris.queue=&f->queue;f->debris.image=7;
 f->effects.pool=&f->pool;f->effects.sound=&f->sound;f->effects.particles=&f->particles;f->effects.flash=&f->flash;f->core.effects=&f->effects;f->core.debris=&f->debris;f->core.smoke_image=9;
 for(i=0;i<3;++i){r=bh_sound_resource_allocate(&f->sound);if(!r)return 0;r->streaming=1;r->base_frequency=8000;r->audible_range=1000;}
 f->core.blast_sound=0;f->core.impact_sound=1;f->core.ricochet_sound=2;
 f->actor=bh_entity_acquire(&f->pool);if(!f->actor)return 0;word(f->actor,0x128,BH_SOUND_INVALID);word(f->actor,0x130,0x1234);return bh_effect_projectile_bind(&f->core,f->actor,orbit);
}
static void finish(Fixture*f){CHECK(bh_sound_queue_dispose(&f->sound)&&bh_draw_queue_dispose(&f->queue)&&bh_entities_dispose(&f->pool),"real owners dispose");}
static void packet(BHEffectPacket*p,long event,long id,long kind){memset(p,0,sizeof(*p));p->words[0]=event;p->words[1]=id;p->words[2]=kind;p->floats[4]=10;p->floats[5]=20;p->floats[6]=30;}
static int closef(float a,double b){return fabs(a-b)<.00001;}
int bh_effect_projectile_checkpoint(unsigned long*count){Fixture f;BHEffectPacket p;BHMathEnvironment fp;BHVec3 a={2,3,4},b={5,6,7},velocity={3,0,4};float zero=0,phase=.02f;unsigned long mode,g,voice,i,expected;BHEntity*e;BHScriptParticleState*state;unsigned long total,particles,debris,flash;
 checks=failures=0;CHECK(bh_math_environment_enter(&fp),"PC53 profile");CHECK(fixture(&f,1),"actual orbit fixture");e=f.actor;e->position.x=1;e->position.y=2;e->position.z=3;put(e,0x110,&a,12);put(e,0x11c,&b,12);put(e,0xe8,&zero,4);packet(&p,3,0,0);e->query(e,&p);CHECK(p.floats[1]==3&&p.floats[2]==5&&p.floats[3]==7,"orbit phase zero real query");put(e,0xe8,&phase,4);packet(&p,3,0,0);e->query(e,&p);CHECK(closef(p.floats[1],1+2*cos(1.0)+5*sin(1.0))&&closef(p.floats[2],2+3*cos(1.0)+6*sin(1.0)),"real50radian phase contract");word(e,0x140,17);word(e,0x144,17);packet(&p,17,17,0);e->query(e,&p);CHECK(!number(e,0x140)&&!number(e,0x144),"both actual reference slots detach");
 packet(&p,18,0x999,0);g=e->generation;e->query(e,&p);CHECK(p.words[1]==0x999&&e->generation==g,"unmatched sync no mutations");packet(&p,18,0x1234,7);e->query(e,&p);CHECK(p.words[1]==-1&&e->generation==g,"matched unknown kind acknowledges only");
 for(mode=0;mode<2;++mode){packet(&p,18,0x1234,mode);e->query(e,&p);CHECK(p.words[1]==-1&&e->position.x==10&&e->position.z==30&&scalar(e,0xe8)==0,"both relocation variants actual position/lifetime");CHECK(e->generation==g+(mode==0?1:1),"mode0 increments original common generation only");}
 packet(&p,18,0x1234,4);e->query(e,&p);CHECK(!f.core.flashes&&!f.core.particles,"orbit ignores reflection kind");packet(&p,20,0,0);e->query(e,&p);CHECK(!e->query&&!e->refresh&&e->step==bh_effect_sound_cleanup,"actual deferred cleanup mapping");CHECK(bh_entities_step(&f.pool)&&!f.pool.used,"real deferred cleanup executes");finish(&f);
 CHECK(fixture(&f,0),"linear fixture");e=f.actor;put(e,0xf8,&velocity,12);packet(&p,18,0x1234,4);p.floats[7]=0;p.floats[8]=1;p.floats[9]=0;srand(73);for(i=0;i<6;++i)rand();expected=rand();srand(73);e->query(e,&p);CHECK(p.words[1]==-1&&scalar(e,0xf8)==0&&scalar(e,0xfc)==5&&scalar(e,0x100)==0,"real reflected direction retains speed");CHECK(rand()==expected&&f.core.random_calls==5&&f.core.flashes==1&&f.core.particles==1,"exact5caller plus1realparticle RNG consumption");CHECK(f.pool.used==3,"source flash and particle acquired");finish(&f);
 CHECK(fixture(&f,0),"linear detonation fixture");e=f.actor;voice=bh_sound_request(&f.sound,0,1,1,&e->position,NULL);word(e,0x128,voice);e->origin=velocity;put(e,0x60,&velocity,12);srand(113);for(i=0;i<126;++i)rand();expected=rand();srand(113);packet(&p,18,0x1234,2);e->query(e,&p);CHECK(p.words[1]==-1&&scalar(e,0xe8)==3&&number(e,0x12c)==1&&number(e,0x128)==BH_SOUND_INVALID,"detonation state and loop handle");CHECK(!e->origin.x&&!e->origin.z&&scalar(e,0x60)==0&&scalar(e,0x68)==0,"actual linear and acceleration zeros");CHECK(!bh_sound_voice(&f.sound,voice)&&rand()==expected&&f.core.random_calls==30,"real voice stopped and90debris+36particle draws");particles=debris=flash=total=0;
 for(e=f.pool.active;e;e=e->next){++total;if(e->step==bh_script_particle_tick){++particles;state=bh_script_particle_state(&f.particles,e);CHECK(state&&state->resource==9&&closef(state->size,.5)&&closef(state->alpha,.8)&&closef(state->acceleration.y,-.002),"actual complete particle fields");}else if(e->step==bh_script_debris_tick)++debris;else if(e->step==bh_script_flash_tick)++flash;}
 CHECK(total==18&&particles==6&&debris==10&&flash==1&&f.core.flashes==1&&f.core.particles==6,"exact actual pool population18");finish(&f);bh_math_environment_leave(&fp);if(count)*count+=checks;printf("{\"scenario\":\"effect-projectile\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_EFFECT_PROJECTILE_STANDALONE
int main(void){unsigned long count=0;return bh_effect_projectile_checkpoint(&count);}
#endif
