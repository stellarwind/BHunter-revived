#include "bh_weapon_path.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
static float stored(double x){float f;bh_round_float(&f,x);return f;}
void __cdecl bh_weapon_path_blend(BHVec3 *out,const BHVec3 *a,const BHVec3 *b,const BHVec3 *c,const BHVec3 *d,float t){BHVec3 start=*a,control=*b,near_target=*c,target=*d,result;float complement,blend,other;double raw,square,weight,line_a,line_b;unsigned long i;
 raw=1.0-t;complement=stored(raw);square=raw*complement;weight=square*3-(square*complement+square*complement);blend=stored(weight);other=stored(1-weight);
 for(i=0;i<3;i++){line_a=(double)complement*((float*)&start)[i]+(double)t*((float*)&control)[i];line_b=(double)t*((float*)&target)[i]+(double)complement*((float*)&near_target)[i];if(i){line_a=stored(line_a);line_b=stored(line_b);}((float*)&result)[i]=stored(line_a*blend+line_b*other);}*out=result;
}
int __cdecl bh_weapon_path_point(BHEntity *weapon,BHEntity *player,BHVec3 *out,float t){BHWeaponCommonState *s;BHVec3 up,side,heading,start,control,target_control,difference;BHEntityPositionPacket sample;BHEntity *target;unsigned long enabled,i;
 if(!weapon||!player||!out||!bh_math_environment_supported())return 0;s=bh_weapon_common_state(weapon);enabled=s->word_108;target=s->hud.target;
 if(!enabled||!target||!(t>=0)||!(t<=1)||!((double)t<=(double)s->hud.charge*(1.0/48))||!target->query)return 0;
 memcpy(&up,(unsigned char*)player+0x24,12);memcpy(&side,(unsigned char*)player+0x30,12);memcpy(&heading,(unsigned char*)player+0x280,12);
 for(i=0;i<3;i++){((float*)&start)[i]=stored(((double)((float*)&player->position)[i]+((float*)&side)[i])-(double)((float*)&up)[i]*(-.5));((float*)&control)[i]=stored((double)((float*)&start)[i]-(double)((float*)&heading)[i]*(-100));}
 sample.event=3;target->query(target,&sample);difference.x=stored((double)sample.position.x-player->position.x);difference.y=stored((double)sample.position.y-player->position.y);difference.z=stored((double)sample.position.z-player->position.z);bh_vec3_normalize(&difference);
 target_control.x=stored((double)difference.x+sample.position.x);target_control.y=stored((double)difference.y+sample.position.y);target_control.z=stored((double)difference.z+sample.position.z);bh_weapon_path_blend(out,&start,&control,&target_control,&sample.position,t);return 1;
}
