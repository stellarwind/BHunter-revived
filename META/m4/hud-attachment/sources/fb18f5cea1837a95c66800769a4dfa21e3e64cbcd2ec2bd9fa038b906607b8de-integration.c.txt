/* Source-only M0 linkage/finite-fixture checkpoint, not an M2 fidelity gate. */
#include <stdio.h>
#include <string.h>
#include <float.h>
#include "../src/bh_math.h"

static int checks, failures;
static void check(int ok, const char *name) {
    ++checks;
    if (!ok) { ++failures; printf("FAIL %s\n", name); }
}
static int vec(BHVec3 v, float x, float y, float z) {
    return v.x==x && v.y==y && v.z==z;
}
int main(void) {
    BHMat34 a={{{0,-1,0,10},{1,0,0,20},{0,0,1,30}}};
    BHMat34 identity={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    BHMat34 b,c;
    BHVec3 v={2,3,4}, w={5,6,7}, o, zero={0,0,0}, unit={3,0,4};
    unsigned int saved=_controlfp(0,0);
    _controlfp(_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC);
    check(sizeof(void*)==4 && sizeof(BHVec3)==12 && sizeof(BHMat34)==48,"x86 layouts");
    bh_mat34_multiply(&a,&identity,&b);
    check(memcmp(&a,&b,sizeof(a))==0,"multiply identity");
    bh_vec3_cross(&v,&w,&o); check(vec(o,-3,6,-3),"cross distinct");
    o=v; bh_vec3_cross(&o,&w,&o); check(vec(o,-3,41,-223),"cross sequential alias");
    check(bh_vec3_normalize(&unit)==1 && vec(unit,0.6f,0,0.8f),"normalize finite");
    check(bh_vec3_normalize(&zero)==0 && vec(zero,0,0,0),"normalize zero");
    o=v; check(bh_vec3_normalize_to(&zero,&o)==0 && vec(o,2,3,4),"normalize_to zero untouched");
    unit.x=0;unit.y=0;unit.z=5;
    check(bh_vec3_normalize_to(&unit,&o)==1 && vec(o,0,0,1),"normalize_to finite");
    bh_mat34_transform_point(&a,&v,&o); check(vec(o,7,22,34),"point");
    bh_mat34_transform_direction(&a,&v,&o); check(vec(o,-3,2,4),"direction");
    w.x=7;w.y=22;w.z=34;
    bh_mat34_inverse_transform_point(&a,&w,&o); check(vec(o,2,3,4),"inverse point");
    b=a;bh_mat34_translate_world(2,3,4,&b);
    check(b.m[0][3]==12 && b.m[1][3]==23 && b.m[2][3]==34,"world translation");
    b=a;bh_mat34_translate_local(2,3,4,&b);
    check(b.m[0][3]==7 && b.m[1][3]==22 && b.m[2][3]==34,"local translation");
    bh_mat34_inverse_rigid(&a,&b);bh_mat34_multiply(&a,&b,&c);
    check(memcmp(&c,&identity,sizeof(c))==0,"rigid inverse composition");
    _controlfp(saved,_MCW_PC|_MCW_RC);
    printf("{\"scenario\":\"m0-source-math\",\"checks\":%d,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures ? 1 : 0;
}
