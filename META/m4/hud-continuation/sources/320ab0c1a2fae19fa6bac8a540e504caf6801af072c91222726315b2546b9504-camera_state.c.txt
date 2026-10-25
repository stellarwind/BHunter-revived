#include "../src/bh_camera_state.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line) {
    ++checks;if(!ok){++failures;if(failures<16)printf("FAIL camera state %d: %s\n",line,label);}
}
static void word(unsigned char *p,unsigned long offset,unsigned long value) {
    memcpy(p+offset,&value,4);
}
static void initialization(void) {
    BHCameraState state,before;unsigned char expected[0x9c];unsigned long i,n;
    static const unsigned long clears[]={0x60,0x64,0x68,0x6c,0x70,0x74,0x90,0x94,0x98};
    static const long modes[]={0,1,8,10,11,-1,0x12345678};
    CHECK(sizeof(BHCameraModeState)==48 && sizeof(state)==156,"exact actor-embedded camera extent");
    CHECK(offsetof(BHCameraState,previous)==0x30 && offsetof(BHCameraState,look_yaw)==0x60 && offsetof(BHCameraState,blend_remaining)==0x68 && offsetof(BHCameraState,position)==0x78 && offsetof(BHCameraState,velocity)==0x90,"recovered independent offsets");
    for(n=0;n<sizeof(modes)/sizeof(modes[0]);++n) {
        for(i=0;i<sizeof(state);++i)((unsigned char *)&state)[i]=(unsigned char)(i*29+n*7);
        before=state;memcpy(expected,&before,sizeof(expected));
        word(expected,0,(unsigned long)modes[n]);word(expected,4,1);
        for(i=0;i<sizeof(clears)/sizeof(clears[0]);++i)word(expected,clears[i],0);
        CHECK(bh_camera_state_init(&state,modes[n]),"initialize player/flyby and arbitrary original mode word");
        CHECK(!memcmp(expected,&state,sizeof(state)),"all eleven written words and every preserved byte match offset oracle");
    }
    CHECK(!bh_camera_state_init(0,1),"null initialization guard");
}
static void transitions(void) {
    BHCameraState state,before;unsigned char expected[0x9c];unsigned long i,n;long mode;
    const double pi=3.14159265358979323846;double angle,wrapped;float output;
    for(n=0;n<96;++n) {
        for(i=0;i<sizeof(state);++i)((unsigned char *)&state)[i]=(unsigned char)(i*31+n);
        state.current.mode=3;state.current.first_update=0;state.current.seed_yaw=9.25f;
        state.elapsed=12.5;state.blend_remaining=.125;
        mode=(n&1)?8:11;angle=((long)n-48)*.375;state.angles.x=(float)angle;before=state;
        memcpy(expected,&before,sizeof(expected));memcpy(expected+0x30,expected,0x30);
        word(expected,0,(unsigned long)mode);word(expected,4,1);
        word(expected,0x68,0);word(expected,0x6c,0x3fe00000);word(expected,0x70,0);word(expected,0x74,0);
        wrapped=(double)before.angles.x+pi;
        while(wrapped>=pi)wrapped-=2*pi;
        while(wrapped< -pi)wrapped+=2*pi;
        output=(float)wrapped;memcpy(expected+8,&output,4);
        CHECK(bh_camera_state_set_mode(&state,mode),"special mode transition succeeds");
        CHECK(!memcmp(expected,&state,sizeof(state)),"independent repeated-subtraction yaw oracle and exact untouched bytes");
        CHECK(state.previous.mode==3 && state.previous.seed_yaw==9.25f,"snapshot precedes mode/reset writes");
        before=state;CHECK(bh_camera_state_set_mode(&state,mode) && !memcmp(&before,&state,sizeof(state)),"same-mode call is byte-preserving including blend and elapsed");
    }
    for(mode=-2;mode<14;++mode)if(mode!=8 && mode!=11) {
        memset(&state,0x6a,sizeof(state));state.current.mode=31;state.elapsed=19;before=state;
        memcpy(expected,&before,sizeof(expected));memcpy(expected+0x30,expected,0x30);
        word(expected,0,(unsigned long)mode);word(expected,4,1);word(expected,0x68,0);word(expected,0x6c,0x3fe00000);
        CHECK(bh_camera_state_set_mode(&state,mode) && !memcmp(expected,&state,sizeof(state)),"ordinary mode snapshot and .5 blend preserve elapsed and seed yaw");
    }
    state.current.mode=3;word((unsigned char *)&state,0x84,0x7f800000);before=state;
    CHECK(!bh_camera_state_set_mode(&state,8) && !memcmp(&before,&state,sizeof(state)),"nonfinite special-mode source guard rejects before snapshot");
    CHECK(bh_camera_state_set_mode(&state,4),"ordinary mode does not consume output yaw");
    CHECK(!bh_camera_state_set_mode(0,8),"null transition guard");
}
/* Isolated install-only pointer identity fixtures. These are test doubles,
   never runtime camera correction implementations and never called here. */
static void correction_a(BHVec3 *eye,const BHVec3 *target,unsigned long room) {(void)eye;(void)target;(void)room;}
static void correction_b(BHVec3 *eye,const BHVec3 *target,unsigned long room) {(void)eye;(void)target;(void)room;}
static void callback_owner(void) {
    BHCameraState state,before;BHCameraStateContext context;
    memset(&state,0xc7,sizeof(state));before=state;context.correction=0;
    CHECK(bh_camera_state_set_correction(&context,&state,correction_a) && context.correction==correction_a,"shared owner installs pointer without calling it");
    CHECK(!memcmp(&state,&before,sizeof(state)),"ignored camera state is byte-preserved");
    CHECK(bh_camera_state_set_correction(&context,0,correction_b) && context.correction==correction_b,"null ignored-state still replaces shared callback");
    CHECK(bh_camera_state_set_correction(&context,&state,0) && !context.correction,"null callback clears shared correction");
    CHECK(!bh_camera_state_set_correction(0,&state,correction_a),"null shared-owner source guard");
}
static int close_value(double a,double b) {return fabs(a-b)<=6e-6*(1+fabs(b));}
static double wrap_reference(double value) {
    const double pi=3.14159265358979323846;
    while(value>=pi)value-=2*pi;
    while(value< -pi)value+=2*pi;
    return value;
}
static unsigned long corrections,correction_room;static BHVec3 correction_target;
static void correction_move(BHVec3 *eye,const BHVec3 *target,unsigned long room) {
    ++corrections;eye->x+=1;correction_room=room;correction_target=*target;
}
static void smoothing(void) {
    BHCameraModeState mode,before;BHCameraStateContext context;
    BHVec3 desired,angles;double expected,delta;float old_product;unsigned long n,i;
    memset(&context,0,sizeof(context));context.target.x=9;context.actor_token=17;
    memset(&mode,0x6a,sizeof(mode));mode.first_update=1;before=mode;
    desired.x=3;desired.y=4;desired.z=5;angles.x=.1f;angles.y=.2f;angles.z=.3f;
    context.correction=correction_move;corrections=0;
    bh_camera_mode_smooth(&context,&mode,&desired,&angles,.8,.9);
    CHECK(corrections==1 && correction_room==17 && correction_target.x==9 && desired.x==4,"correction runs before first update and changes borrowed candidate");
    CHECK(!mode.first_update && mode.position.x==4 && mode.position.y==4 && mode.position.z==5 && !memcmp(&mode.angles,&angles,sizeof(angles)),"first update copies corrected pose without retention");
    CHECK(mode.mode==before.mode && mode.seed_yaw==before.seed_yaw && !memcmp(mode.mode_data,before.mode_data,sizeof(mode.mode_data)),"smoothing preserves all non-pose mode storage");
    context.correction=0;
    for(n=0;n<80;++n) {
        memset(&mode,0,sizeof(mode));
        mode.position.x=(float)(n*.03125);mode.position.y=(float)(n*-.0625);mode.position.z=(float)(n*.125);
        mode.angles.x=(float)(n*.07-2.8);mode.angles.y=(float)(2.8-n*.07);mode.angles.z=(float)(n*.03-1.2);
        desired.x=(float)(7-n*.125);desired.y=(float)(n*.25-9);desired.z=(float)(5-n*.5);
        angles.x=(float)(2.9-n*.05);angles.y=(float)(n*.06-2.9);angles.z=(float)(1.1-n*.04);before=mode;
        bh_camera_mode_smooth(&context,&mode,&desired,&angles,.8,.9);
        for(i=0;i<3;++i) {
            old_product=(float)(((float *)&before.position)[i]*.8);
            expected=((float *)&desired)[i]*.2+old_product;
            CHECK(close_value(((float *)&mode.position)[i],expected),"position smoothing uses separately rounded old product");
            delta=(float)(((float *)&angles)[i]-((float *)&before.angles)[i]);
            expected=wrap_reference(wrap_reference(delta)*.1+((float *)&before.angles)[i]);
            CHECK(close_value(((float *)&mode.angles)[i],expected),"shortest-angle smoothing against independent subtraction oracle");
        }
    }
    mode.first_update=0;mode.angles.x=3;angles=mode.angles;angles.x=-3;desired=mode.position;
    bh_camera_mode_smooth(&context,&mode,&desired,&angles,.5,.5);
    CHECK(close_value(fabs(mode.angles.x),3.141592653589793),"seam crossing takes short path through pi rather than zero");
    mode.angles.x=mode.angles.y=mode.angles.z=1e-7f;
    angles.x=angles.y=angles.z=3.1415927410125732f;
    bh_camera_mode_smooth(&context,&mode,&desired,&angles,.5,.5);
    CHECK(mode.angles.x<0 && mode.angles.y>0 && mode.angles.z>0,"original X delta spills before upper comparison while Y Z compare retained difference");
    mode.angles.x=mode.angles.y=mode.angles.z=-1e-7f;
    angles.x=angles.y=angles.z=-3.1415927410125732f;
    bh_camera_mode_smooth(&context,&mode,&desired,&angles,.5,.5);
    CHECK(mode.angles.x>0 && mode.angles.y>0 && mode.angles.z>0,"negative wrap compares reloaded binary32 candidate despite positive-bound comparison retaining original");
}
static void make_input(BHCameraInput *input) {
    memset(input,0,sizeof(*input));input->basis_x.x=1;input->basis_y.y=1;input->basis_z.z=1;input->actor_token=0x1234;
}
static void matrix_identity(double m[3][3]) {
    unsigned long i,j;for(i=0;i<3;++i)for(j=0;j<3;++j)m[i][j]=(i==j)?1:0;
}
static void matrix_append(double a[3][3],const double b[3][3]) {
    double result[3][3];unsigned long i,j,k;
    for(i=0;i<3;++i)for(j=0;j<3;++j){result[i][j]=0;for(k=0;k<3;++k)result[i][j]+=a[i][k]*b[k][j];}
    memcpy(a,result,sizeof(result));
}
static void matrix_angle(double a[3][3],double angle,int axis) {
    double rotation[3][3],c=cos(angle),s=sin(angle);matrix_identity(rotation);
    if(axis==0){rotation[1][1]=rotation[2][2]=c;rotation[1][2]=-s;rotation[2][1]=s;}
    if(axis==1){rotation[0][0]=rotation[2][2]=c;rotation[0][2]=s;rotation[2][0]=-s;}
    if(axis==2){rotation[0][0]=rotation[1][1]=c;rotation[0][1]=-s;rotation[1][0]=s;}
    matrix_append(a,rotation);
}
static void composed_look(void) {
    BHCameraState state;BHCameraStateContext context;BHCameraInput input;
    double expected[3][3],actual[3][3];unsigned long i,j,n;
    for(n=0;n<40;++n) {
        memset(&state,0,sizeof(state));memset(&context,0,sizeof(context));make_input(&input);
        input.angles.x=(float)((long)n*.03-.6);input.angles.y=(float)(.4-(long)n*.02);input.angles.z=(float)((long)n*.01-.2);
        bh_camera_state_init(&state,0);
        CHECK(bh_camera_state_update(&context,&state,&input,.1,.7f,-.5f),"nonzero yaw pitch roll and combined look full update");
        CHECK(close_value(state.look_yaw,.175) && close_value(state.look_pitch,-.125),"unclamped requests retain quarter-delta slew");
        matrix_identity(expected);matrix_angle(expected,input.angles.x,1);matrix_angle(expected,input.angles.y,0);matrix_angle(expected,input.angles.z,2);
        matrix_angle(expected,state.look_yaw*1.0471975511965976,1);matrix_angle(expected,state.look_pitch*1.0471975511965976,0);
        matrix_identity(actual);matrix_angle(actual,state.angles.x,1);matrix_angle(actual,state.angles.y,0);matrix_angle(actual,state.angles.z,2);
        for(i=0;i<3;++i)for(j=0;j<3;++j)CHECK(close_value(expected[i][j],actual[i][j]),"reconstructed final Euler basis equals independent five-matrix camera composition");
    }
}
static void updates(void) {
    BHCameraState state,before;BHCameraStateContext context,saved_context;BHCameraInput input;
    double expected;float product;unsigned long i,n;
    memset(&state,0,sizeof(state));memset(&context,0,sizeof(context));make_input(&input);
    input.target.x=10;input.target.y=20;input.target.z=30;
    input.angles.x=.5f;input.angles.y=-.25f;input.angles.z=.125f;
    bh_camera_state_init(&state,0);
    CHECK(bh_camera_state_update(&context,&state,&input,.1,0,0),"real mode0 full camera update");
    CHECK(close_value(state.position.x,10) && close_value(state.position.y,19.7) && close_value(state.position.z,30),"mode0 first eye uses target minus .3 basis_y");
    CHECK(!memcmp(&state.angles,&input.angles,sizeof(state.angles)) && !state.current.first_update,"unblended eye and angles publish real mode pose");
    CHECK(close_value(state.velocity.x,100) && close_value(state.velocity.y,197) && close_value(state.velocity.z,300),"first update velocity uses previous published eye and inverse dt");
    CHECK(context.elapsed==.1 && state.elapsed==.1 && context.actor_token==input.actor_token && !memcmp(&context.target,&input.target,sizeof(input.target)),"elapsed target and opaque room shared by all mode dispatch");
    before=state;input.target.x+=2;input.target.y+=4;input.target.z+=8;
    CHECK(bh_camera_state_update(&context,&state,&input,.1,0,0),"second real mode update smooths");
    for(i=0;i<3;++i) {
        product=(float)(((float *)&before.current.position)[i]*.05);
        expected=(((float *)&input.target)[i]-(i==1?(float).3:0))*.95+product;
        CHECK(close_value(((float *)&state.position)[i],expected),"mode0 repeat position independently composes retention");
        CHECK(close_value(((float *)&state.velocity)[i],(((float *)&state.position)[i]-((float *)&before.position)[i])*10),"per-axis source camera velocity");
    }
    memset(&state,0,sizeof(state));make_input(&input);bh_camera_state_init(&state,0);
    bh_camera_state_update(&context,&state,&input,.125,0,0);
    CHECK(bh_camera_state_set_mode(&state,1),"player mode1 transition starts blend");
    CHECK(bh_camera_state_update(&context,&state,&input,.125,0,0),"full mode0 to player mode1 real table blend");
    CHECK(state.previous.mode==0 && state.current.mode==1 && state.blend_remaining==.375,"transition previous mode advances while timer counts down");
    CHECK(close_value(state.position.y,-.85) && close_value(state.position.z,-1.875),".75 old plus .25 new mode uses actual target offsets");
    CHECK(close_value(state.velocity.y,-4.4) && close_value(state.velocity.z,-15),"blended camera velocity tracks published eye");
    CHECK(bh_camera_state_update(&context,&state,&input,.5,0,0) && state.blend_remaining==0,"expired blend clamps timer and stops previous dispatch");
    CHECK(!memcmp(&state.position,&state.current.position,sizeof(state.position)),"expired blend directly publishes current mode");
    /* Explicit isolated correction fixture tests call count/order. No runtime
       camera collision callback is supplied by these tests. */
    memset(&state,0,sizeof(state));memset(&context,0,sizeof(context));make_input(&input);
    bh_camera_state_init(&state,0);bh_camera_state_update(&context,&state,&input,.1,0,0);
    bh_camera_state_set_mode(&state,1);context.correction=correction_move;corrections=0;
    CHECK(bh_camera_state_update(&context,&state,&input,.1,0,0) && corrections==4,"blended frame repeats real original correction at both mode candidates previous eye and final eye");
    CHECK(correction_room==input.actor_token,"all correction callbacks receive actor token without float conversion");
    context.correction=0;
    memset(&state,0,sizeof(state));bh_camera_state_init(&state,0);make_input(&input);
    CHECK(bh_camera_state_update(&context,&state,&input,.01,1,0),"horizontal look slew and decomposition");
    CHECK(close_value(state.look_yaw,.08) && close_value(state.angles.x,.08*1.0471975511965977) && close_value(state.angles.y,0) && close_value(state.angles.z,0),"positive look clamp times 60-degree conversion produces expected yaw");
    memset(&state,0,sizeof(state));bh_camera_state_init(&state,0);
    CHECK(bh_camera_state_update(&context,&state,&input,.01,0,-1),"vertical look slew and decomposition");
    CHECK(close_value(state.look_pitch,-.08) && close_value(state.angles.y,-.08*1.0471975511965977) && close_value(state.angles.x,0) && close_value(state.angles.z,0),"negative look clamp produces expected pitch");
    for(n=0;n<5;++n) {
        before=state;saved_context=context;
        if(n==0)expected=0;else if(n==1)expected=-.1;else expected=.1;
        if(n==2)state.current.mode=12;
        if(n==3)word((unsigned char *)&input,0,0x7f800000);
        if(n==4)word((unsigned char *)&state,0x74,0x7ff00000);
        before=state;
        CHECK(!bh_camera_state_update(&context,&state,&input,expected,0,0) && !memcmp(&state,&before,sizeof(state)) && !memcmp(&context,&saved_context,sizeof(context)),"invalid update guard preserves both owning records");
        state.current.mode=0;make_input(&input);
    }
    CHECK(!bh_camera_state_update(0,&state,&input,.1,0,0) && !bh_camera_state_update(&context,0,&input,.1,0,0) && !bh_camera_state_update(&context,&state,0,.1,0,0),"null update bindings reject");
    for(n=0;n<12;++n) {
        memset(&state,0,sizeof(state));memset(&context,0,sizeof(context));make_input(&input);
        input.target.x=3;input.target.y=-5;input.target.z=7;input.angles.x=.25f;input.angles.y=-.125f;input.angles.z=.0625f;
        bh_camera_state_init(&state,(long)n);state.current.seed_yaw=.375f;
        state.position.x=1;state.position.y=2;state.position.z=3;before=state;
        CHECK(bh_camera_state_update(&context,&state,&input,.125,0,0),"all twelve actual table entries link and execute outer camera owner");
        CHECK(_finite(state.position.x) && _finite(state.position.y) && _finite(state.position.z) && _finite(state.angles.x) && _finite(state.angles.y) && _finite(state.angles.z),"every actual mode produces finite regular fixture pose");
        CHECK(!memcmp(&state.position,&state.current.position,sizeof(state.position)) && !memcmp(&state.angles,&state.current.angles,sizeof(state.angles)),"all unblended modes publish exact state pose");
        for(i=0;i<3;++i)
            CHECK(close_value(((float *)&state.velocity)[i],(((float *)&state.position)[i]-((float *)&before.position)[i])*8),"all-mode velocity independently follows output displacement");
    }
}
int bh_camera_state_checkpoint(unsigned long *count) {
    BHMathEnvironment environment;checks=0;failures=0;
    if(!bh_math_environment_enter(&environment))return 1;
    initialization();transitions();callback_owner();smoothing();updates();composed_look();
    bh_math_environment_leave(&environment);if(count)*count=checks;
    printf("{\"scenario\":\"camera-state\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
