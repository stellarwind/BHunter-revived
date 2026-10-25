#include "../src/bh_camera_curve.h"
#include "../src/bh_fp.h"
#include "route_query_cases.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL camera-curve: %s\n",s);}} while(0)
static int close_to(float actual,double expected,double tolerance){return fabs((double)actual-expected)<=tolerance;}
static void examples(void) {
    BHVec3 a={1,2,3},b={5,6,7},c={9,10,11},d={13,14,15},out;
    bh_camera_curve(0,&out,&a,&b,&c,&d);CHECK(!memcmp(&out,&a,12),"zero selects first endpoint");
    bh_camera_curve(1,&out,&a,&b,&c,&d);CHECK(!memcmp(&out,&d,12),"one selects final endpoint");
    bh_camera_curve(-.5f,&out,&a,&b,&c,&d);CHECK(out.x==-1 && out.y==0 && out.z==1,"negative parameter extrapolates first line");
    bh_camera_curve(1.5f,&out,&a,&b,&c,&d);CHECK(out.x==15 && out.y==16 && out.z==17,"parameter beyond one extrapolates final line");
    bh_camera_curve(.5f,&out,&a,&b,&c,&d);CHECK(out.x==7 && out.y==8 && out.z==9,"midpoint weights every endpoint equally");
    a.x=a.y=a.z=0;b.x=b.y=b.z=10;c.x=c.y=c.z=100;d.x=d.y=d.z=200;
    bh_camera_curve(.25f,&out,&a,&b,&c,&d);CHECK(out.x==21.640625f && out.y==out.x && out.z==out.x,"quarter smoothstep blend independent rational control");
    bh_camera_curve(.75f,&out,&a,&b,&c,&d);CHECK(out.x==148.828125f && out.y==out.x && out.z==out.x,"three-quarter independent rational control");
    bh_camera_curve(.0001f,&out,&a,&b,&c,&d);CHECK(close_to(out.x/.0001,10,.1),"left derivative agrees with first tangent");
    bh_camera_curve(.9999f,&out,&a,&b,&c,&d);CHECK(close_to((200-out.x)/.0001,100,.3),"right derivative agrees with last tangent");
}
static double polynomial(float t,double a,double b,double c,double d) {
    double x=t,x2=x*x,x3=x2*x,x4=x3*x;
    return a*(1-x-3*x2+5*x3-2*x4)+b*(x-3*x3+2*x4)+c*(3*x2-5*x3+2*x4)+d*(3*x3-2*x4);
}
static void real_route_points(void) {
    BHVec3 p[4],out;unsigned long row,i,j;float t;
    /* Captured real TEST.LEV route goal/portal/projected/anchor geometry,
       reused as independent finite control points. This does not claim that
       all callers choose these four fields in this order. */
    for(row=0;row<4;++row) {
        for(i=0;i<4;++i)memcpy(p+i,route_query_cases[row]+4+i*3,12);
        for(j=1;j<16;++j) {
            t=(float)j/16;bh_camera_curve(t,&out,p,p+1,p+2,p+3);
            CHECK(close_to(out.x,polynomial(t,p[0].x,p[1].x,p[2].x,p[3].x),.0003) &&
                close_to(out.y,polynomial(t,p[0].y,p[1].y,p[2].y,p[3].y),.0003) &&
                close_to(out.z,polynomial(t,p[0].z,p[1].z,p[2].z,p[3].z),.0003),
                "real route-point fixture agrees with independently expanded quartic weights");
        }
    }
}
static void aliases(void) {
    BHVec3 p[4]={{-7,2,15},{8,-4,3},{11,6,-5},{19,-8,9}},copy[4],out;unsigned long i;
    bh_camera_curve(.375f,&out,p,p+1,p+2,p+3);
    for(i=0;i<4;++i){memcpy(copy,p,sizeof(p));bh_camera_curve(.375f,copy+i,copy,copy+1,copy+2,copy+3);
        CHECK(!memcmp(copy+i,&out,12),"output may replace any entire input vector");}
}
int bh_camera_curve_tests(void) {
    checks=failures=0;examples();real_route_points();aliases();
    printf("camera-curve: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_camera_curve_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;if(!bh_math_environment_enter(&fp))return 2;
    bh_camera_curve_tests();bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"camera-curve\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_CAMERA_CURVE_STANDALONE
int main(void) {unsigned long count=0;return bh_camera_curve_checkpoint(&count);}
#endif

