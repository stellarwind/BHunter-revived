#include "math_cases.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

typedef struct BHMathGolden {
    unsigned long id;
    int function,alias,status,count;
    unsigned long words[12];
} BHMathGolden;
static const BHMathGolden golden[]={
#include "math_golden.h"
};
static unsigned long checks;
static int failures;
static void check(int ok,const char *name) {
    ++checks;
    if (!ok) { if (failures<12) printf("FAIL math numerical: %s\n",name);++failures; }
}
static float from_word(unsigned long word) {float value;memcpy(&value,&word,4);return value;}
static unsigned long to_word(float value) {unsigned long word;memcpy(&word,&value,4);return word;}

static void environment_tests(void) {
    unsigned int saved=_control87(0,0),mask=_MCW_PC|_MCW_RC|_MCW_EM,current;
    unsigned int precision[]={_PC_24,_PC_64},rounding[]={_RC_DOWN,_RC_UP,_RC_CHOP};
    unsigned int exceptions[]={_EM_INVALID,_EM_DENORMAL,_EM_ZERODIVIDE,_EM_OVERFLOW,_EM_UNDERFLOW,_EM_INEXACT};
    BHMathEnvironment scope,nested;
    int i;
    check(bh_math_environment_supported(),"declared profile accepted");
    check(!bh_math_environment_enter(NULL),"null environment rejected");
    bh_math_environment_leave(NULL);
    check((_control87(0,0)&mask)==(saved&mask),"null leave does not change controls");
    for(i=0;i<2;i++) {
        _clearfp();_control87(precision[i],_MCW_PC);current=_control87(0,0);
        check(!bh_math_environment_supported(),"unsupported precision rejected");
        check(bh_math_environment_enter(&scope) && bh_math_environment_supported(),"scope establishes PC53");
        check(bh_math_environment_enter(&nested),"nested environment scope");
        bh_math_environment_leave(&nested);check(bh_math_environment_supported(),"nested restore retains supported parent");
        bh_math_environment_leave(&scope);
        check((_control87(0,0)&mask)==(current&mask),"scope restores previous precision");
        _control87(saved,mask);
    }
    for(i=0;i<3;i++) {
        _clearfp();_control87(rounding[i],_MCW_RC);current=_control87(0,0);
        check(!bh_math_environment_supported(),"unsupported rounding rejected");
        check(bh_math_environment_enter(&scope),"scope establishes nearest rounding");
        bh_math_environment_leave(&scope);
        check((_control87(0,0)&mask)==(current&mask),"scope restores previous rounding");
        _control87(saved,mask);
    }
    for(i=0;i<6;i++) {
        _clearfp();_control87(saved&~exceptions[i],_MCW_EM);current=_control87(0,0);
        check(!bh_math_environment_supported(),"unmasked exception rejected including denormal");
        check(bh_math_environment_enter(&scope),"scope masks every x87 exception");
        bh_math_environment_leave(&scope);
        check((_control87(0,0)&mask)==(current&mask),"scope restores exception masks");
        _clearfp();_control87(saved,mask);
    }
}

static void analytical_tests(void) {
    static const int exponents[]={-149,-140,-126,-80,-1,0,20,80,120,125};
    BHVec3 input,output,zero;
    BHMat34 rotation={{{0,-1,0,0},{1,0,0,0},{0,0,1,0}}},squared,inverse,identity;
    unsigned long zero_words[]={0x80000000UL,0,0x80000000UL};
    int i;
    float scale;
    for(i=0;i<sizeof(exponents)/sizeof(exponents[0]);i++) {
        scale=(float)ldexp(1.0,exponents[i]);input.x=3*scale;input.y=0;input.z=4*scale;
        check(bh_vec3_normalize_to(&input,&output)==1,"varied-magnitude normalize status");
        check(to_word(output.x)==to_word(0.6f) && to_word(output.y)==0 && to_word(output.z)==to_word(0.8f),"3-4-5 normalization from subnormal to huge");
        check(fabs((double)output.x*output.x+(double)output.z*output.z-1.0)<1e-6,"analytical unit length");
    }
    memcpy(&zero,zero_words,12);input=zero;output.x=5;output.y=6;output.z=7;
    check(bh_vec3_normalize(&input)==0 && !memcmp(&input,&zero,12),"signed-zero normalization preserves input bits");
    check(bh_vec3_normalize_to(&zero,&output)==0 && output.x==5 && output.y==6 && output.z==7,"zero normalize_to preserves destination");
    input.x=from_word(0x7fc12345UL);input.y=1;input.z=2;zero=input;
    check(bh_vec3_normalize(&input)==0 && !memcmp(&input,&zero,12),"original unordered normalization returns zero unchanged");
    input.x=from_word(0x7f800000UL);input.y=-1;input.z=0;
    check(bh_vec3_normalize(&input)==1 && bh_math_word_class(to_word(input.x))==3 && to_word(input.y)==0x80000000UL && to_word(input.z)==0,"infinite length follows original reciprocal behavior");
    check((_status87()&_SW_INVALID)!=0 && bh_math_environment_supported(),"masked invalid does not change numerical controls");
    _clearfp();
    bh_mat34_multiply(&rotation,&rotation,&squared);
    check(squared.m[0][0]==-1 && squared.m[1][1]==-1 && squared.m[2][2]==1,"same input matrix pointer with distinct output");
    bh_mat34_inverse_rigid(&rotation,&inverse);bh_mat34_multiply(&rotation,&inverse,&identity);
    check(identity.m[0][0]==1 && identity.m[1][1]==1 && identity.m[2][2]==1 && identity.m[0][1]==0 && identity.m[1][0]==0,"rigid orthonormal inverse composition");
    input.x=2;input.y=3;input.z=4;
    bh_mat34_transform_direction(&rotation,&input,&input);
    check(input.x==-3 && input.y==-3 && input.z==4,"in-place direction follows sequential input mutation");
}

int bh_math_numeric_checkpoint(unsigned long *total_checks) {
    BHMathOps source;
    BHMathCase input;
    BHMathResult actual;
    BHMathEnvironment environment;
    unsigned long i;
    int word;
    checks=0;failures=0;
    if (!bh_math_environment_enter(&environment)) {check(0,"cannot establish supported environment");*total_checks=checks;return 1;}
    bh_math_source_ops(&source);
    environment_tests();analytical_tests();
    for (i=0;i<sizeof(golden)/sizeof(golden[0]);i++) {
        bh_math_case(golden[i].id,&input);
        bh_math_eval(&source,golden[i].function,golden[i].alias,&input,&actual);
        check(actual.status==golden[i].status,"golden original return value");
        check(actual.guards_ok && actual.control_ok && actual.count==golden[i].count,"golden ABI guards and unchanged FP controls");
        for(word=0;word<actual.count;word++)
            check(actual.words[word]==golden[i].words[word] ||
                  (bh_math_word_class(actual.words[word])==3 && bh_math_word_class(golden[i].words[word])==3),
                  "golden exact finite/zero/infinity bits or explicit NaN class");
    }
    _clearfp();bh_math_environment_leave(&environment);
    *total_checks=checks;
    printf("{\"scenario\":\"math-numerical\",\"checks\":%lu,\"failures\":%d,\"golden_cases\":%lu,\"profile\":\"x87-PC53-nearest-masked\",\"original_code_required\":false}\n",checks,failures,(unsigned long)(sizeof(golden)/sizeof(golden[0])));
    return failures ? 1 : 0;
}
