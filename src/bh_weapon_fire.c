#include "bh_weapon_fire.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <stdlib.h>
static const char empty_text[]="5Hello Mister,\r\n\r\nseems like you're\r\nfresh out of ammo\r\non that one!\r\n\r\nCome visit one of my\r\nshops for a refill.\r\n\r\n";
static void get(const BHEntity *e,unsigned long a,void *p,unsigned long n){memcpy(p,(const unsigned char*)e+a,n);}
static void put(BHEntity *e,unsigned long a,const void *p,unsigned long n){memcpy((unsigned char*)e+a,p,n);}
static float stored(double x){float f;bh_round_float(&f,x);return f;}
static int valid(BHWeaponFireContext *c,BHEntity *w,BHEntity *p){return c&&w&&p&&c->sound&&c->transport&&bh_math_environment_supported();}
static unsigned char target_id(BHWeaponFireContext *c,BHEntity *target){unsigned long i;unsigned char id=255;if(!target)return id;if(target==c->local)return (unsigned char)c->local_id;for(i=0;i<8;i++)if(c->remote[i]==target)id=(unsigned char)i;return id;}
void __cdecl bh_weapon_fire_packet(BHWeaponFireContext *c,unsigned char a,unsigned char b,unsigned char d,BHEntity *target){BHPacketWriter writer;unsigned char payload[5];if(c->local_id<0)return;payload[0]=(unsigned char)c->local_id;payload[1]=a;payload[2]=b;payload[3]=d;payload[4]=target_id(c,target);if(bh_packet_begin(&writer,c->packet,sizeof(c->packet),8,c->sender)&&bh_packet_append(&writer,payload,5)&&bh_packet_finish(&writer,&c->packet_bytes)){bh_network_send(c->transport,0,c->packet,c->packet_bytes,0);++c->packets;}else c->error=2;}
int __cdecl bh_weapon_fire_empty(BHWeaponFireContext *c,const BHEntity *p){if(!c->messages||!c->empty_warning){c->error=2;return 0;}bh_sound_request(c->sound,c->empty_sound,.8f,1.5f,&p->position,&p->origin);++c->empty_cues;if(!*c->empty_warning){bh_message_enqueue(c->messages,empty_text);*c->empty_warning=1;}return 1;}
int __cdecl bh_weapon_fire_ballistic(BHWeaponFireContext *c,BHEntity *w,BHEntity *p){BHWeaponCommonState *s;BHVec3 up,side,forward,right,muzzle,velocity,recoil;BHWorldGeometry *world;unsigned long phase,i;int r;double sign,center,launch_x;
 if(!valid(c,w,p)||!c->projectiles)return 0;s=bh_weapon_common_state(w);phase=s->word_e8;c->error=0;if(!(phase&1)){if(!s->ammo)return 1;s->ammo=(long)((unsigned long)s->ammo-1);get(p,0x24,&up,12);get(p,0x30,&side,12);get(p,0x18,&right,12);get(p,0x280,&forward,12);
 /* 0043066c/8b spill without popping: the side offset uses retained centers.
    0043071b/25 retain X as double across rand; Y/Z reload their float spills. */
 for(i=0;i<3;i++){center=((double)((float*)&p->position)[i]-(double)((float*)&up)[i]*(-.5))-(double)((float*)&side)[i]*(-1.5);((float*)&muzzle)[i]=stored((phase&2)?(double)((float*)&right)[i]+center:center-(double)((float*)&right)[i]);}
 launch_x=(double)p->origin.x-(double)forward.x*(-150);
 velocity.y=stored((double)p->origin.y-(double)forward.y*(-150));velocity.z=stored((double)p->origin.z-(double)forward.z*(-150));
 r=rand();velocity.x=stored(launch_x-((double)r-16384)*(-1.0/8192));
 for(i=1;i<3;i++){r=rand();((float*)&velocity)[i]=stored((double)((float*)&velocity)[i]-((double)r-16384)*(-1.0/8192));}
 bh_sound_request(c->sound,c->ballistic_sound,.75f,1.5f,&muzzle,&p->origin);get(p,0xc4,&world,4);bh_projectile_create_ballistic(c->projectiles,world,p,&muzzle,&velocity,2);
 if(c->peers>1&&!s->delay){bh_weapon_fire_packet(c,4,2,1,NULL);s->delay=8;}
 get(p,0x2a4,&recoil,12);for(i=0;i<3;i++)((float*)&recoil)[i]=stored((double)((float*)&recoil)[i]-(double)((float*)&forward)[i]*(-3));put(p,0x2a4,&recoil,12);sign=(phase&2)?-.5:.5;for(i=0;i<3;i++)((float*)&recoil)[i]=stored((double)((float*)&recoil)[i]-(double)((float*)&right)[i]*sign);put(p,0x2a4,&recoil,12);++c->shots;
 }s->word_e8=phase+1;return c->error==0;
}
int __cdecl bh_weapon_fire_homing(BHWeaponFireContext *c,BHEntity *w,BHEntity *p){BHWeaponCommonState *s;BHVec3 up,side,forward,muzzle,velocity,recoil;BHWorldGeometry *world;unsigned long phase,i;int r;
 if(!valid(c,w,p)||!c->projectiles)return 0;s=bh_weapon_common_state(w);phase=s->word_e8;c->error=0;if(!(phase&3)){if(!s->ammo)return 1;s->ammo=(long)((unsigned long)s->ammo-1);bh_sound_request(c->sound,c->homing_sound,.75f,2,&p->position,&p->origin);get(p,0x24,&up,12);get(p,0x30,&side,12);get(p,0x280,&forward,12);
 for(i=0;i<3;i++){((float*)&muzzle)[i]=stored(((double)((float*)&p->position)[i]-(double)((float*)&up)[i]*(-.5))-(double)((float*)&side)[i]*(-1.5));((float*)&velocity)[i]=stored((double)((float*)&p->origin)[i]-(double)((float*)&forward)[i]*(-120));}
 for(i=0;i<3;i++){r=rand();((float*)&velocity)[i]=stored((double)((float*)&velocity)[i]-((double)r-16384)*(-1.0/8192));}
 get(p,0xc4,&world,4);bh_projectile_create_homing(c->projectiles,world,p,&muzzle,&velocity,2,s->hud.target);get(p,0x2a4,&recoil,12);for(i=0;i<3;i++)((float*)&recoil)[i]=stored((double)((float*)&recoil)[i]-(double)((float*)&forward)[i]*4);put(p,0x2a4,&recoil,12);
 if(c->peers>1&&!s->delay){bh_weapon_fire_packet(c,2,4,2,s->hud.target);s->delay=8;}++c->shots;
 }s->word_e8=phase+1;return c->error==0;
}
int __cdecl bh_weapon_area_start(BHWeaponFireContext *c,BHEntity *w,BHEntity *p){BHWeaponCommonState *s;if(!valid(c,w,p))return 0;s=bh_weapon_common_state(w);c->error=0;if(s->word_e8)return 1;if(!s->ammo)return bh_weapon_fire_empty(c,p);s->ammo=(long)((unsigned long)s->ammo-1);s->word_e8=5;if(c->peers>1)bh_weapon_fire_packet(c,5,1,4,NULL);return c->error==0;}
int __cdecl bh_weapon_area_tick(BHWeaponFireContext *c,BHEntity *w,BHEntity *p){BHWeaponCommonState *s;BHVec3 forward,recoil;BHWorldGeometry *world;unsigned long phase,i;float strength;long distance;double gain;if(!valid(c,w,p)||!c->area)return 0;s=bh_weapon_common_state(w);phase=s->word_e8;c->error=0;if(!phase)return 1;if(phase==5){bh_sound_request(c->sound,c->area_sound,1,1,&p->position,&p->origin);bh_sound_request(c->sound,c->area_sound,1,.5f,&p->position,&p->origin);}
 get(p,0x280,&forward,12);distance=(long)(3UL-phase);strength=stored(1-(double)(distance<0?-(double)distance:distance)*.25);get(p,0xc4,&world,4);bh_area_projectile_create(c->area,world,&p->position,&forward,p,strength);gain=((phase&1)-.5)*6;get(p,0x2a4,&recoil,12);for(i=0;i<3;i++)((float*)&recoil)[i]=stored((double)((float*)&forward)[i]*gain+((float*)&recoil)[i]);put(p,0x2a4,&recoil,12);s->word_e8=phase-1;++c->bursts;return 1;
}
