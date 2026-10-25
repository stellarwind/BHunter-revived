#include "../src/bh_axis_snap.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL axis-snap: %s\n",s);}} while(0)
static unsigned long bits(float f) {unsigned long b;memcpy(&b,&f,4);return b;}
int bh_axis_snap_checkpoint(unsigned long *count) {
    BHMathEnvironment env;BHVec3 v;int x,y,z,axis;double length,expected;float rounded;unsigned long i;
    checks=failures=0;CHECK(bh_math_environment_enter(&env),"real numerical environment");
    for(x=-3;x<=3;++x)for(y=-3;y<=3;++y)for(z=-3;z<=3;++z) {
        v.x=(float)x;v.y=(float)y;v.z=(float)z;expected=sqrt((double)(x*x+y*y+z*z));
        axis=(abs(x)>abs(y) && abs(x)>abs(z))?0:(abs(y)>abs(z)?1:2);
        length=bh_vec3_snap_axis(&v);
        CHECK(length==expected,"unrounded returned length retained");
        for(i=0;i<3;++i)if((int)i!=axis)CHECK(bits(((float*)&v)[i])==0,"unselected components positive zero");
        bh_round_float(&rounded,(axis==0?x:(axis==1?y:z))>0?expected:-expected);
        CHECK(bits(((float*)&v)[axis])==bits(rounded),"dominant sign and z/y/x tie priority");
        if(x==0 && y==0 && z==0)CHECK(bits(v.z)==0x80000000,"all-zero tie writes negative zero to z");
    }
    v.x=1;v.y=1.0f/1024;v.z=0;length=bh_vec3_snap_axis(&v);
    CHECK(length!=(double)v.x && length>1,"return remains retained even when output coordinate rounded to float");
    bh_math_environment_leave(&env);if(count)*count=checks;
    printf("{\"scenario\":\"axis-snap\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_AXIS_SNAP_STANDALONE
int main(void) {return bh_axis_snap_checkpoint(NULL);}
#endif
