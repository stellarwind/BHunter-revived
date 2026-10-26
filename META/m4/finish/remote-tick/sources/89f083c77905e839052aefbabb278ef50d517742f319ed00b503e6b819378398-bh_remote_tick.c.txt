#include "bh_remote_tick.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
static float rf(double v){float f;bh_round_float(&f,v);return f;}
static unsigned long word(BHEntity *e,unsigned long o){unsigned long n;memcpy(&n,(char*)e+o,4);return n;}
static void setword(BHEntity *e,unsigned long o,unsigned long n){memcpy((char*)e+o,&n,4);}
static short half(BHEntity *e,unsigned long o){short n;memcpy(&n,(char*)e+o,2);return n;}
static void sethalf(BHEntity *e,unsigned long o,unsigned short n){memcpy((char*)e+o,&n,2);}
static float number(BHEntity *e,unsigned long o){float f;memcpy(&f,(char*)e+o,4);return f;}
static void real(BHEntity *e,unsigned long o,float f){memcpy((char*)e+o,&f,4);}
int __cdecl bh_remote_tick(BHRemoteTick *c,BHEntity *e){unsigned long i,offset,flags,remaining;short countdown;BHEntity *target;BHVec3 position,start,end,p,velocity,acceleration={0,-.002f,0};BHWorldGeometry *cached;BHWorldHit self;BHWorldSegmentHit hit;int result,n,r,j;float fraction,radius,width,fall,life,angle,stored_angle,spin;double cs,sn,raw_angle;
 if(!c||!e||!c->fire||!c->fire->tick||!bh_math_environment_supported()){if(c)c->error=1;return 0;}c->error=0;++c->calls;
 for(i=0;i<9;++i){offset=0x19c+12*i;if(half(e,offset)){countdown=(short)(unsigned short)((unsigned short)half(e,offset+4)-1);sethalf(e,offset+4,(unsigned short)countdown);if(countdown<1){sethalf(e,offset+4,(unsigned short)((unsigned short)half(e,offset+2)+(unsigned short)countdown));sethalf(e,offset,(unsigned short)((unsigned short)half(e,offset)-1));memcpy(&target,(char*)e+offset+8,4);++c->scheduled;if(!bh_remote_fire(c->fire,e,(short)(unsigned short)*c->fire->tick,i,target)){c->error=2;return 0;}}}}
 remaining=word(e,0x108);if(remaining){flags=*((unsigned char*)e+0x11a);if(flags&6){if(!c->particles||!c->particles->pool||!c->particles->queue||!bh_remote_predict_position(e,(unsigned short)*c->fire->tick,&position)){c->error=1;return 0;}}
  if(flags&2){if(!c->world){c->error=1;return 0;}cached=(BHWorldGeometry*)word(e,0xc4);start=end=position;start.y=rf((double)position.y-(-.3));end.y=rf((double)position.y-(-8.3));self.kind=BH_WORLD_HIT_ACTOR;self.object.actor=e;result=bh_world_segment_run(c->world,&self,&cached,&start,&end,-1,&hit);if(result<0){c->error=3;return 0;}if(result){r=rand();fraction=hit.fraction;if(!_finite(fraction)||fraction<0||fraction>1){c->error=4;return 0;}n=(int)(1-((1-(double)fraction)*r)*(-1.0/8192));radius=rf((1-(double)fraction)*.25);width=rf(2.5-((double)fraction+fraction));fall=rf((1-(double)fraction)*(-.02));life=rf((1-(double)fraction)*.5);
    for(j=0;j<n;++j){r=rand();raw_angle=(double)r*.00019175345108273588;stored_angle=rf(raw_angle);cs=cos(raw_angle);sn=sin((double)stored_angle);velocity.x=rf(cs*radius);velocity.y=0;velocity.z=rf(sn*radius);acceleration.x=rf(cs*radius*(-1.0/15));acceleration.y=fall;acceleration.z=rf(sn*radius*(-1.0/15));p.x=rf((double)rf(cs)*width+hit.point.x);p.y=hit.point.y;p.z=rf((double)rf(sn)*width+hit.point.z);r=rand();angle=rf((double)r*.00019175345108273588);bh_script_particle_create(c->particles,c->smoke_image,3,&p,&velocity,&acceleration,.5f,angle,20,0,.8f,.9f,life,radius);++c->ring_particles;}
  }}
  flags=*((unsigned char*)e+0x11a);if(flags&4){r=rand();if(r<16384){for(i=0;i<3;++i){r=rand();((float*)&velocity)[i]=rf(((double)r-16384)*(1.0/262144));}r=rand();spin=rf(((double)r-16384)*(1.0/16384));r=rand();angle=rf((double)r*(1.0/512));acceleration.x=acceleration.z=0;acceleration.y=-.002f;bh_script_particle_create(c->particles,c->smoke_image,3,&position,&velocity,&acceleration,.5f,angle,12,spin,.9f,.95f,2,.3f);++c->smoke_particles;}}
  remaining=word(e,0x108)-1;setword(e,0x108,remaining);if(!remaining&&word(e,0x104)!=BH_SOUND_INVALID){if(!c->sound){c->error=1;return 0;}bh_sound_stop(c->sound,word(e,0x104));setword(e,0x104,BH_SOUND_INVALID);}
 }
 for(i=0;i<6;++i)real(e,0x60+4*i,rf((double)number(e,0x60+4*i)*.8));for(i=0;i<6;++i)real(e,0x48+4*i,rf((double)number(e,0x48+4*i)*.965));return 1;
}
void __cdecl bh_remote_step(BHEntity *e){BHRemoteTick *c;memcpy(&c,e->payload+540,4);if(c)bh_remote_tick(c,e);}
int __cdecl bh_remote_tick_bind(BHRemoteTick *c,BHEntity *e){BHEntity *at;BHEntityPool *pool;unsigned long n=0;if(!c||!e||!c->fire||!c->fire->pulse||!(pool=c->fire->pulse->effects.pool))return 0;for(at=pool->active;at&&n++<pool->capacity;at=at->next)if(at==e){memcpy(e->payload+540,&c,4);e->step=bh_remote_step;return 1;}return 0;}
