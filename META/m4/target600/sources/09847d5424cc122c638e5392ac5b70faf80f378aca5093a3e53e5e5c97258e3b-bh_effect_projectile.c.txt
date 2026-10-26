#include "bh_effect_projectile.h"
#include "bh_round.h"
#include "bh_effect_orbit_tick.h"
#include "bh_effect_split_tick.h"
#include "bh_effect_accelerated.h"
#include "bh_fp.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static const BHVec3 zero={0,0,0};
static void get(const BHEntity*e,unsigned long at,void*out,unsigned long n){memcpy(out,(const char*)e+at,n);}
static void put(BHEntity*e,unsigned long at,const void*in,unsigned long n){memcpy((char*)e+at,in,n);}
static float scalar(BHEntity*e,unsigned long at){float f;get(e,at,&f,4);return f;}
static void value(BHEntity*e,unsigned long at,float f){put(e,at,&f,4);}
static void word(BHEntity*e,unsigned long at,unsigned long n){put(e,at,&n,4);}
static float rf(double d){float f;bh_round_float(&f,d);return f;}
static BHEffectProjectileContext*context(BHEntity*e){BHEffectProjectileContext*c;memcpy(&c,e->payload+608,4);return c;}
static int live(BHEffectProjectileContext*c,BHEntity*e){BHEntity*p;unsigned long i=0,g;memcpy(&g,e->payload+612,4);for(p=c->effects->pool->active;p&&i++<c->effects->pool->capacity;p=p->next)if(p==e)return e->generation==g;return 0;}
int __cdecl bh_effect_projectile_bind(BHEffectProjectileContext*c,BHEntity*e,int orbit){BHEntityStep step;unsigned long handle;
 if(!c||!c->effects||!c->effects->pool||!c->effects->sound||!c->effects->particles||!c->effects->flash||!c->debris||!e)return 0;
 if(c->effects->particles->pool!=c->effects->pool||c->effects->flash->pool!=c->effects->pool||c->debris->pool!=c->effects->pool)return 0;
 c->cleanup.pool=c->effects->pool;c->cleanup.sound=c->effects->sound;step=e->step;get(e,0x128,&handle,4);
 if(!bh_effect_cleanup_bind(&c->cleanup,e,handle))return 0;e->step=step;
 memcpy(e->payload+608,&c,4);memcpy(e->payload+612,&e->generation,4);
 return bh_entity_set_query(c->effects->pool,e,orbit?bh_effect_orbit_query:bh_effect_linear_query);
}
static int random_value(BHEffectProjectileContext*c){++c->random_calls;return rand();}
static void flash(BHEffectProjectileContext*c,BHEntity*e,float strength,float size){BHEntity*f=bh_entity_acquire(c->effects->pool);if(f){f->position=e->position;bh_flash_flare_bind(c->effects->flash,f,.1f,strength,size);++c->flashes;}}
static void particle(BHEffectProjectileContext*c,BHEntity*e,const BHVec3*v,float size,float size_rate,float lifetime,float alpha){BHVec3 gravity={0,-.002f,0};float spin,angle;int r;
 r=random_value(c);spin=rf(((double)r-16384)*(1.0/16384.0));r=random_value(c);angle=rf((double)r*(1.0/512.0));
 if(bh_script_particle_create(c->effects->particles,c->smoke_image,3,&e->position,v,&gravity,size,angle,size_rate,spin,.9f,.95f,lifetime,alpha))++c->particles;
}
static void terminate_effects(BHEffectProjectileContext*c,BHEntity*e){unsigned long handle;BHVec3 v;int i,j;float*o=&v.x;
 value(e,0xe8,3);e->origin=zero;put(e,0x60,&zero,12);get(e,0x128,&handle,4);bh_sound_stop(c->effects->sound,handle);word(e,0x128,BH_SOUND_INVALID);
 bh_sound_request(c->effects->sound,c->blast_sound,4,1,&e->position,&zero);bh_sound_request(c->effects->sound,c->impact_sound,1,1,&e->position,&zero);word(e,0x12c,1);flash(c,e,500,8);
 bh_script_debris_create(c->debris,10,&e->position,2,20,.5f);
 for(i=0;i<6;++i){for(j=0;j<3;++j)o[j]=rf(((double)random_value(c)-16384)*(1.0/262144.0));particle(c,e,&v,.5f,20,1,.8f);}
}
static void detach(BHEffectProjectileContext*c,BHEntity*e,BHEffectPacket*p){unsigned long n;get(e,0x140,&n,4);if(n==(unsigned long)p->words[1])word(e,0x140,0);get(e,0x144,&n,4);if(n==(unsigned long)p->words[1])word(e,0x144,0);(void)c;}
static void retire(BHEffectProjectileContext*c,BHEntity*e){e->step=bh_effect_sound_cleanup;bh_entity_set_query(c->effects->pool,e,NULL);e->refresh=NULL;}
static void synchronize(BHEffectProjectileContext*c,BHEntity*e,BHEffectPacket*p,int orbit){unsigned long id;long kind;BHVec3 v;double speed;int i;float*o=&v.x;
 get(e,0x130,&id,4);if((unsigned long)p->words[1]!=id)return;p->words[1]=-1;kind=p->words[2];
 if(kind==0||kind==1){value(e,0xe8,0);if(!kind){++e->generation;memcpy(e->payload+612,&e->generation,4);}memcpy(&e->position,p->floats+4,12);return;}
 if(kind==2){memcpy(&e->position,p->floats+4,12);terminate_effects(c,e);return;}
 if(kind!=4||orbit)return;
 memcpy(&e->position,p->floats+4,12);get(e,0xf8,&v,12);speed=sqrt(((double)v.x*v.x+(double)v.y*v.y)+(double)v.z*v.z);
 for(i=0;i<3;++i)value(e,0xf8+i*4,rf((double)p->floats[7+i]*speed));
 for(i=0;i<3;++i)o[i]=rf(((double)random_value(c)-16384)*(1.0/262144.0));flash(c,e,100,4);
 bh_sound_request(c->effects->sound,c->ricochet_sound,1,2,&e->position,&zero);particle(c,e,&v,.1f,4,.5f,1);
}
void __cdecl bh_effect_orbit_query(BHEntity*e,void*packet){BHEffectProjectileContext*c=context(e);BHEffectPacket*p=(BHEffectPacket*)packet;float sn,cs;double phase;BHVec3 a,b;if(!c||!p||!live(c,e))return;
 switch(p->words[0]){
 case 3:phase=(double)scalar(e,0xe8)*50;sn=rf(sin(phase));cs=rf(cos(phase));get(e,0x110,&a,12);get(e,0x11c,&b,12);
  p->floats[1]=rf(((double)e->position.x+(double)a.x*cs)+(double)b.x*sn);p->floats[2]=rf(((double)e->position.y+(double)a.y*cs)+(double)b.y*sn);p->floats[3]=rf(((double)e->position.z+(double)a.z*cs)+(double)b.z*sn);break;
 case 17:detach(c,e,p);break;case 18:synchronize(c,e,p,1);break;case 20:retire(c,e);break;default:break;}
}
void __cdecl bh_effect_linear_query(BHEntity*e,void*packet){BHEffectProjectileContext*c=context(e);BHEffectPacket*p=(BHEffectPacket*)packet;if(!c||!p||!live(c,e))return;
 if(p->words[0]==17)detach(c,e,p);else if(p->words[0]==18)synchronize(c,e,p,0);else if(p->words[0]==20)retire(c,e);
}

static void directional(BHEntity*e,void*p){BHEffectProjectileContext*c=context(e);(void)p;if(c&&c->draw)bh_effect_directional_draw(c->draw,e);}
static void variable(BHEntity*e,void*p){BHEffectProjectileContext*c=context(e);(void)p;if(c&&c->draw)bh_effect_variable_draw(c->draw,e);}
static void fixed(BHEntity*e,void*p){BHEffectProjectileContext*c=context(e);(void)p;if(c&&c->draw)bh_effect_fixed_draw(c->draw,e);}
static void scaled(BHEntity*e,unsigned long a,const BHVec3*v,double n){value(e,a,rf((double)v->x*n));value(e,a+4,rf((double)v->y*n));value(e,a+8,rf((double)v->z*n));}
static int constructor_context(BHEffectProjectileContext*c){return c&&c->effects&&c->effects->pool&&c->effects->sound&&c->draw&&c->draw->queue&&c->debris&&c->pending_identity;}
static BHEntity *acquire(BHEffectProjectileContext*c,BHWorldGeometry*world,BHEntity*source,const BHVec3*p,float life,float grace){BHEntity*e=bh_entity_acquire(c->effects->pool);if(!e)return NULL;e->position=*p;value(e,0xe8,life);value(e,0xec,grace);word(e,0x130,c->debris->origin_tag);c->debris->origin_tag=0;put(e,0x144,&source,4);put(e,0x138,&world,4);return e;}
static void identity(BHEffectProjectileContext*c,BHEntity*e){word(e,0x134,*c->pending_identity);*c->pending_identity=0;}
BHEntity *__cdecl bh_effect_orbit_create(BHEffectProjectileContext*c,BHWorldGeometry*w,BHEntity*source,const BHVec3*p,const BHVec3*v,const BHVec3*a,const BHVec3*b,const BHVec3*acc,float life,BHEntity*target){BHEntity*e;unsigned long voice;
 if(!constructor_context(c)||!c->orbit||!c->trails||!p||!v||!a||!b||!acc)return NULL;e=acquire(c,w,source,p,life,1);if(!e)return NULL;
 scaled(e,0xf8,v,1.0/30.0);scaled(e,0x104,acc,1.0/15.0);scaled(e,0x110,a,.1);scaled(e,0x11c,b,.1);word(e,0x128,BH_SOUND_INVALID);
 c->orbit->core=c;if(!bh_effect_orbit_tick_bind(c->orbit,e)){c->error=1;bh_entity_release(c->effects->pool,e);return NULL;}e->refresh=directional;
 bh_trail_create_pair(c->trails,c->trail_image,3,0,p,e,life,2,30,1,0,1,&zero,16,NULL,NULL);voice=bh_sound_request(c->effects->sound,c->loop_sound,1,1,p,v);word(e,0x128,voice);put(e,0x13c,&source,4);put(e,0x140,&target,4);identity(c,e);return e;
}
static BHEntity *split_create(BHEffectProjectileContext*c,BHWorldGeometry*w,BHEntity*source,const BHVec3*p,const BHVec3*v,float life,int count,unsigned long model,float multiplier,int shard){BHEntity*e;BHEntity*ignore;unsigned long voice;
 if(!constructor_context(c)||!c->split||!c->network||!c->effects->segment||!c->effects->contacts||!c->effects->markers||!c->explosion||!c->trails||!p||!v)return NULL;e=acquire(c,w,source,p,life,1);if(!e)return NULL;word(e,0x148,model);scaled(e,0xf8,v,1.0/30.0);word(e,0x128,BH_SOUND_INVALID);
 if(!bh_effect_projectile_bind(c,e,0)){c->error=1;bh_entity_release(c->effects->pool,e);return NULL;}e->step=bh_effect_split_tick;e->refresh=variable;
 voice=shard?BH_SOUND_INVALID:bh_sound_request(c->effects->sound,c->loop_sound,1,.5f,p,v);word(e,0x128,voice);ignore=shard?NULL:source;put(e,0x13c,&ignore,4);word(e,0x12c,(unsigned long)count);value(e,0xf4,multiplier);value(e,0xf0,rf((double)random_value(c)*.0001953125));identity(c,e);return e;
}
BHEntity *__cdecl bh_effect_split_create(BHEffectProjectileContext*c,BHWorldGeometry*w,BHEntity*s,const BHVec3*p,const BHVec3*v,float life,int count,unsigned long model,float multiplier){return split_create(c,w,s,p,v,life,count,model,multiplier,0);}
BHEntity *__cdecl bh_effect_shard_create(BHEffectProjectileContext*c,BHWorldGeometry*w,BHEntity*s,const BHVec3*p,const BHVec3*v,float life,int count,unsigned long model,float multiplier){return split_create(c,w,s,p,v,life,count,model,multiplier,1);}
BHEntity *__cdecl bh_effect_accelerated_create(BHEffectProjectileContext*c,BHWorldGeometry*w,BHEntity*source,const BHVec3*p,const BHVec3*v,const BHVec3*acc){BHEntity*e;unsigned long voice;
 if(!constructor_context(c)||!c->accelerated||!c->accelerated->puffs||!c->network||!c->effects->segment||!c->effects->contacts||!c->effects->markers||!c->explosion||!c->trails||!p||!v||!acc)return NULL;e=acquire(c,w,source,p,0,2);if(!e)return NULL;e->origin=*v;put(e,0x60,acc,12);word(e,0x128,BH_SOUND_INVALID);
 if(!bh_effect_projectile_bind(c,e,0)){c->error=1;bh_entity_release(c->effects->pool,e);return NULL;}e->step=bh_effect_accelerated_tick;e->refresh=fixed;put(e,0x13c,&source,4);value(e,0xf0,rf((double)random_value(c)*.0001953125));word(e,0x12c,0);value(e,0xf4,0);voice=bh_sound_request(c->effects->sound,c->second_loop_sound,0,.5f,p,v);word(e,0x128,voice);identity(c,e);return e;
}

void __cdecl bh_effect_detonate(BHEffectProjectileContext*c,BHEntity*e){terminate_effects(c,e);}
