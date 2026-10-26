#include "bh_weapon_arc_fire.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <stdlib.h>
static const BHVec3 zero={0,0,0};
static float stored(double x){float f;bh_round_float(&f,x);return f;}
int __cdecl bh_weapon_arc_fire(BHWeaponArcFireContext *c,BHEntity *w,BHEntity *p){BHWeaponCommonState *s;BHWeaponFireContext *f;BHVec3 point,side,up,recoil;BHEntity *e,*target;long packet[10];unsigned long i;float progress,spin,angle;int r;
 if(!c||!c->fire||!w||!p||!c->particles||!c->flash||c->particles->pool!=c->flash->pool||!bh_math_environment_supported())return 0;f=c->fire;if(!f->sound||!f->transport)return 0;s=bh_weapon_common_state(w);f->error=0;if(s->ammo)s->word_108=1;if(s->word_e8){s->word_e8--;return 1;}if(!s->ammo)return 1;s->ammo=(long)((unsigned long)s->ammo-1);progress=stored((double)s->hud.charge*(1.0/48)-.01);if(progress<0)progress=0;else if(progress>1)progress=1;
 if(!bh_weapon_path_point(w,p,&point,progress)){memcpy(&side,(unsigned char*)p+0x30,12);memcpy(&up,(unsigned char*)p+0x24,12);for(i=0;i<3;i++)((float*)&point)[i]=stored((((double)((float*)&p->position)[i]-(double)((float*)&side)[i]*(-3))-(double)((float*)&p->origin)[i]*(-.1))-(double)((float*)&up)[i]*(-.75));}
 r=rand();spin=stored(((double)r-16384)*(1.0/8192));r=rand();angle=stored((double)r*(1.0/4096));bh_script_particle_create(c->particles,c->first_image,3,&point,NULL,NULL,.8f,angle,10,spin,.8f,1,.75f,1);
 r=rand();spin=stored(((double)r-16384)*(1.0/8192));r=rand();angle=stored((double)r*(1.0/4096));bh_script_particle_create(c->particles,c->second_image,3,&point,NULL,NULL,.4f,angle,16,spin,.8f,1,.65f,1);
 r=rand();angle=stored((double)r*(1.0/4096));bh_script_particle_create(c->particles,c->first_image,3,&point,NULL,NULL,.4f,angle,24,0,.8f,1,.5f,1);
 e=bh_entity_acquire(c->flash->pool);if(e){e->position=point;bh_flash_flare_bind(c->flash,e,.1f,30,2);}target=s->hud.target;if(target&&s->hud.charge>=48){if(!target->query){f->error=2;return 0;}packet[0]=11;target->query(target,packet);++c->queries;}
 memcpy(&recoil,(unsigned char*)p+0x2a4,12);for(i=0;i<3;i++){r=rand();((float*)&recoil)[i]=stored((double)((float*)&recoil)[i]-((double)r-16384)*(-1.0/16384));}memcpy((unsigned char*)p+0x2a4,&recoil,12);bh_sound_request(f->sound,c->sound,.4f,3,&point,&zero);s->word_e8=4;if(f->peers>1)bh_weapon_fire_packet(f,1,0,5,s->hud.target);++c->shots;return f->error==0;
}
