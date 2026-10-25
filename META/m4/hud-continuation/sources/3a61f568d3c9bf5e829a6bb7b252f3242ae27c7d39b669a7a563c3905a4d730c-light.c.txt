#include "../src/bh_light.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<12)printf("FAIL light %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<=3e-6*(1+fabs(b));}
static int vector(const BHVec3 *v,double x,double y,double z){return close_value(v->x,x)&&close_value(v->y,y)&&close_value(v->z,z);}
static void context(BHLightContext *c,BHLight *light) {
    memset(c,0,sizeof(*c));c->lights=light;c->world_to_model.m[0][0]=c->world_to_model.m[1][1]=c->world_to_model.m[2][2]=1;
    c->local_reference.x=17;c->local_reference.y=18;c->local_reference.z=19;
}
static void light_init(BHLight *p,unsigned long type) {
    memset(p,0,sizeof(*p));p->type=type;p->color=0x123456;p->intensity=.75f;p->range=100;
    p->inner_cos=.8f;p->outer_cos=.4f;p->direction.z=1;
    p->local_position.x=31;p->local_position.y=32;p->local_position.z=33;
    p->local_direction.x=41;p->local_direction.y=42;p->local_direction.z=43;
    p->effective_intensity=51;p->prepared_type=61;p->opaque00=0xabc;p->opaque08=0xdef;
}
static void fixed(void) {
    BHLightContext c;BHLight p,saved;BHVec3 reference={0,0,16},override={3,4,0};
    CHECK(sizeof(BHLight)==0x5c && offsetof(BHLight,next)==4 && offsetof(BHLight,type)==0xc && offsetof(BHLight,effective_intensity)==0x24 && offsetof(BHLight,prepared_type)==0x58,"original32-bit light layout including all prepared fields");
    light_init(&p,3);context(&c,&p);
    CHECK(bh_light_prepare(&c,&reference,2) && p.prepared_type==3,"exact distance/8 boundary retains point light");
    CHECK(vector(&p.local_position,0,0,0) && vector(&p.local_direction,41,42,43) && p.effective_intensity==51,"near point updates position only; cached direction/effective strength retained");
    CHECK(vector(&c.local_reference,17,18,19),"without override the evaluator reference cache remains untouched");
    light_init(&p,3);CHECK(bh_light_prepare(&c,&reference,1.999f) && p.prepared_type==2,"just below radius threshold converts point to directional state");
    CHECK(vector(&p.local_direction,0,0,1) && close_value(p.effective_intensity,.75*.84) && vector(&p.local_position,31,32,33),"far point uses unit light-to-reference vector and linear range attenuation");
    light_init(&p,3);p.range=10;saved=p;
    CHECK(bh_light_prepare(&c,&reference,2) && p.prepared_type==0,"range cull precedes near-point retention");p.prepared_type=saved.prepared_type;
    CHECK(!memcmp(&p,&saved,sizeof(p)),"inactive point retains all other state");
    light_init(&p,3);p.range=16;CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==0,"exact range yields zero effective attenuation");
    light_init(&p,3);p.range=0;CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==2 && p.effective_intensity==.75f,"zero range means unbounded constant intensity");
    p.range=-5;CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==2 && p.effective_intensity==.75f,"negative range follows same unbounded branch");
    light_init(&p,4);p.direction.z=-1;
    CHECK(bh_light_prepare(&c,&reference,2) && p.prepared_type==4 && vector(&p.local_direction,0,0,-1) && p.effective_intensity==51,"near spot retains full spot state without cone attenuation");
    light_init(&p,4);CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==2 && close_value(p.effective_intensity,.75*.84),"far spot on axis becomes attenuated directional state");
    p.direction.z=-1;CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==0,"far spot outside cone disables");
    reference.x=3;reference.y=0;reference.z=4;light_init(&p,4);p.inner_cos=1;p.outer_cos=.5f;
    CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==2 && close_value(p.effective_intensity,.75*.95*.6),"far spot interpolates inner/outer cone then applies distance falloff");
    CHECK(vector(&p.local_direction,.6,0,.8),"spot approximation uses normalized ray rather than stored spot axis");
    light_init(&p,4);p.inner_cos=p.outer_cos=1;reference.x=reference.y=0;reference.z=16;
    CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==2,"equal cone thresholds on axis avoid divide by zero");
    light_init(&p,3);reference.z=0;CHECK(bh_light_prepare(&c,&reference,0) && p.prepared_type==3,"coincident point/radius0 retained without division by zero");
    c.world_to_model.m[0][0]=0;c.world_to_model.m[0][1]=-1;c.world_to_model.m[1][0]=1;c.world_to_model.m[1][1]=0;
    c.world_to_model.m[0][3]=10;c.world_to_model.m[1][3]=-20;c.world_to_model.m[2][3]=30;
    light_init(&p,3);p.position=override;
    CHECK(bh_light_prepare(&c,&override,0) && p.prepared_type==3 && vector(&p.local_position,6,-17,30),"point conversion uses complete transform including translation");
    light_init(&p,2);p.direction=override;
    CHECK(bh_light_prepare(&c,&reference,0) && vector(&p.local_direction,-4,3,0) && p.prepared_type==61 && p.effective_intensity==51,"directional preparation excludes translation and leaves prepared state/intensity unchanged");
    c.reference_override=&override;light_init(&p,3);p.position.x=3;p.position.y=4;
    CHECK(bh_light_prepare(&c,NULL,0) && p.prepared_type==3 && vector(&c.local_reference,6,-17,30),"override controls raw distance and publishes separate transformed evaluator reference");
    c.lights=NULL;c.local_reference.x=0;CHECK(bh_light_prepare(&c,NULL,0) && vector(&c.local_reference,6,-17,30),"override still transforms with an empty list");
    c.reference_override=NULL;CHECK(bh_light_prepare(&c,&reference,0),"empty valid list succeeds");
}
static void sweep(void) {
    BHLight p;BHLightContext c;BHVec3 r;unsigned long n;double distance,attenuation;float radius;
    context(&c,&p);
    /* Independent analytical 3-4-5 rays with varying distance, sphere size,
       range and cone. Rotation has exact coefficients; expected vector and
       attenuation do not call any production lighting helper. */
    c.world_to_model.m[0][0]=0;c.world_to_model.m[0][1]=-1;c.world_to_model.m[1][0]=1;c.world_to_model.m[1][1]=0;
    c.world_to_model.m[0][3]=70;c.world_to_model.m[1][3]=-50;c.world_to_model.m[2][3]=20;
    for(n=1;n<=256;++n) {
        light_init(&p,n%2?3:4);p.position.x=5;p.position.y=-7;p.position.z=3;
        r.x=5+(float)(3*n);r.y=-7;r.z=3+(float)(4*n);distance=5.0*n;
        radius=(n%3==0)?(float)(distance*.125):0;p.range=(n%4==0)?(float)(distance*.5):(n%4==1?0:(float)(distance*2));
        CHECK(bh_light_prepare(&c,&r,radius),"varied mixed light preparation");
        if(p.range>0 && distance>radius+p.range) {
            CHECK(p.prepared_type==0 && vector(&p.local_position,31,32,33) && vector(&p.local_direction,41,42,43) && p.effective_intensity==51,"analytical outside-range light only clears prepared tag");
        } else if(radius>=distance*.125) {
            CHECK(p.prepared_type==p.type && vector(&p.local_position,77,-45,23) && p.effective_intensity==51,"analytical intersecting light keeps full model-space position");
            CHECK(p.type==3?vector(&p.local_direction,41,42,43):vector(&p.local_direction,0,0,1),"only full spots transform their stored direction");
        } else {
            attenuation=p.range>0?1-distance/p.range:1;
            CHECK(p.prepared_type==2 && close_value(p.effective_intensity,.75*attenuation),"analytical distant approximation intensity");
            CHECK(vector(&p.local_direction,0,.6,.8) && vector(&p.local_position,31,32,33),"analytical rotated distant ray ignores translation");
        }
        CHECK(p.color==0x123456 && p.intensity==.75f && p.opaque00==0xabc && p.opaque08==0xdef && p.position.x==5 && p.direction.z==1,"preparation preserves descriptor and opaque ownership fields");
    }
}
static void lists_and_guards(void) {
    BHLight p[5],saved[5];BHLightContext c,prior;BHVec3 r={0,0,16};unsigned long i,bits=0x7fc00001UL;float invalid;
    for(i=0;i<5;++i){light_init(p+i,i+1);p[i].next=i<4?p+i+1:NULL;}
    context(&c,p);memcpy(saved,p,sizeof(p));
    CHECK(bh_light_prepare(&c,&r,0),"mixed borrowed list traverses all nodes");
    CHECK(!memcmp(p,saved,sizeof(*p)) && !memcmp(p+4,saved+4,sizeof(*p)),"ambient/unknown types are byte-for-byte unchanged");
    CHECK(vector(&p[1].local_direction,0,0,1) && p[2].prepared_type==2 && p[3].prepared_type==2,"directional point and spot members all prepare");
    p[4].next=p;memcpy(saved,p,sizeof(p));prior=c;
    CHECK(!bh_light_prepare(&c,&r,0) && !memcmp(saved,p,sizeof(p)) && !memcmp(&c,&prior,sizeof(c)),"cyclic list rejected before mutation");p[4].next=NULL;
    memcpy(&invalid,&bits,4);p[3].position.x=invalid;c.reference_override=&r;memcpy(saved,p,sizeof(p));prior=c;
    CHECK(!bh_light_prepare(&c,NULL,0) && !memcmp(saved,p,sizeof(p)) && !memcmp(&c,&prior,sizeof(c)),"invalid later descriptor preflight leaves earlier lights and override cache unchanged");
    CHECK(!bh_light_prepare(NULL,&r,0),"null context rejects safely");
    context(&c,NULL);CHECK(!bh_light_prepare(&c,NULL,0) && !bh_light_prepare(&c,&r,invalid),"missing reference/nonfinite radius rejected");
}
int bh_light_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    fixed();sweep();lists_and_guards();CHECK(bh_math_environment_supported(),"lighting preserves caller PC53 environment");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"light-prepare\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
