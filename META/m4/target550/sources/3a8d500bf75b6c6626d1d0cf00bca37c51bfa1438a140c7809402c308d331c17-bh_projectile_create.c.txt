#include "bh_projectile_create.h"
#include "bh_round.h"
#include <string.h>
#include <stdlib.h>
static float scalar(const BHEntity *e,unsigned long o){float f;memcpy(&f,(const unsigned char*)e+o,4);return f;}
static void put(BHEntity *e,unsigned long o,const void *v,unsigned long n){memcpy((unsigned char*)e+o,v,n);}
static float stored(double v){float f;bh_round_float(&f,v);return f;}
static void endpoint(BHVec3 *p,const BHEntity *e,double scale){unsigned int i;for(i=0;i<3;i++)((float*)p)[i]=stored((double)((const float*)&e->position)[i]-(double)((const float*)&e->origin)[i]*scale);}
static void result(BHProjectileDrawContext *c,int n){++c->attempted;if(n>0)++c->queued;else if(n<0)c->error=-1;else if(!c->error)c->error=1;}
int __cdecl bh_projectile_ballistic_draw(BHProjectileDrawContext *c,const BHEntity *e){BHVec3 a,b;if(!c||!c->queue||!e)return -1;c->error=0;
 result(c,bh_flare_queue(c->queue,&e->position,2,50,c->ballistic_flare,0,0,0,0));endpoint(&a,e,-1.0/30.0);endpoint(&b,e,1.0/30.0);
 result(c,bh_draw_segment(c->queue,3,c->ballistic_trail,&a,&b,.3f,.3f,1,1,0));return c->error<0?-1:!c->error;
}
int __cdecl bh_projectile_homing_draw(BHProjectileDrawContext *c,const BHEntity *e){BHVec3 a,b;float angle;int r;if(!c||!c->queue||!e)return -1;c->error=0;
 result(c,bh_flare_queue(c->queue,&e->position,2,50,c->homing_flare,0,0,0,0));endpoint(&a,e,-1.0/30.0);endpoint(&b,e,1.0/30.0);
 result(c,bh_draw_segment(c->queue,3,c->homing_trail,&a,&b,.5f,.5f,1,1,0));r=rand();++c->random_calls;angle=stored((double)r*(1.0/4096.0));
 result(c,bh_draw_billboard(c->queue,3,c->homing_trail,&a,1.5f,1.5f,angle,0,1));return c->error<0?-1:!c->error;
}
int __cdecl bh_projectile_pulse_draw(BHProjectileDrawContext *c,const BHEntity *e){BHVec3 up={0,-1,0};float alpha,size,angle;int r;if(!c||!c->queue||!e)return -1;c->error=0;
 alpha=stored((double)scalar(e,0xe8)*.5);size=scalar(e,0xec);r=rand();++c->random_calls;angle=stored((double)r*(1.0/4096.0));
 result(c,bh_draw_oriented(c->queue,3,c->pulse_image,&e->position,&up,&e->origin,size,size,angle,8,alpha));return c->error<0?-1:!c->error;
}
static BHProjectileDrawContext *draw_context(BHEntity *e){BHProjectileDrawContext *c;memcpy(&c,e->payload+616,4);return c;}
static void __cdecl ballistic_draw(BHEntity *e,void *unused){(void)unused;bh_projectile_ballistic_draw(draw_context(e),e);}
static void __cdecl homing_draw(BHEntity *e,void *unused){(void)unused;bh_projectile_homing_draw(draw_context(e),e);}
static int valid(BHProjectileCreateContext *c,int homing){BHHomingProjectileContext *b;
 if(!c||!c->lifetime||!c->lifetime->pool||!c->lifetime->sound||!c->draw||!c->draw->queue||!c->next_identity)return 0;
 b=homing?c->homing:(c->ballistic?&c->ballistic->bindings:NULL);
 return b&&b->pool==c->lifetime->pool&&b->sound==c->lifetime->sound&&b->particles&&b->contacts&&b->flash&&b->markers&&b->segment;
}
static BHEntity *create(BHProjectileCreateContext *c,BHWorldGeometry *world,BHEntity *source,const BHVec3 *p,const BHVec3 *v,float lifetime,BHEntity *target,int homing){BHEntity *e;BHProjectileQueryState state;unsigned long zero=0;BHEntityPositionPacket packet;
 if(!valid(c,homing)||!p||!v||(target&&!target->query)){if(c)c->error=1;return NULL;}c->error=0;
 e=bh_entity_acquire(c->lifetime->pool);if(!e)return NULL;
 bh_projectile_query_state(e,&state);state.countdown=lifetime;state.identity=*c->next_identity;*c->next_identity=0;state.secondary=source;if(homing)state.target=target;
 e->position=*p;e->origin=*v;
 if(!(homing?bh_homing_projectile_bind(c->homing,e):bh_ballistic_projectile_bind(c->ballistic,e))){c->error=1;bh_entity_release(c->lifetime->pool,e);return NULL;}
 memcpy(e->payload+616,&c->draw,4);e->refresh=homing?homing_draw:ballistic_draw;
 if(homing)state.sound=BH_SOUND_INVALID;
 /* Install actual query before sound requests as in original constructor. */
 if(!bh_projectile_lifetime_bind(c->lifetime,e,&state)){c->error=1;bh_entity_release(c->lifetime->pool,e);return NULL;}
 if(!homing){state.sound=bh_sound_request(c->lifetime->sound,c->loop_sound,1,1.5f,p,v);put(e,0x118,&state.sound,4);bh_sound_request(c->lifetime->sound,c->launch_sound,.75f,4,p,v);}
 put(e,0x128,&source,4);put(e,0x124,&world,4);
 if(homing&&target){packet.event=3;target->query(target,&packet);put(e,0x100,&packet.position,12);}
 put(e,0x120,&zero,4);return e;
}
BHEntity *__cdecl bh_projectile_create_ballistic(BHProjectileCreateContext *c,BHWorldGeometry *w,BHEntity *s,const BHVec3 *p,const BHVec3 *v,float t){return create(c,w,s,p,v,t,NULL,0);}
BHEntity *__cdecl bh_projectile_create_homing(BHProjectileCreateContext *c,BHWorldGeometry *w,BHEntity *s,const BHVec3 *p,const BHVec3 *v,float t,BHEntity *target){return create(c,w,s,p,v,t,target,1);}
