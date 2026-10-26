#include "bh_effect_puff.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static void get(const BHEntity *e,unsigned long a,void*p,unsigned long n){memcpy(p,(const char*)e+a,n);}
static float scalar(const BHEntity*e,unsigned long a){float f;get(e,a,&f,4);return f;}
static float rf(double x){float f;bh_round_float(&f,x);return f;}
static unsigned long advance(unsigned long x){return x+(((unsigned long)((long)x>>13)*3UL)^(x*7UL));}
static BHEffectPuffContext *context(BHEntity*e){BHEffectPuffContext*c;memcpy(&c,e->payload+624,4);return c;}
int __cdecl bh_effect_puff_bind(BHEffectPuffContext*c,BHEntity*e,int ring){if(!c||!c->queue||!e)return 0;memcpy(e->payload+624,&c,4);e->refresh=ring?bh_effect_puff_ring_draw:bh_effect_puff_random_draw;return 1;}
static void draw(BHEffectPuffContext*c,unsigned long image,const BHVec3*p,const BHVec3*up,const BHVec3*n,float w,float h,float angle,unsigned long control,float alpha){int r;++c->attempted;r=bh_draw_oriented(c->queue,3,image,p,up,n,w,h,angle,control,alpha);if(r>0)++c->queued;else if(r<0)c->error=1;}
void __cdecl bh_effect_puff_random_draw(BHEntity*e,void*ignored){BHEffectPuffContext*c=context(e);float alpha,size,angle;BHVec3 v,p,up={0,-1,0};unsigned long seed,image;int i;(void)ignored;
 if(!c||!(scalar(e,0x198)<=0))return;alpha=rf((double)scalar(e,0xe8)/scalar(e,0xec));size=rf((1.0-alpha)*scalar(e,0x194));get(e,0x1a8,&seed,4);get(e,0xf0,&image,4);
 for(i=0;i<48;++i){seed=advance(seed);v.x=(float)((long)((seed>>16)&32767)-16384);seed=advance(seed);v.y=(float)((long)((seed>>16)&32767)-16384);seed=advance(seed);v.z=(float)((long)((seed>>16)&32767)-16384);seed=advance(seed);bh_vec3_normalize(&v);p.x=rf((double)v.x*size+e->position.x);p.y=rf((double)v.y*size+e->position.y);p.z=rf((double)v.z*size+e->position.z);angle=rf((double)((seed>>16)&32767)*.0001953125);draw(c,image,&p,&up,&v,size,size,angle,8,alpha);}
}
void __cdecl bh_effect_puff_ring_draw(BHEntity*e,void*ignored){BHEffectPuffContext*c=context(e);float alpha,size,angle=0,sn,cs,w,h;BHVec3 p,up,n={0,-1,0};unsigned long image;int i;(void)ignored;
 if(!c||!(scalar(e,0x198)<=0))return;alpha=rf((double)scalar(e,0xe8)/scalar(e,0xec));size=rf((1.0-alpha)*scalar(e,0x194));get(e,0xf0,&image,4);w=rf((double)size*.10799224746714915);h=rf((double)size*(1.0/3.0));
 for(i=0;i<64;++i){sn=rf(sin(angle));cs=rf(cos(angle));angle=rf((double)angle+.09817477042468103);p.x=rf((double)e->position.x+(double)size*cs);p.y=e->position.y;p.z=rf((double)size*sn+e->position.z);up.x=cs;up.y=0;up.z=sn;draw(c,image,&p,&up,&n,w,h,0,12,alpha);}
}
