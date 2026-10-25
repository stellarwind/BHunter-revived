#include "../src/bh_actor_basis.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line) {++checks;if(!ok){++failures;if(failures<16)printf("FAIL actor basis %d: %s\n",line,label);}}
static int close_value(double a,double b) {return fabs(a-b)<=3e-6*(1+fabs(b));}
typedef struct Fixture {
    BHActorBasisContext context;
    BHActorBasisSample samples[42];BHActorBasisGroup a[21],b[21];
    BHActorBasisIndices ia[21],ib[21];BHActorBasisScale sa[21],sb[21];
} Fixture;
static void prepare(Fixture *f) {
    memset(f,0x5a,sizeof(*f));memset(&f->context,0,sizeof(f->context));
    f->context.samples=f->samples;f->context.sample_capacity=42;
    f->context.groups_a=f->a;f->context.groups_b=f->b;f->context.a_capacity=f->context.b_capacity=21;
    f->context.indices_a=f->ia;f->context.indices_b=f->ib;f->context.scales_a=f->sa;f->context.scales_b=f->sb;
}
static const BHActorBasisSample expected_samples[14]={
    {{0,0,0},{0,0,0},{0,0,0}},
    {{1,0,0},{0,1.5f,0},{.7f,0,1.5f}},
    {{0,1,0},{-1.5f,0,.7f},{.7f,0,1.5f}},
    {{0,0,1},{0,-.7f,0},{.7f,0,1.5f}},
    {{-1,0,0},{0,-1.5f,0},{-.7f,0,1.5f}},
    {{0,1,0},{-1.5f,0,-.7f},{-.7f,0,1.5f}},
    {{0,0,1},{0,.7f,0},{-.7f,0,1.5f}},
    {{-1,0,0},{0,1.5f,0},{-.7f,0,-1.5f}},
    {{0,1,0},{1.5f,0,-.7f},{-.7f,0,-1.5f}},
    {{0,0,-1},{0,-.7f,0},{-.7f,0,-1.5f}},
    {{1,0,0},{0,-1.5f,0},{.7f,0,-1.5f}},
    {{0,1,0},{1.5f,0,.7f},{.7f,0,-1.5f}},
    {{0,0,-1},{0,.7f,0},{.7f,0,-1.5f}},
    {{0,-1,0},{0,0,0},{0,-1,0}}
};
static int same_vector(const BHVec3 *a,const BHVec3 *b) {return a->x==b->x && a->y==b->y && a->z==b->z;}
static void sample_values(void) {
    Fixture f;BHVec3 preserved;unsigned long i;BHActorBasisSample untouched;
    prepare(&f);f.samples[0].direction.x=3;f.samples[0].direction.y=4;f.samples[0].direction.z=5;preserved=f.samples[0].direction;
    untouched=f.samples[14];
    CHECK(sizeof(BHActorBasisSample)==36 && sizeof(BHActorBasisGroup)==48 && sizeof(BHActorBasisIndices)==64 && sizeof(BHActorBasisScale)==8,"original sample group index and scale strides");
    CHECK(bh_actor_basis_acquire(&f.context),"complete one-time basis construction and actual group builder");
    CHECK(f.context.refcount==1 && f.context.sample_count==14 && f.context.a_count==7 && f.context.b_count==7,"first owner call appends exactly14 plus7 plus7 records");
    CHECK(!memcmp(&preserved,&f.samples[0].direction,12),"original zero normalize_to preserves all destination bytes instead of initializing sample0 direction");
    for(i=0;i<14;++i) {
        if(i)CHECK(same_vector(&f.samples[i].direction,&expected_samples[i].direction),"analytic cardinal direction fixtures");
        CHECK(same_vector(&f.samples[i].torque,&expected_samples[i].torque),"independent corner lever-arm torque fixtures");
        CHECK(same_vector(&f.samples[i].point,&expected_samples[i].point),"all original contact point fixtures");
    }
    CHECK(!memcmp(&untouched,&f.samples[14],sizeof(untouched)),"sample after14 outputs remains untouched");
}
static float stored(double x) {float value;bh_round_float(&value,x);return value;}
static void verify_groups(const Fixture *f,const long *definition,
    const BHActorBasisGroup *groups,const BHActorBasisIndices *indices,const BHActorBasisScale *scales) {
    unsigned long cursor=0,group=0,i,j;long count,index;
    float sums[6];double magnitude,scale,expected;
    while(definition[cursor]>0) {
        memset(sums,0,sizeof(sums));count=definition[cursor++];
        CHECK(indices[group].count==count+1,"original index-list count includes leading count word");
        for(i=0;i<(unsigned long)count;++i) {
            index=definition[cursor++];CHECK(indices[group].sample[i]==index,"absolute sample references retain original ordering");
            for(j=0;j<3;++j) {
                sums[j]=stored((double)sums[j]+((const float *)&f->samples[index].direction)[j]);
                sums[3+j]=stored((double)sums[3+j]+((const float *)&f->samples[index].torque)[j]);
            }
        }
        for(i=(unsigned long)count;i<15;++i)CHECK(indices[group].sample[i]==0x5a5a5a5a,"unused index-list tail preserves prior bytes");
        for(i=0;i<6;++i)CHECK(((const float *)&groups[group].direction)[i]==sums[i],"per-component group sum includes expected referenced samples");
        for(i=0;i<2;++i) {
            magnitude=sqrt(((double)sums[i*3]*sums[i*3]+(double)sums[i*3+1]*sums[i*3+1])+(double)sums[i*3+2]*sums[i*3+2]);
            if(magnitude==0) {
                CHECK(!_finite(((const float *)&scales[group])[i]) && !_isnan(((const float *)&scales[group])[i]),"zero group retains infinite reciprocal contract");
                for(j=0;j<3;++j)CHECK(_isnan(((const float *)&groups[group].unit_direction)[i*3+j]),"zero sum times infinity retains NaN result class");
            } else {
                scale=1/magnitude;CHECK(close_value(((const float *)&scales[group])[i],scale),"group reciprocal magnitude fixture");
                for(j=0;j<3;++j){expected=sums[i*3+j]*scale;CHECK(close_value(((const float *)&groups[group].unit_direction)[i*3+j],expected),"normalized group uses retained reciprocal precision");}
            }
        }
        ++group;
    }
    CHECK(group==7,"each pinned static family ends after seven groups");
}
static void groups_and_zero(void) {
    Fixture f;BHActorBasisGroup untouched;unsigned long i;
    static const long expected_a[25]={1,0,4,1,6,7,12,4,3,4,9,10,2,2,5,2,8,11,2,2,11,2,5,8,-1};
    static const long expected_b[22]={1,0,4,2,5,8,11,1,13,2,1,10,2,4,7,2,6,3,2,9,12,-1};
    prepare(&f);f.samples[0].direction.x=f.samples[0].direction.y=f.samples[0].direction.z=0;untouched=f.a[7];
    CHECK(!memcmp(expected_a,bh_actor_basis_definition_a,sizeof(expected_a)) && !memcmp(expected_b,bh_actor_basis_definition_b,sizeof(expected_b)),"exact original static group definitions and terminators");
    CHECK(bh_actor_basis_acquire(&f.context),"original BSS-zero initial direction fixture");
    verify_groups(&f,expected_a,f.a,f.ia,f.sa);verify_groups(&f,expected_b,f.b,f.ib,f.sb);
    CHECK(!memcmp(&untouched,&f.a[7],sizeof(untouched)),"group afterseven outputs remains untouched");
    for(i=0;i<3;++i)CHECK(_isnan(((float *)&f.a[0].unit_direction)[i]) && _isnan(((float *)&f.a[0].unit_torque)[i]),"first zero placeholder group remains intentionally nonfinite");
}
static void retained_reciprocal(void) {
    Fixture f;double inverse;float expected,rounded_control;
    prepare(&f);memcpy(f.samples,expected_samples,sizeof(expected_samples));f.context.sample_count=14;
    f.samples[0].direction.x=.3f;f.samples[0].direction.y=.7f;f.samples[0].direction.z=1.5f;
    f.samples[0].torque.x=.125f;f.samples[0].torque.y=.25f;f.samples[0].torque.z=.875f;
    CHECK(bh_actor_basis_build_groups(&f.context),"standalone real group owner over preexisting sample definitions");
    inverse=1/sqrt(((double)f.samples[0].direction.x*f.samples[0].direction.x+(double)f.samples[0].direction.y*f.samples[0].direction.y)+(double)f.samples[0].direction.z*f.samples[0].direction.z);
    expected=stored((double)f.samples[0].direction.y*inverse);
    rounded_control=stored((double)f.samples[0].direction.y*stored(inverse));
    CHECK(expected!=rounded_control,"positive control distinguishes retained reciprocal from binary32 reciprocal");
    CHECK(f.a[0].unit_direction.y==expected && f.a[0].unit_direction.y!=rounded_control,"normalized vector retains reciprocal while separately publishing rounded scale");
    CHECK(f.sa[0].direction==stored(inverse),"scale output independently stores binary32 reciprocal");
    verify_groups(&f,bh_actor_basis_definition_a,f.a,f.ia,f.sa);verify_groups(&f,bh_actor_basis_definition_b,f.b,f.ib,f.sb);
}
static void guard_and_append(void) {
    Fixture f,before;unsigned long i;
    prepare(&f);f.samples[0].direction.x=1;f.samples[0].direction.y=f.samples[0].direction.z=0;
    CHECK(bh_actor_basis_acquire(&f.context),"prepare full basis for refcount guard");before=f;
    CHECK(bh_actor_basis_acquire(&f.context) && f.context.refcount==2 && !memcmp(f.samples,before.samples,sizeof(f.samples)) && !memcmp(f.a,before.a,sizeof(f.a)) && f.context.sample_count==14,"second acquire only increments reference count");
    f.context.refcount=0xffffffff;before=f;
    CHECK(bh_actor_basis_acquire(&f.context) && f.context.refcount==0 && !memcmp(f.samples,before.samples,sizeof(f.samples)) && f.context.a_count==7,"original32-bit reference count wraps without triggering init on old-nonzero value");
    CHECK(bh_actor_basis_acquire(&f.context) && f.context.refcount==1 && f.context.sample_count==28 && f.context.a_count==14 && f.context.b_count==14,"next old-zero call appends another whole family without clearing existing counts");
    CHECK(f.ia[7].sample[0]==0 && f.ib[7].sample[0]==0,"appended groups still reference absolute original sample indices");
    CHECK(!memcmp(f.samples,before.samples,14*sizeof(f.samples[0])),"append preserves preceding sample entries");
    prepare(&f);f.context.sample_capacity=13;before=f;
    CHECK(!bh_actor_basis_acquire(&f.context) && !memcmp(&f,&before,sizeof(f)),"first-call sample capacity guard is atomic");
    prepare(&f);f.context.b_capacity=6;before=f;
    CHECK(!bh_actor_basis_acquire(&f.context) && !memcmp(&f,&before,sizeof(f)),"group capacity guard rejects before reference increment or sample writes");
    prepare(&f);f.context.sample_count=13;before=f;
    CHECK(!bh_actor_basis_build_groups(&f.context) && !memcmp(&f,&before,sizeof(f)),"standalone builder rejects missing absolute sample13 before writes");
    f.context.refcount=7;f.context.samples=0;
    CHECK(bh_actor_basis_acquire(&f.context) && f.context.refcount==8,"old-nonzero reference path does not inspect unused array bindings");
    CHECK(!bh_actor_basis_acquire(0) && !bh_actor_basis_build_groups(0),"null explicit owner guards");
    (void)i;
}
int bh_actor_basis_checkpoint(unsigned long *count) {
    BHMathEnvironment environment;checks=0;failures=0;if(!bh_math_environment_enter(&environment))return 1;
    sample_values();groups_and_zero();retained_reciprocal();guard_and_append();bh_math_environment_leave(&environment);if(count)*count=checks;
    printf("{\"scenario\":\"actor-basis\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
