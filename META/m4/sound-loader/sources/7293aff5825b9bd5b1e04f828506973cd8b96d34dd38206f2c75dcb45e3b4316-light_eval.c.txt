#include "../src/bh_light_eval.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL light-evaluation %d: %s\n",line,label);}}
static void init(BHLightContext *c,BHLight *p,unsigned long type) {
    memset(c,0,sizeof(*c));memset(p,0,sizeof(*p));c->lights=p;
    c->world_to_model.m[0][0]=c->world_to_model.m[1][1]=c->world_to_model.m[2][2]=1;
    p->type=p->prepared_type=type;p->intensity=p->effective_intensity=1;p->color=0x804020;
    p->direction.z=p->local_direction.z=1;p->inner_cos=1;p->outer_cos=.5f;
}
static void fixed(void) {
    BHLightContext c,saved_c;BHLight p[3],saved[3];BHVec3 point={0,0,4},normal={0,0,-1},override={0,0,8};unsigned long rgb=0xdeadbeef;
    CHECK(sizeof(BHLight)==0x5c && offsetof(BHLight,prepared_type)==0x58,"original descriptor ABI");
    init(&c,p,1);p->intensity=99;p->effective_intensity=.5f;
    CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0x402010,"ambient uses cached strength and needs no point/normal");
    CHECK(bh_light_evaluate_prepared(&c,NULL,NULL,2,&rgb)&&rgb==0x804020,"scale applied after channel accumulation");
    p->effective_intensity=-1;CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0,"negative ambient channels clamp low");
    p->effective_intensity=4;CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0xffff80,"channels saturate independently");
    p->effective_intensity=1;CHECK(bh_light_evaluate_world(&c,NULL,NULL,.99f,&rgb)&&rgb==0x7e3f1f,"conversion truncates toward zero rather than rounds");
    init(&c,p,2);CHECK(bh_light_evaluate_world(&c,NULL,&normal,1,&rgb)&&rgb==0x804020,"directional front face");
    normal.z=1;CHECK(bh_light_evaluate_world(&c,NULL,&normal,1,&rgb)&&rgb==0,"back face contributes nothing");
    normal.z=-.5f;CHECK(bh_light_evaluate_world(&c,NULL,&normal,1,&rgb)&&rgb==0x402010,"normal deliberately not normalized");normal.z=-1;
    p->local_direction.z=-1;CHECK(bh_light_evaluate_prepared(&c,NULL,&normal,1,&rgb)&&rgb==0,"prepared evaluator uses local direction");
    init(&c,p,3);p->range=8;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x402010,"point light linear distance falloff");
    p->range=4;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0,"exact range boundary");
    p->range=2;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0,"beyond range clamps attenuation");
    p->range=0;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x804020,"zero range is unbounded");
    p->range=-8;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x804020,"negative range is unbounded");
    point.z=0;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0,"coincident point has no positive diffuse result");point.z=4;
    init(&c,p,4);CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x804020,"spot inner boundary");
    p->direction.z=.75f;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x402010,"spot analytical half cone ramp");
    p->direction.z=.5f;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0,"spot exact outer boundary");
    p->direction.z=.25f;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0,"outside spot cone");
    p->direction.z=p->inner_cos=p->outer_cos=1;CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x804020,"equal cone boundaries avoid division");
    init(&c,p,3);p->range=16;c.reference_override=&override;c.local_reference.z=4;
    CHECK(bh_light_evaluate_world(&c,NULL,&normal,1,&rgb)&&rgb==0x402010,"world override uses raw override position");
    CHECK(bh_light_evaluate_prepared(&c,NULL,&normal,1,&rgb)&&rgb==0x603018,"prepared override uses separately transformed cached position");
    p->prepared_type=0;CHECK(bh_light_evaluate_prepared(&c,NULL,NULL,1,&rgb)&&rgb==0,"inactive prepared tag ignores world type and inputs");
    init(&c,p,1);p->color=0x010101;p->effective_intensity=16777216;
    p[1]=p[2]=*p;p->next=p+1;p[1].next=p+2;p[1].effective_intensity=1;p[2].effective_intensity=-16777216;
    CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0x010100,"blue rounds each contribution; red and green retain cancellation");
    p->effective_intensity=2;p[1].effective_intensity=-1.0f/33554432;p[1].next=NULL;
    CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0x010202,"green rounds once before scale; red remains retained");
    memcpy(saved,p,sizeof(p));saved_c=c;CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&!memcmp(saved,p,sizeof(p))&&!memcmp(&saved_c,&c,sizeof(c)),"evaluation is read only");
    p->effective_intensity=1;p->next=NULL;p->color=0x804020;
    CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&p->color)&&p->color==0x804020,"output may alias a consumed descriptor field");
    c.lights=NULL;CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0,"empty list needs no geometry");
}
static void sweep(void) {
    BHLightContext c;BHLight p;BHVec3 point={0,0,8},normal={0,0,-1};unsigned long n,type,rgb,expected;float strength;
    /* Axis rays give exact distance/diffuse; dyadic cone/range/intensity
       controls independently yield exact half/quarter channel fractions. */
    for(n=0;n<256;++n)for(type=1;type<=4;++type) {
        init(&c,&p,type);p.color=((n&255)<<16)|(((255-n)&255)<<8)|(n/2);
        p.intensity=p.effective_intensity=(n%4+1)*.25f;
        strength=p.intensity;
        if(type>=3){p.range=n%2?16:0;if(n%2)strength*=.5f;}
        if(type==4){p.direction.z=p.local_direction.z=.75f;strength*=.5f;}
        expected=((unsigned long)(((n&255)*strength))<<16)|((unsigned long)((255-n)*strength)<<8)|(unsigned long)((n/2)*strength);
        CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==expected,"analytical world four-mode sweep");
        CHECK(bh_light_evaluate_prepared(&c,&point,&normal,1,&rgb)&&rgb==expected,"analytical prepared four-mode sweep");
    }
}
static void prepare_chain(void) {
    BHLightContext c;BHLight p;BHVec3 point={0,0,8},normal={0,0,-1};unsigned long a,b;
    init(&c,&p,3);p.range=16;
    CHECK(bh_light_prepare(&c,&point,0)&&p.prepared_type==2,"real preparation creates distant directional approximation");
    CHECK(bh_light_evaluate_prepared(&c,&point,&normal,1,&a)&&bh_light_evaluate_world(&c,&point,&normal,1,&b)&&a==b&&a==0x402010,"prepared and world lighting agree at preparation reference");
    CHECK(bh_light_prepare(&c,&point,1)&&p.prepared_type==3,"real preparation retains exact near point at radius boundary");
    CHECK(bh_light_evaluate_prepared(&c,&point,&normal,1,&a)&&a==0x402010,"near point evaluator consumes real preparation fields");
    init(&c,&p,4);p.range=16;p.direction.z=.75f;
    CHECK(bh_light_prepare(&c,&point,0)&&p.prepared_type==2,"real spot preparation applies cosine ramp");
    CHECK(bh_light_evaluate_prepared(&c,&point,&normal,1,&a)&&a==0x201008,"prepared spot approximation consumed without second cone attenuation");
}
static void off_axis(void) {
    BHLightContext c;BHLight p;BHVec3 point={3,4,12},normal={0,0,-1};unsigned long rgb;
    init(&c,&p,3);p.color=0x82411a;
    CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x783c18,"independent 3-4-12 ray has length13 and diffuse12/13");
    normal.x=-1;normal.z=0;
    CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x1e0f06,"off axis X diffuse3/13");
    normal.x=0;normal.y=-1;
    CHECK(bh_light_evaluate_prepared(&c,&point,&normal,1,&rgb)&&rgb==0x281408,"off axis Y diffuse4/13");
    init(&c,&p,4);p.outer_cos=0;p.color=0xffffff;normal.y=0;normal.z=-1;
    CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0xd9d9d9,"off axis spot uses independent squared12/13 diffuse and cone");
    init(&c,&p,3);p.range=16;point.x=point.y=0;point.z=4;p.local_position.z=-4;
    CHECK(bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0x603018,"world evaluator uses world position");
    CHECK(bh_light_evaluate_prepared(&c,&point,&normal,1,&rgb)&&rgb==0x402010,"prepared evaluator uses local position");
    point=normal;point.z=-1;init(&c,&p,2);p.direction.z=p.local_direction.z=1;
    CHECK(bh_light_evaluate_world(&c,&point,&point,1,&rgb)&&rgb==0x804020,"point and normal pointers may alias");
}
static void guards(void) {
    BHLightContext c;BHLight p;BHVec3 point={0,0,8},normal={0,0,-1};unsigned long rgb=0xabcdef,bits=0x7fc00000;float invalid;
    memcpy(&invalid,&bits,4);init(&c,&p,3);
    CHECK(!bh_light_evaluate_world(NULL,&point,&normal,1,&rgb)&&rgb==0xabcdef,"null context leaves output unchanged");
    CHECK(!bh_light_evaluate_world(&c,NULL,&normal,1,&rgb)&&rgb==0xabcdef,"required point missing");
    CHECK(!bh_light_evaluate_world(&c,&point,NULL,1,&rgb)&&rgb==0xabcdef,"required normal missing");
    CHECK(!bh_light_evaluate_world(&c,&point,&normal,1,NULL),"null output rejected");
    p.next=&p;CHECK(!bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0xabcdef,"cyclic list rejected");p.next=NULL;
    p.range=invalid;CHECK(!bh_light_evaluate_world(&c,&point,&normal,1,&rgb)&&rgb==0xabcdef,"consumed nonfinite field rejected");
    p.type=99;CHECK(bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0,"unknown type ignores unconsumed fields");
    rgb=0xabcdef;init(&c,&p,1);p.effective_intensity=1e20f;
    CHECK(!bh_light_evaluate_world(&c,NULL,NULL,1,&rgb)&&rgb==0xabcdef,"extreme conversion outside admitted domain rejects explicitly");
    CHECK(!bh_light_evaluate_world(&c,NULL,NULL,invalid,&rgb)&&rgb==0xabcdef,"nonfinite scale rejected");
}
int bh_light_eval_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    fixed();sweep();prepare_chain();off_axis();guards();CHECK(bh_math_environment_supported(),"preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"light-evaluation\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_LIGHT_EVAL_STANDALONE
int main(void){unsigned long count;return bh_light_eval_checkpoint(&count);}
#endif
