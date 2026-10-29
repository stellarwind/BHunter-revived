/* Isolated route/smoke window with real pursuit, particle, pool and RNG owners. */
#include "../src/bh_actor_response.h"
#include "../src/bh_fp.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static BHActorResponseContext *active_context;
static unsigned long pursuit_calls;
static void __cdecl oracle_pursuit(BHEntity *e)
{
    ++pursuit_calls;
    if(!bh_actor_pursuit_route(active_context->attack->pursuit,e))exit(3);
}
static BHEntity *__cdecl oracle_particle(unsigned long resource,unsigned long bucket,
    const BHVec3 *p,const BHVec3 *v,const BHVec3 *a,float size,float angle,float rate,
    float spin,float decay,float angular_decay,float life,float alpha)
{
    return bh_script_particle_create(active_context->damage->particles,resource,bucket,p,v,a,
        size,angle,rate,spin,decay,angular_decay,life,alpha);
}
#include "../build/ai-response-smoke-match/original-smoke.h"
typedef struct Result
{
    unsigned char actor[0x200];
    BHVec3 death_position,particle_position;
    BHScriptParticleState particle;
    unsigned long emitted,allocated,next_random;
}Result;
static void set_float(BHEntity *e,unsigned long at,float v){memcpy((char*)e+at,&v,4);}
static void set_word(BHEntity *e,unsigned long at,unsigned long v){memcpy((char*)e+at,&v,4);}
static unsigned long rng=0x439bfeUL;
static float sample(void){rng=rng*1664525UL+1013904223UL;return ((long)(rng&65535)-32768)/8192.0f;}
static void collect(Result *out,BHEntity *e,BHEntityPool *pool,unsigned long emitted)
{
    BHEntity *particle=pool->active;
    memcpy(out->actor,(char*)e+0xe8,sizeof(out->actor));out->emitted=emitted;out->next_random=rand();
    if(particle!=e&&particle->step==bh_script_particle_tick)
    {
        out->allocated=1;out->particle_position=particle->position;
        out->particle=*bh_script_particle_state(active_context->damage->particles,particle);
        bh_entity_release(pool,particle);
    }
}
int main(void)
{
    BHEntityPool pool;BHDrawQueue queue;BHScriptParticleContext particles;
    BHActorResponseContext context;BHActorDamageContext damage;BHActorAttackContext attack;
    BHActorPursuitRouteContext pursuit;BHActorRouteContext route;BHNavigation navigation;
    BHWorldGeometry world;BHActorRouteState state;BHMathEnvironment environment;
    BHEntity *e;unsigned char *code;unsigned char initial[sizeof(BHEntity)];
    Result source,reference;unsigned long i,j,seed,code_return,cases=0,bad=0,errors=0,smoke=0,skipped=0,transitions=0;
    int result;int (__cdecl *original)(BHEntity *,BHVec3 *);
    if(!bh_math_environment_enter(&environment))return 2;
    memset(&context,0,sizeof(context));memset(&damage,0,sizeof(damage));memset(&attack,0,sizeof(attack));
    memset(&pursuit,0,sizeof(pursuit));memset(&route,0,sizeof(route));memset(&navigation,0,sizeof(navigation));memset(&world,0,sizeof(world));
    world.minimum.x=world.minimum.y=world.minimum.z=-100;world.maximum.x=world.maximum.y=world.maximum.z=100;
    if(!bh_entities_init(&pool,8,NULL)||!bh_draw_queue_init(&queue,16,NULL)||!bh_script_particles_init(&particles,&pool,&queue))return 2;
    context.damage=&damage;context.attack=&attack;damage.route=&route;damage.particles=&particles;damage.smoke_resource=15;
    attack.pursuit=&pursuit;pursuit.route=&route;pursuit.navigation=&navigation;route.tick=1234;active_context=&context;
    e=bh_entity_acquire(&pool);code=VirtualAlloc(0,8192,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!e||!code)return 2;
    install_original_smoke(code);memcpy(code+3072,&damage.smoke_resource,4);FlushInstructionCache(GetCurrentProcess(),code,8192);
    original=(int (__cdecl *)(BHEntity *,BHVec3 *))code;
    for(i=0;i<8192;++i)
    {
        memset(&state,0,sizeof(state));state.current=state.next=&world;state.speed=10;state.speed_scale=1;
        state.duration=1;state.phase=i%4==0?1.25f:.5f;state.sound=BH_SOUND_INVALID;
        for(j=0;j<12;++j)((float*)state.controls)[j]=sample();
        bh_actor_route_write(e,&state);set_word(e,0xfc,i%2?6:2);set_word(e,0x1d4,i%4==0?1:0);
        for(j=0;j<6;++j)set_float(e,0x1bc+4*j,sample());
        for(j=0;j<3;++j)set_float(e,0x15c+4*j,sample());
        set_float(e,0x194,i%5==0?1.5f:i%5==1?0:i%5==2?.5f:i%5==3?.02f:1);
        memcpy(initial,e,sizeof(*e));seed=i*19+7;memset(&source,0,sizeof(source));memset(&reference,0,sizeof(reference));
        srand(seed);context.error=0;result=bh_actor_response_route_smoke(e,&context,&source.death_position);
        if(result<0||context.error)++errors;collect(&source,e,&pool,result>0);
        memcpy(e,initial,sizeof(*e));srand(seed);pursuit_calls=0;code_return=original(e,&reference.death_position);
        collect(&reference,e,&pool,code_return);transitions+=pursuit_calls;
        ++cases;if(source.emitted)++smoke;else ++skipped;
        if(memcmp(&source,&reference,sizeof(source)))++bad;
    }
    printf("{\"route_smoke_cases\":%lu,\"differences\":%lu,\"source_errors\":%lu,\"smoke\":%lu,\"skipped\":%lu,\"real_pursuit_transitions_each_side\":%lu,\"precision\":\"PC53\",\"full_tick_claimed\":false}\n",cases,bad,errors,smoke,skipped,transitions);
    VirtualFree(code,0,MEM_RELEASE);bh_entity_release(&pool,e);bh_draw_queue_dispose(&queue);bh_entities_dispose(&pool);bh_math_environment_leave(&environment);
    return bad||errors?1:0;
}
