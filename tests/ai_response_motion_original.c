/* Isolated arithmetic prefix439935..439bfe, not a full tick or game test. */
#include "../src/bh_actor_response.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../build/ai-response-motion-match/original-motion.h"
static unsigned long random_state = 0x439910UL;
static unsigned long next_word(void)
{
    random_state = random_state * 1664525UL + 1013904223UL;
    return random_state;
}
static float sample(float scale)
{
    return ((long)(next_word() & 65535) - 32768) * (scale / 32768);
}
static void put(BHEntity *e, unsigned long at, float f) { memcpy((char *)e + at, &f, 4); }
int main(void)
{
    BHMathEnvironment environment;
    BHActorResponseContext context;
    BHActorDamageContext damage;
    BHActorRouteContext route;
    BHEntity original_actor, source_actor;
    unsigned char *code;
    unsigned long i, j, clock, cases = 0, differences = 0, source_errors = 0;
    unsigned long changed[9] = {0,0,0,0,0,0,0,0,0};
    const unsigned long offsets[9] = {0x198,0x184,0x180,0x19c,0x194,0x15c,0x160,0x164,0x1e4};
    const unsigned long scales[6] = {0x3f800000UL,0x3f800001UL,0x3f7fffffUL,0x3f83f573UL,0x3f83f574UL,0x40000000UL};
    void (__cdecl *original)(BHEntity *);
    if (!bh_math_environment_enter(&environment)) return 2;
    memset(&context,0,sizeof(context));memset(&damage,0,sizeof(damage));memset(&route,0,sizeof(route));
    context.damage=&damage;damage.route=&route;route.live_tick=&clock;
    code=VirtualAlloc(0,8192,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if (!code) return 2;
    install_original_motion(code);FlushInstructionCache(GetCurrentProcess(),code,8192);
    original=(void (__cdecl *)(BHEntity *))code;
    for(i=0;i<16384;++i)
    {
        memset(&original_actor,0,sizeof(original_actor));
        clock=next_word();memcpy(code+2048,&clock,4);
        j=i%3==0?6:2;memcpy((char *)&original_actor+0xfc,&j,4);
        put(&original_actor,0x184,40+sample(30));put(&original_actor,0x180,20+sample(10));
        put(&original_actor,0x188,5+sample(4));
        memcpy((char *)&original_actor+0x198,scales+i%6,4);
        put(&original_actor,0x19c,i%4==0?1: .6f+sample(.6f));
        put(&original_actor,0x194,i%8==0?0: .6f+sample(.8f));
        for(j=0;j<3;++j)
        {
            put(&original_actor,0x15c+4*j,sample(i%4==0?80:2));
            put(&original_actor,0x168+4*j,sample(4));
        }
        source_actor=original_actor;context.error=0;
        original(&original_actor);
        if(!bh_actor_response_motion(&source_actor,&context)||context.error)++source_errors;
        ++cases;
        if(memcmp(&source_actor,&original_actor,sizeof(source_actor)))
        {
            ++differences;
            for(j=0;j<9;++j)if(memcmp((char *)&source_actor+offsets[j],(char *)&original_actor+offsets[j],4))++changed[j];
        }
    }
    printf("{\"response_motion_cases\":%lu,\"differences\":%lu,\"source_errors\":%lu,\"scale_duration_phase_fade_damage_xyz_tick_differences\":[",cases,differences,source_errors);
    for(j=0;j<9;++j)printf(j?",%lu":"%lu",changed[j]);
    printf("],\"precision\":\"PC53\",\"full_tick_claimed\":false}\n");
    VirtualFree(code,0,MEM_RELEASE);bh_math_environment_leave(&environment);
    return differences||source_errors?1:0;
}
