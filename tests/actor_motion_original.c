/* Isolated motion-query/curve reference closure; never linked into the game. */
#include "../src/bh_actor_response.h"
#include "../src/bh_camera_curve.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "../build/ai-motion/original.h"
int main(int argc,char **argv)
{
    BHEntityPool pool; BHActorResponseContext response; BHActorDamageContext damage; BHActorRouteContext route; BHActorVehicleLifetimeContext vehicles; BHActorAttackContext attack;
    BHMathEnvironment env;
    BHActorRouteState s;
    BHEntity *e;
    unsigned char *code;
    void (__cdecl *original)(BHEntity *, void *);
    unsigned long state=1979, i,j,bad=0,curve_bad=0,box_cases=0,box_bad=0;
    float expected[10],actual[10];
    BHVec3 reference_point,source_point;
    void (__cdecl *original_curve)(float,BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*);
    FILE *gold;
    memset(&response,0,sizeof(response));memset(&damage,0,sizeof(damage));
    memset(&route,0,sizeof(route));memset(&vehicles,0,sizeof(vehicles));memset(&attack,0,sizeof(attack));
    if(!bh_math_environment_enter(&env)||!bh_entities_init(&pool,32,0))return 2;
    vehicles.pool=&pool;vehicles.route=&route;damage.pool=&pool;damage.route=&route;
    response.damage=&damage;response.attack=&attack;response.vehicles=&vehicles;
    code=VirtualAlloc(0,16384,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
    if(!code)return 2;
    install_original(code);
    FlushInstructionCache(GetCurrentProcess(),code,16384);
    original=(void (__cdecl *)(BHEntity*,void*))code;
    original_curve=(void (__cdecl *)(float,BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*,const BHVec3*))(code+4096);
    e=bh_entity_acquire(&pool);if(!e||!bh_actor_response_bind(&response,e))return 2;memset(&s,0,sizeof(s));
    gold=fopen("motion-golden.csv","w");if(!gold)return 2;
    for(i=0;i<4096;++i)
    {
        bh_actor_route_read(e,&s);
        for(j=0;j<12;++j){state=state*1664525UL+1013904223UL;((float*)s.controls)[j]=(float)((long)(state>>8)-8388608)/65536.0f;}
        state=state*1664525UL+1013904223UL;s.duration=(float)(1+(state&65535))/8192.0f;
        state=state*1664525UL+1013904223UL;s.phase=(float)(state&65535)/8192.0f;
        if(i==0){s.phase=.5f;s.duration=1;}
        bh_actor_route_write(e,&s);
        for(j=0;j<3;++j){state=state*1664525UL+1013904223UL;{float v=(float)((long)(state>>8)-8388608)/524288.0f;memcpy((char*)e+0x15c+j*4,&v,4);}}
        memset(expected,0,sizeof(expected));memset(actual,0,sizeof(actual));
        ((long*)expected)[0]=((long*)actual)[0]=16;
        original(e,expected);bh_actor_response_query(e,actual);
        original_curve(s.phase/s.duration,&reference_point,s.controls,s.controls+1,s.controls+2,s.controls+3);
        bh_camera_curve(s.phase/s.duration,&source_point,s.controls,s.controls+1,s.controls+2,s.controls+3);
        if(memcmp(&reference_point,&source_point,12))++curve_bad;
        if(response.error||memcmp(expected+1,actual+1,36)){
            if(bad<4){printf("DIFF %lu",i);for(j=1;j<10;++j)printf(" %08lx/%08lx",((unsigned long*)expected)[j],((unsigned long*)actual)[j]);puts("");}++bad;
        }
        fprintf(gold,"%lu",i);for(j=1;j<10;++j)fprintf(gold,",%08lx",((unsigned long*)expected)[j]);fputs("\n",gold);
    }
    fclose(gold);
    if(argc>1){
        struct {unsigned long tick,actor;float phase,duration;BHVec3 controls[4],offset;float motion[10];} sample;
        FILE *capture=fopen(argv[1],"rb");if(!capture)return 2;
        while(fread(&sample,sizeof(sample),1,capture)==1){
            s.phase=sample.phase;s.duration=sample.duration;memcpy(s.controls,sample.controls,sizeof(s.controls));
            bh_actor_route_write(e,&s);memcpy((char*)e+0x15c,&sample.offset,12);
            memset(expected,0,sizeof(expected));((long*)expected)[0]=16;
            original(e,expected);++box_cases;
            if(memcmp(expected+1,sample.motion+1,36))++box_bad;
        }
        if(ferror(capture)||ftell(capture)%sizeof(sample))return 2;
        fclose(capture);if(!box_cases)return 2;
    }
    VirtualFree(code,0,MEM_RELEASE);bh_entities_dispose(&pool);bh_math_environment_leave(&env);
    printf("{\"cases\":4096,\"motion_differences\":%lu,\"curve_differences\":%lu,\"box_cases\":%lu,\"box_differences\":%lu,\"precision\":53}\n",bad,curve_bad,box_cases,box_bad);
    return bad||curve_bad||box_bad?1:0;
}
