#include "../src/bh_weapon_latch.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
#include "weapon_latch_cases.h"

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-latch: %s\n",s);}}while(0)
int bh_weapon_latch_checkpoint(unsigned long *count){
    BHMathEnvironment fp;BHEntity weapon,expected;BHWeaponCommonState *s,*e;
    unsigned long i,j,n=sizeof(latch_cases)/sizeof(latch_cases[0]);
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    CHECK(!bh_weapon_latch_tick(NULL),"missing allocation rejected without a fabricated state record");
    for(i=0;i<n;++i){
        j=(i*7+3)%n;memset(&weapon,0xa5,sizeof(weapon));s=bh_weapon_common_state(&weapon);
        s->word_f0=latch_cases[i][0];s->word_f4=latch_cases[j][0];expected=weapon;e=bh_weapon_common_state(&expected);
        e->word_f0=latch_cases[i][1];e->word_f4=latch_cases[j][1];
        CHECK(bh_weapon_latch_tick(&weapon) && !memcmp(&weapon,&expected,sizeof(weapon)),
            "independent raw-word timer oracle and complete allocation preservation");
    }
    memset(&weapon,0x5a,sizeof(weapon));s=bh_weapon_common_state(&weapon);
    for(i=0;i<sizeof(latch_sequence)/sizeof(latch_sequence[0]);++i){
        if(!i)s->word_f0=0x3f800000UL;
        s->word_f4=0x80000000UL;
        CHECK(bh_weapon_latch_tick(&weapon) && s->word_f0==latch_sequence[i] && s->word_f4==0x80000000UL,
            "source-built1-second timer crosses below zero without clamp and retains signed zero");
    }
    s->word_f0=0x7fc12345UL;s->word_f4=0xffc54321UL;_clear87();
    CHECK(bh_weapon_latch_tick(&weapon) && s->word_f0==0x7fc12345UL && s->word_f4==0xffc54321UL && (_status87()&_SW_INVALID),
        "unordered comparisons preserve original NaN payloads and masked invalid sticky state");
    _clear87();CHECK(bh_math_environment_supported(),"timer owner preserves caller PC53 environment");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"weapon-latch\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_WEAPON_LATCH_STANDALONE
int main(void){unsigned long count;return bh_weapon_latch_checkpoint(&count);}
#endif
