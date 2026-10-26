#include "../src/bh_weapon_path.h"
#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-path %lu %s\n",checks,s);}}while(0)
int __cdecl bh_weapon_path_checkpoint(unsigned long *count){BHMathEnvironment fp;BHEntityPool pool;BHEntity *weapon,*player,*target;BHWeaponCommonState *s;BHVec3 a={1,2,3},b={4,5,6},c={7,8,9},d={10,11,12},out,alias;float zero=0,one=1,t;unsigned long i;
 CHECK(bh_math_environment_enter(&fp),"PC53");CHECK(bh_entities_init(&pool,4,NULL),"actual pool");weapon=bh_entity_acquire(&pool);player=bh_entity_acquire(&pool);target=bh_entity_acquire(&pool);CHECK(bh_entity_set_query(&pool,target,bh_marker_query),"actual target position receiver");
 bh_weapon_path_blend(&out,&a,&b,&c,&d,0);CHECK(!memcmp(&out,&a,12),"start exact endpoint");bh_weapon_path_blend(&out,&a,&b,&c,&d,1);CHECK(!memcmp(&out,&d,12),"target exact endpoint");bh_weapon_path_blend(&out,&a,&b,&c,&d,.5f);CHECK(out.x==5.5f&&out.y==6.5f&&out.z==7.5f,"balanced interior interpolation");
 for(i=0;i<17;i++){t=(float)i/16;bh_weapon_path_blend(&out,&a,&b,&c,&d,t);alias=a;bh_weapon_path_blend(&alias,&alias,&b,&c,&d,t);CHECK(!memcmp(&out,&alias,12),"actual alias-safe interpolation");}
 s=bh_weapon_common_state(weapon);s->word_108=1;s->hud.target=target;s->hud.charge=48;player->position.x=10;target->position.x=20;memcpy((unsigned char*)player+0x30,&one,4);memcpy((unsigned char*)player+0x28,&one,4);memcpy((unsigned char*)player+0x280,&one,4);
 CHECK(bh_weapon_path_point(weapon,player,&out,0)&&out.x==11&&out.y==.5f&&out.z==0,"actual player side/up start construction");CHECK(bh_weapon_path_point(weapon,player,&out,1)&&out.x==20&&out.y==0,"actual event3 target endpoint");CHECK(bh_weapon_path_point(weapon,player,&out,.5f)&&out.x==40.75f&&out.y==.25f,"whole target-control normalization plus cubic path");
 alias=out;s->word_108=0;CHECK(!bh_weapon_path_point(weapon,player,&out,.5f)&&!memcmp(&out,&alias,12),"eligibility rejection preserves output");s->word_108=1;s->hud.target=NULL;CHECK(!bh_weapon_path_point(weapon,player,&out,0),"NULL target gate");s->hud.target=target;s->hud.charge=24;CHECK(bh_weapon_path_point(weapon,player,&out,.5f)&&!bh_weapon_path_point(weapon,player,&out,.5001f),"signed charge fraction inclusive gate");CHECK(!bh_weapon_path_point(weapon,player,&out,-.1f)&&!bh_weapon_path_point(weapon,player,&out,1.1f),"bounded path progress");s->hud.charge=-1;CHECK(!bh_weapon_path_point(weapon,player,&out,0),"negative charge gate");CHECK(bh_entities_dispose(&pool),"actual disposal");bh_math_environment_leave(&fp);if(count)*count+=checks;return !failures;
}
#ifdef BH_WEAPON_PATH_STANDALONE
int main(void){unsigned long n=0;int ok=bh_weapon_path_checkpoint(&n);printf("weapon-path: %lu checks, %lu failures\n",n,failures);return !ok;}
#endif
