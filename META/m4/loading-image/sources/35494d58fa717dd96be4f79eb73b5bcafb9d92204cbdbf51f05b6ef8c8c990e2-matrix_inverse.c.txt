#include "../src/bh_matrix_inverse.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "matrix_inverse_cases.h"
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<15)printf("FAIL matrix inverse %d: %s\n",line,label);}}
static unsigned long bits(float f){unsigned long r;memcpy(&r,&f,4);return r;}
static int close_value(double a,double b,double tolerance){return _finite(a) && fabs(a-b)<=tolerance*(1+fabs(b));}
/* Independent Gauss-Jordan double oracle: no source LU/pivot/solve calls. */
static int oracle(const BHMat34 *in,double result[3][4]) {
    double a[4][8],best,temporary,divisor;int i,j,k,pivot;
    memset(a,0,sizeof(a));
    for(i=0;i<4;++i){for(j=0;j<4;++j)a[i][j]=i<3?in->m[i][j]:(double)(j==3);a[i][i+4]=1;}
    for(j=0;j<4;++j) {
        pivot=j;best=fabs(a[j][j]);for(i=j+1;i<4;++i)if(best<fabs(a[i][j])){best=fabs(a[i][j]);pivot=i;}
        if(!best)return 0;
        for(k=0;k<8;++k){temporary=a[j][k];a[j][k]=a[pivot][k];a[pivot][k]=temporary;}
        divisor=a[j][j];for(k=0;k<8;++k)a[j][k]/=divisor;
        for(i=0;i<4;++i)if(i!=j){divisor=a[i][j];for(k=0;k<8;++k)a[i][k]-=divisor*a[j][k];}
    }
    for(i=0;i<3;++i)for(j=0;j<4;++j)result[i][j]=a[i][j+4];return 1;
}
static void exact_cases(void) {
    float a[4][4]={{1,1,0,0},{2,-2,0,0},{0,0,1,0},{0,0,0,1}};
    const float factors[4][4]={{2,-2,0,0},{.5f,2,0,0},{0,0,1,0},{0,0,0,1}};
    float rhs[4]={3,-2,5,7},parity;long pivot[4];unsigned long i,j;
    const float stored[4][4]={{3.25f,-7.125f,2.375f,8.75f},{9.5f,1.125f,-3.75f,.375f},{-1.875f,4.25f,6.625f,-2.5f},{.625f,8.875f,-5.25f,3.125f}};
    CHECK(bh_mat4_lu(a,pivot,&parity) && pivot[0]==1 && pivot[1]==1 && pivot[2]==2 && pivot[3]==3 && parity==-1,"equal scaled score selects LAST row; exact pivot and parity contract");
    CHECK(!memcmp(a,factors,sizeof(a)),"analytical pivoted LU factors including binary32 reciprocal multiplier");
    bh_mat4_lu_solve(a,pivot,rhs);CHECK(rhs[0]==1 && rhs[1]==2 && rhs[2]==5 && rhs[3]==7,"pivoted forward/back solve against hand-computed vector");
    memset(a,0,sizeof(a));for(i=0;i<4;++i)a[i][i]=1;
    rhs[0]=rhs[1]=rhs[2]=0;rhs[3]=4;
    CHECK(bh_mat4_lu(a,pivot,&parity),"identity factorization");bh_mat4_lu_solve(a,pivot,rhs);
    CHECK(rhs[0]==0 && rhs[1]==0 && rhs[2]==0 && rhs[3]==4,"first-nonzero shortcut preserves leading-zero right-hand side");
    memcpy(a,stored,sizeof(a));CHECK(bh_mat4_lu(a,pivot,&parity),"nonsymmetric fixed numerical storage vector");
    for(i=0;i<4;++i){CHECK(pivot[i]==factor_pivots[i],"fixed independently modeled pivot sequence");for(j=0;j<4;++j)CHECK(bits(a[i][j])==factor_bits[i*4+j],"exact instruction-derived binary32 factors retain PC53 row sums");}
    memset(a,0,sizeof(a));for(i=0;i<4;++i)a[i][i]=1;
    a[0][0]=100;a[0][1]=10000;a[1][0]=1;a[1][1]=1;
    CHECK(bh_mat4_lu(a,pivot,&parity) && pivot[0]==1,"scaled pivot chooses unit row over larger absolute hundred pivot");
}
static void affine_cases(void) {
    BHMat34 a={{{2,1,0,3},{0,4,2,-2},{0,0,.5f,5}}},out,alias,product,rigid;
    const BHMat34 expected={{{.5f,-.125f,.5f,-4.25f},{0,.25f,-1,5.5f},{0,0,2,-10}}};
    BHVec3 point={1,2,3},transformed,restored;unsigned long i,j;
    CHECK(bh_mat34_inverse_general(&a,&out) && !memcmp(&out,&expected,sizeof(out)),"analytical scale/shear/translation inverse with exact dyadic entries");
    bh_mat34_inverse_rigid(&a,&rigid);CHECK(memcmp(&rigid,&out,sizeof(out))!=0,"positive control proves existing rigid inverse cannot satisfy this clipper dependency");
    alias=a;CHECK(bh_mat34_inverse_general(&alias,&alias) && !memcmp(&alias,&expected,sizeof(alias)),"general inverse supports actual input/output alias");
    bh_mat34_multiply(&a,&out,&product);for(i=0;i<3;++i)for(j=0;j<4;++j)CHECK(product.m[i][j]==(float)(i==j),"exact affine inverse composition");
    bh_mat34_transform_point(&a,&point,&transformed);bh_mat34_transform_point(&out,&transformed,&restored);
    CHECK(!memcmp(&point,&restored,sizeof(point)),"source transform/inverse round trip uses real math definitions");
    /* Projected textures in00452140 invert the camera basis once, transform
       camera positions, then use world X/Z affine coefficients for U and V. */
    point.x=7;point.y=-3;point.z=2;bh_mat34_transform_point(&a,&point,&transformed);bh_mat34_transform_point(&out,&transformed,&restored);
    CHECK(restored.x*2+restored.z*.5f+.25f==15.25f && restored.x*-.25f+restored.z*4-.5f==5.75f,"clipper projected-texture dependency restores correct world X/Z under nonrigid camera basis");
}
static void failure_contracts(void) {
    BHMat34 a={{{1,0,0,0},{0,0,0,0},{0,0,1,0}}},out,saved;
    float lu[4][4]={{1,0,0,0},{0,0,0,0},{0,0,1,0},{0,0,0,1}},original[4][4],parity=0,rhs[4]={1,2,3,4};long pivot[4]={-7,-7,-7,-7};int nonfinite=0,i,j;
    memset(&out,0x39,sizeof(out));saved=out;
    CHECK(!bh_mat34_inverse_general(&a,&out) && !memcmp(&out,&saved,sizeof(out)),"zero-row rejection leaves affine destination unchanged");
    memcpy(original,lu,sizeof(lu));CHECK(!bh_mat4_lu(lu,pivot,&parity) && parity==1 && !memcmp(original,lu,sizeof(lu)) && pivot[0]==-7 && pivot[3]==-7,
        "zero-row scale pass initializes parity but mutates neither matrix nor pivots");
    bh_mat4_lu_solve(lu,pivot,rhs);CHECK(rhs[0]==1 && rhs[1]==2 && rhs[2]==3 && rhs[3]==4,"source invalid-pivot guard rejects before right-hand-side mutation");
    a.m[1][0]=2;a.m[1][1]=0;a.m[1][2]=0;a.m[1][3]=0;
    CHECK(bh_mat34_inverse_general(&a,&out),"dependent nonzero rows retain original success return; no invented tiny pivot or rank certificate");
    for(i=0;i<3;++i)for(j=0;j<4;++j)if(!_finite(out.m[i][j]))++nonfinite;
    CHECK(nonfinite>0,"rank-deficient accepted decomposition yields original nonfinite solve behavior");
    memset(&a,0,sizeof(a));a.m[0][0]=1e-20f;a.m[1][1]=a.m[2][2]=1;
    CHECK(bh_mat34_inverse_general(&a,&out) && close_value(out.m[0][0],1.0/a.m[0][0],1e-6),"small nonzero pivot accepted without arbitrary epsilon rejection");
    {unsigned long nan=0x7fc12345UL;memcpy(&a.m[0][0],&nan,4);out=saved;
     CHECK(!bh_mat34_inverse_general(&a,&out) && !memcmp(&out,&saved,sizeof(out)),"source guard rejects nonfinite LU input with no published inverse");}
}
static void varied_cases(void) {
    BHMat34 a,out,alias,product;double reference[3][4];unsigned long seed=0x713951UL,n,i,j;
    for(n=0;n<128;++n) {
        for(i=0;i<3;++i)for(j=0;j<4;++j){seed=seed*1664525UL+1013904223UL;a.m[i][j]=(float)((long)((seed>>16)%33)-16)*.25f;if(i==j)a.m[i][j]+=12;}
        CHECK(oracle(&a,reference) && bh_mat34_inverse_general(&a,&out),"varied nonsymmetric affine matrices invert through real dependency family");
        for(i=0;i<3;++i)for(j=0;j<4;++j)CHECK(close_value(out.m[i][j],reference[i][j],2e-6),"independent double Gauss-Jordan oracle bounds LU inverse error");
        alias=a;CHECK(bh_mat34_inverse_general(&alias,&alias) && !memcmp(&alias,&out,sizeof(out)),"varied in-place inverse equals disjoint result bit-for-bit");
        bh_mat34_multiply(&a,&out,&product);for(i=0;i<3;++i)for(j=0;j<4;++j)CHECK(close_value(product.m[i][j],(double)(i==j),2e-5),"varied inverse composes with real existing affine math");
    }
}
int bh_matrix_inverse_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    exact_cases();affine_cases();failure_contracts();varied_cases();CHECK(bh_math_environment_supported(),"general affine family retains caller PC53 control contract");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"matrix-inverse\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
