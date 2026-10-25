#include "../src/bh_depth_subdivide.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL depth-subdivide: %s\n",s);}} while(0)
typedef struct Capture {unsigned long calls,count[128],fail_at;float z[128][16],u[128][16],v[128][16],x[128][16],y[128][16],a[128][16];int valid;} Capture;
static int __cdecl capture(BHClipVertex *ring,void *user) {
    Capture *c=(Capture *)user;BHClipVertex *p=ring;unsigned long i=c->calls++,n=0;
    if(i>=128)return 0;
    do {
        if(n==16 || p->next->previous!=p || p->previous->next!=p){c->valid=0;return 0;}
        c->z[i][n]=p->point.z;c->u[i][n]=p->u;c->v[i][n]=p->v;c->x[i][n]=p->sx;c->y[i][n]=p->sy;memcpy(&c->a[i][n],&p->unknown28,4);
        ++n;p=p->next;
    }while(p!=ring);c->count[i]=n;
    return !c->fail_at || c->calls!=c->fail_at;
}
static void fixture(BHClipVertex *v,float low,float high) {
    unsigned long i;float a;
    memset(v,0,3*sizeof(*v));
    for(i=0;i<3;++i){v[i].point.z=i==2?high:low;v[i].u=10.0f*i;v[i].v=2.0f*(i+1);v[i].sx=10.0f*(i+1);v[i].sy=i==0?10:i==1?30:20;
        v[i].next=v+(i+1)%3;v[i].previous=v+(i+2)%3;a=2.0f*(i+1);memcpy(&v[i].unknown28,&a,4);}
}
static void interpolation(void) {
    BHClipVertex v[3],before[3];BHDepthSplitContext c;Capture got;unsigned long attribute;
    for(attribute=0;attribute<2;++attribute) {
        memset(&got,0,sizeof(got));got.valid=1;fixture(v,15,17);memcpy(before,v,sizeof(v));
        CHECK(bh_depth_split_init(&c,0,640,0,480,capture,&got),"typed selected leaf binding");
        CHECK(attribute?bh_depth_subdivide_scalar(&c,v):bh_depth_subdivide(&c,v),"strict equality subdivides");
        CHECK(c.splits==1 && c.leaves==2 && c.deepest==1 && got.calls==2 && got.valid,"both reciprocal child rings consumed near first");
        CHECK(got.count[0]==4 && got.count[1]==3 && got.z[0][0]==16 && got.z[0][1]==16 && got.z[1][1]==17,
            "near quadrilateral then far triangle, binary32 depth midpoint");
        CHECK(got.u[0][0]==15 && got.v[0][0]==5 && got.u[0][1]==10 && got.v[0][1]==4,
            "UV interpolates linearly in depth, not screen coordinates");
        CHECK(got.x[0][0]==25.3125f && got.y[0][0]==24.6875f && got.x[0][1]==20.625f && got.y[0][1]==15.3125f,
            "independent rational perspective screen intersection");
        if(attribute)CHECK(got.a[0][0]==5 && got.a[0][1]==4 && got.a[1][0]==5 && got.a[1][2]==4,"scalar float interpolation preserved across both child lifetimes");
        CHECK(!memcmp(v,before,sizeof(v)),"all original fields and four spliced links restored");
        memset(&got,0,sizeof(got));got.valid=1;fixture(v,15.0001f,17);
        CHECK(attribute?bh_depth_subdivide_scalar(&c,v):bh_depth_subdivide(&c,v),"strictly smaller depth range completes");
        CHECK(!c.splits && c.leaves==1 && !c.deepest && got.count[0]==3,"leaf threshold positive control changes branch");
        memset(&got,0,sizeof(got));got.valid=1;fixture(v,15,17);c.left=26;c.right=27;c.top=18;c.bottom=21;
        CHECK(attribute?bh_depth_subdivide_scalar(&c,v):bh_depth_subdivide(&c,v),"live inclusive screen clamps");
        CHECK(got.x[0][0]==26 && got.x[0][1]==26 && got.y[0][0]==21 && got.y[0][1]==18,"only generated screen intersections clamped");
    }
}
static void recursion_and_guards(void) {
    BHClipVertex v[3],before[3];BHDepthSplitContext c;Capture got;unsigned long i,j;float low,high,prior=0;
    memset(&got,0,sizeof(got));got.valid=1;fixture(v,1,8);memcpy(before,v,sizeof(v));
    CHECK(bh_depth_split_init(&c,0,640,0,480,capture,&got) && bh_depth_subdivide_scalar(&c,v),"multiple recursive cuts");
    CHECK(c.splits>2 && c.leaves==c.splits+1 && c.deepest>1 && c.leaves==got.calls && got.valid,"full binary subdivision tree reaches real selected leaf");
    for(i=0;i<got.calls;++i){low=high=got.z[i][0];for(j=1;j<got.count[i];++j){if(got.z[i][j]<low)low=got.z[i][j];if(got.z[i][j]>high)high=got.z[i][j];}
        CHECK((high-low)*16.0<(double)low+high && low>=prior,"every leaf satisfies strict range and near-first order");prior=low;}
    CHECK(!memcmp(v,before,sizeof(v)),"nested original links restored");
    for(i=1;i<=2;++i){memset(&got,0,sizeof(got));got.valid=1;got.fail_at=i;fixture(v,15,17);memcpy(before,v,sizeof(v));
        CHECK(!bh_depth_subdivide_scalar(&c,v) && c.error==BH_DEPTH_SPLIT_LEAF && got.calls==i && !memcmp(v,before,sizeof(v)),"first/second leaf failure restores temporary stack links");}
    got.fail_at=0;fixture(v,1,1);
    CHECK(bh_depth_subdivide(&c,v) && c.leaves==1 && !c.splits,"equal positive depth is immediate leaf");
    v[1].previous=v+2;
    CHECK(!bh_depth_subdivide(&c,v) && c.error==BH_DEPTH_SPLIT_TOPOLOGY && !c.leaves,"malformed reciprocal topology rejected before callback");
    fixture(v,0,17);CHECK(!bh_depth_subdivide(&c,v) && c.error==BH_DEPTH_SPLIT_NUMERICAL,"zero depth cannot recurse or divide");
    fixture(v,15,17);{unsigned long nan=0x7fc00123UL;memcpy(&v[1].unknown28,&nan,4);}
    CHECK(!bh_depth_subdivide_scalar(&c,v) && c.error==BH_DEPTH_SPLIT_NUMERICAL,"scalar nonfinite bits rejected in scalar owner");
    CHECK(bh_depth_subdivide(&c,v),"plain owner does not inspect unknown scalar words");
    CHECK(!bh_depth_split_init(&c,2,1,0,480,capture,&got) && !bh_depth_split_init(&c,0,640,0,480,NULL,&got),"invalid live bounds or absent leaf cannot bind");
    CHECK(!bh_depth_subdivide(NULL,v) && !bh_depth_subdivide(&c,NULL),"missing source binding or ring rejected");
}
typedef struct Probe {float target;unsigned long bits,seen;} Probe;
static int __cdecl probe(BHClipVertex *ring,void *user) {
    Probe *p=(Probe *)user;BHClipVertex *v=ring;
    do{if(v->point.z==p->target){memcpy(&p->bits,&v->u,4);++p->seen;}v=v->next;}while(v!=ring);return 1;
}
static void retained_precision(void) {
    BHClipVertex v[3],before[3];BHDepthSplitContext c;Probe p;unsigned long variant;
    for(variant=0;variant<2;++variant){fixture(v,94.39231872558594f,652484800.0f);
        v[0].u=v[1].u=8.018010139465332f;v[2].u=-7.7358808517456055f;memcpy(before,v,sizeof(v));
        memset(&p,0,sizeof(p));p.target=326242432.0f;
        CHECK(bh_depth_split_init(&c,0,640,0,480,probe,&p) &&
            (variant?bh_depth_subdivide_scalar(&c,v):bh_depth_subdivide(&c,v)),"wide-depth retained/stored denominator fixture");
        CHECK(p.seen && p.bits==(variant?0x3e107358UL:0x3e107359UL),"independent binary32 expected UV distinguishes both original denominator contracts by one ULP");
        CHECK(!memcmp(v,before,sizeof(v)),"deep precision fixture restores complete source ring");
    }
}
int bh_depth_subdivide_tests(void) {
    BHMathEnvironment fp;checks=failures=0;
    if(!bh_math_environment_enter(&fp)){checks=failures=1;printf("FAIL depth-subdivide: cannot establish required FP profile\n");return 0;}
    interpolation();recursion_and_guards();retained_precision();CHECK(bh_math_environment_supported(),"PC53 nearest profile retained");bh_math_environment_leave(&fp);
    printf("depth-subdivide: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_depth_subdivide_checkpoint(unsigned long *count) {
    bh_depth_subdivide_tests();if(count)*count=checks;
    printf("{\"scenario\":\"depth-subdivide\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_DEPTH_SUBDIVIDE_STANDALONE
int main(void){unsigned long count;return bh_depth_subdivide_checkpoint(&count);}
#endif
