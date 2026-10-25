#include "../src/bh_camera_modes.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL camera modes %d: %s\n",__LINE__,s);}} while(0)
static int close_to(float a,double b) {return fabs((double)a-b)<.0001;}
static int vec(const BHVec3 *v,double x,double y,double z) {return close_to(v->x,x)&&close_to(v->y,y)&&close_to(v->z,z);}
static void input(BHCameraInput *in) {
    memset(in,0,sizeof(*in));in->target.x=10;in->target.y=20;in->target.z=30;
    in->basis_x.x=in->basis_y.y=in->basis_z.z=1;
}
static void all_initial(void) {
    static const float positions[12][3]={{10,19.7f,30},{10,17.5f,22.5f},{10,22,45},{10,5,30},
        {-5,20,30},{25,20,30},{10,35,30},{10,20,22},{10,20,40},{10,20,80},{10,17.5f,22.5f},{10,20,40}};
    static const double angles[12][3]={{0,0,0},{0,0,0},{3.141592653589793,0,0},{0,-1.5707963267948966,0},
        {1.5707963267948966,0,0},{-1.5707963267948966,0,0},{0,1.5707963267948966,0},
        {0,0,0},{3.141592653589793,0,0},{3.141592653589793,0,0},{0,0,0},{3.141592653589793,0,0}};
    BHCameraInput in;BHCameraModeState s;BHCameraStateContext c;int mode;input(&in);memset(&c,0,sizeof(c));
    for(mode=0;mode<12;++mode) {
        memset(&s,0,sizeof(s));s.mode=mode;s.first_update=1;
        CHECK(bh_camera_mode_update(&c,&s,&in),"actual table dispatches every supported mode");
        CHECK(!s.first_update && vec(&s.position,positions[mode][0],positions[mode][1],positions[mode][2]),"independent Cartesian first-position fixture");
        CHECK(vec(&s.angles,angles[mode][0],angles[mode][1],angles[mode][2]),"independent Cartesian first-angle fixture");
        if(mode==7 || mode==8 || mode==9 || mode==11)CHECK(!memcmp(s.mode_data,&in.target,12),"dynamic mode retains current target history");
    }
}
static void geometry(void) {
    BHVec3 origin={0,0,0},up={0,1,0},target,a;int x,z,y;double yaw,pitch;
    for(x=-2;x<=2;++x)for(z=-2;z<=2;++z)for(y=-2;y<=2;++y) {
        if(!x&&!z)continue;target.x=(float)x;target.y=(float)y;target.z=(float)z;
        bh_camera_look_at(&a,&target,&origin,&up);
        yaw=atan2((double)x,z);pitch=atan2(-(double)y,sqrt((double)(x*x+z*z)));
        CHECK(vec(&a,yaw,pitch,0),"independent look-at grid has zero bank and analytical yaw/pitch");
        a=target;bh_camera_look_at(&a,&a,&origin,&up);CHECK(vec(&a,yaw,pitch,0),"look-at output aliases target safely");
    }
    target.x=0;target.y=0;target.z=1;up.x=1;up.y=0;up.z=0;
    bh_camera_look_at(&a,&target,&origin,&up);CHECK(vec(&a,0,0,-1.5707963267948966),"rotated up vector produces negative quarter bank");
}
static void dynamic(void) {
    BHCameraInput in;BHCameraModeState s;BHCameraStateContext c;BHVec3 old;double phase,range;int m;
    input(&in);memset(&c,0,sizeof(c));c.elapsed=.25;
    for(m=8;m<=11;m+=3) {
        memset(&s,0,sizeof(s));s.mode=m;s.first_update=1;s.seed_yaw=.1f;
        CHECK(bh_camera_mode_update(&c,&s,&in),"timed orbit mode");
        phase=m==8?.25:(.25-sin(.25*3.7)*-0.2631578947368421)*.65;
        if(m==11)phase=(float)phase;
        CHECK(close_to(s.position.x,10+10*sin((double).1f+phase)) &&
              close_to(s.position.z,30+10*cos((double).1f+phase)),"independent timed orbit horizontal path");
        CHECK(s.position.y>20 && s.position.y<21,"orbit retains vertical wobble");
    }
    memset(&s,0,sizeof(s));s.mode=7;s.first_update=1;c.elapsed=0;
    CHECK(bh_camera_mode_update(&c,&s,&in),"trailing mode init");
    in.target.x+=2;old=s.position;
    CHECK(bh_camera_mode_update(&c,&s,&in) && s.position.x<old.x && s.position.z>old.z,"moving trailing camera uses previous target displacement");
    input(&in);memset(&s,0,sizeof(s));s.mode=9;s.first_update=1;srand(1);
    CHECK(bh_camera_mode_update(&c,&s,&in) && rand()==41,"held camera initialization consumes no random values");
    in.target.x=200;srand(1);
    CHECK(bh_camera_mode_update(&c,&s,&in) && rand()==26500,"distant camera restart consumes exactly three real CRT random calls");
    CHECK(vec(&s.position,250+(41-16384)/2048.0,20+(18467-16384)/2048.0,30+(6334-16384)/2048.0),"independent restart jitter fixture and signed random multiplier");
    old=s.position;srand(1);CHECK(bh_camera_mode_update(&c,&s,&in) && !memcmp(&s.position,&old,12) && rand()==41,"nearby held camera stays fixed and does not consume random state");
    s.mode=-1;old=s.position;CHECK(!bh_camera_mode_update(&c,&s,&in)&&!memcmp(&s.position,&old,12),"invalid mode rejected unchanged");
    CHECK(!bh_camera_mode_update(0,&s,&in)&&!bh_camera_mode_update(&c,0,&in)&&!bh_camera_mode_update(&c,&s,0),"missing dispatch bindings reject");
    (void)range;
}
int bh_camera_modes_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 1;
    all_initial();geometry();dynamic();bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"camera-modes\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
