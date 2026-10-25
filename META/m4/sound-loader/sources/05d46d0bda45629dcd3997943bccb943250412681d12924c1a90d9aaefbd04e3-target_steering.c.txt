#include "../src/bh_target_steering.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL target-steering: %s\n",s);}} while(0)
static float from_bits(unsigned long b) {float f;memcpy(&f,&b,4);return f;}
static unsigned long bits(float f) {unsigned long b;memcpy(&b,&f,4);return b;}
static void set_bound(BHWorldGeometry *actor,unsigned long index,unsigned long b) {memcpy((unsigned char *)&actor->minimum+index*4,&b,4);}
static void bounds(BHWorldGeometry *actor) {
    BHVec3 low={-1,-2,-3},high={1,2,3};
    memcpy((unsigned char *)&actor->minimum,&low,12);memcpy((unsigned char *)&actor->maximum,&high,12);
}
static void finite(BHWorldGeometry *a) {
    int x,y,z,reverse;BHVec3 p,expected;BHWorldGeometry before;
    for(reverse=0;reverse<2;++reverse) {
        bounds(a);if(reverse) {BHVec3 low={4,4,4};memcpy((unsigned char *)&a->minimum,&low,12);}
        before=*a;
        for(x=-5;x<=5;++x)for(y=-5;y<=5;++y)for(z=-5;z<=5;++z) {
            p.x=(float)x;p.y=(float)y;p.z=(float)z;expected=p;
            if(expected.x<(reverse?4:-1))expected.x=(float)(reverse?4:-1);else if(expected.x>1)expected.x=1;
            if(expected.y<(reverse?4:-2))expected.y=(float)(reverse?4:-2);else if(expected.y>2)expected.y=2;
            if(expected.z<(reverse?4:-3))expected.z=(float)(reverse?4:-3);else if(expected.z>3)expected.z=3;
            bh_world_clamp_point(a,&p);
            CHECK(!memcmp(&p,&expected,12),"exhaustive ordered clamp and reversed-bounds branch order");
        }
        CHECK(!memcmp(a,&before,sizeof(*a)),"disjoint output changes no actor byte");
    }
}
static void exceptional(BHWorldGeometry *a) {
    BHVec3 p;unsigned long axis,which;float *v=&p.x;
    bounds(a);p.x=p.y=p.z=from_bits(0x7fc00001);
    bh_world_clamp_point(a,&p);
    CHECK(p.x==-1 && p.y==2 && p.z==3,"NaN point: original asymmetric lower/upper choice");
    for(axis=0;axis<3;++axis)for(which=0;which<2;++which) {
        bounds(a);p.x=p.y=p.z=0;set_bound(a,axis+which*3,0x7fc00001);
        bh_world_clamp_point(a,&p);
        CHECK((axis==0 && which==0) || (axis!=0 && which==1) ?
            ((bits(v[axis])&0x7fffffff)>0x7f800000):v[axis]==0,
            "unordered bound preserves axis-specific FCOM branches");
    }
    for(axis=0;axis<3;++axis) {
        bounds(a);p.x=p.y=p.z=0;v[axis]=from_bits(0x7f800000);bh_world_clamp_point(a,&p);
        CHECK(v[axis]==(float)(axis+1),"positive infinity clamps to finite upper bound");
        bounds(a);p.x=p.y=p.z=0;v[axis]=from_bits(0xff800000);bh_world_clamp_point(a,&p);
        CHECK(v[axis]==-(float)(axis+1),"negative infinity clamps to finite lower bound");
        bounds(a);set_bound(a,axis,0);set_bound(a,axis+3,0);p.x=p.y=p.z=0;v[axis]=from_bits(0x80000000);
        bh_world_clamp_point(a,&p);CHECK(bits(v[axis])==0x80000000,"equal zero performs no write, retaining input negative zero");
    }
    _clearfp();
}
/* Independent bit-level reference for all aliases across the six stored bound
   words. Source-owned output can overlap bounds; compare whole actor bytes. */
static int unordered(unsigned long a,unsigned long b) {
    return (a&0x7fffffff)>0x7f800000 || (b&0x7fffffff)>0x7f800000;
}
static void aliasing(BHWorldGeometry *a) {
    BHWorldGeometry expected;unsigned long offset,axis,values[9],i,low,high,point;
    float pl,ll,hl;
    for(offset=0;offset<4;++offset) {
        bounds(a);for(i=0;i<6;++i) {values[i]=bits((float)((long)i-4));memcpy((unsigned char *)&a->minimum+4*i,values+i,4);}
        expected=*a;
        for(axis=0;axis<3;++axis) {
            memcpy(&point,(unsigned char *)&expected.minimum+4*(offset+axis),4);
            memcpy(&low,(unsigned char *)&expected.minimum+4*axis,4);
            memcpy(&high,(unsigned char *)&expected.maximum+4*axis,4);
            pl=from_bits(point);ll=from_bits(low);hl=from_bits(high);
            if((axis==0 && unordered(point,low)) || pl<ll)point=low;
            else if((axis!=0 && unordered(point,high)) || pl>hl)point=high;
            memcpy((unsigned char *)&expected.minimum+4*(offset+axis),&point,4);
        }
        bh_world_clamp_point(a,(BHVec3 *)((unsigned char *)&a->minimum+4*offset));
        CHECK(!memcmp(a,&expected,sizeof(*a)),"sequential alias output preserves later bound reads");
    }
}
int bh_target_steering_checkpoint(unsigned long *count) {
    BHWorldGeometry world;BHMathEnvironment env;
    checks=failures=0;CHECK(bh_math_environment_enter(&env),"real numerical environment");
    memset(&world,0,sizeof(world));finite(&world);exceptional(&world);aliasing(&world);
    bh_math_environment_leave(&env);
    if(count)*count=checks;
    printf("{\"scenario\":\"target-steering\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_TARGET_STEERING_STANDALONE
int main(void) {return bh_target_steering_checkpoint(NULL);}
#endif
