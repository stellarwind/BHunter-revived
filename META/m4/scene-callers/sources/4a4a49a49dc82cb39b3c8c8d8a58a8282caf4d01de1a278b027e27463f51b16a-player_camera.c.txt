#include "../src/bh_player_camera.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line) {++checks;if(!ok){++failures;if(failures<16)printf("FAIL player camera %d: %s\n",line,label);}}
static int close_value(double a,double b) {return fabs(a-b)<=6e-5*(1+fabs(b));}
static BHVec3 vector_at(const BHEntity *player,unsigned long offset) {BHVec3 v;memcpy(&v,(const unsigned char *)player+offset,12);return v;}
static void put_vector(BHEntity *player,unsigned long offset,float x,float y,float z) {BHVec3 v;v.x=x;v.y=y;v.z=z;memcpy((unsigned char *)player+offset,&v,12);}
static void prepare(BHEntity *player,BHCameraStateContext *camera) {
    BHCameraState *state;memset(player,0xa5,sizeof(*player));memset(camera,0,sizeof(*camera));
    player->position.x=2;player->position.y=-3;player->position.z=5;
    state=bh_player_camera_state(player);memset(state,0,sizeof(*state));bh_camera_state_init(state,0);
    state->current.position.x=10;state->current.position.y=20;state->current.position.z=30;
    state->previous.position.x=40;state->previous.position.y=50;state->previous.position.z=60;
    state->position.x=70;state->position.y=80;state->position.z=90;
    put_vector(player,0x298,99,98,97);put_vector(player,0x2a4,96,95,94);
}
static int changed_range(unsigned long offset) {
    return (offset>=0xc && offset<0x78) || (offset>=0x84 && offset<0xb4) ||
        (offset>=0xe8 && offset<0x184) || (offset>=0x298 && offset<0x2b2);
}
static void identity_reset(void) {
    BHEntity player,before;BHCameraStateContext camera;BHPlayerCameraStart starts[2];BHPlayerCameraContext context;
    BHCameraState *state;BHVec3 v;BHMat34 matrix;unsigned long i;unsigned short active;
    memset(starts,0,sizeof(starts));starts[1].position.x=100;starts[1].position.y=-20;starts[1].position.z=30;
    starts[1].metadata[0]=0x12345678;starts[1].metadata[1]=0xabcdef;
    prepare(&player,&camera);before=player;
    context.player=&player;context.starts=starts;context.start_count=2;context.player_count=1;context.local_player_index=7;context.camera=&camera;
    CHECK(sizeof(BHPlayerCameraStart)==32 && offsetof(BHPlayerCameraStart,angles)==12 && offsetof(BHPlayerCameraStart,metadata)==24,"original start-pose stride and untouched metadata offsets");
    CHECK(bh_player_camera_reset(&context,1),"complete indexed player reset and real fifty-update camera settle");
    state=bh_player_camera_state(&player);
    CHECK(!memcmp(&player.position,&starts[1].position,12),"single player uses indexed position unchanged");
    v=vector_at(&player,0x18);CHECK(v.x==1 && v.y==0 && v.z==0,"identity right basis in actual actor+18");
    v=vector_at(&player,0x24);CHECK(v.x==0 && v.y==1 && v.z==0,"identity up basis in actual actor+24");
    v=vector_at(&player,0x30);CHECK(v.x==0 && v.y==0 && v.z==1,"identity forward basis in actual actor+30");
    for(i=0;i<4;++i){v=vector_at(&player,0x48+12*i);CHECK(v.x==0 && v.y==0 && v.z==0,"four original actor motion vectors clear");}
    memcpy(&matrix,(const unsigned char *)&player+0x84,sizeof(matrix));
    CHECK(matrix.m[0][0]==1 && matrix.m[1][1]==1 && matrix.m[2][2]==1 && matrix.m[0][3]==100 && matrix.m[1][3]==-20 && matrix.m[2][3]==30,"actual actor world matrix includes new position");
    CHECK(state->current.mode==1 && !state->current.first_update && state->blend_remaining==0,"real source mode1 installed settled and blend expired");
    CHECK(close_value(state->position.x,100) && close_value(state->position.y,-22.5) && close_value(state->position.z,22.5),"fifty updates converge actual chase eye using real handlers");
    CHECK(close_value(camera.elapsed,50.0/30) && close_value(state->elapsed,50.0/30) && camera.actor_token==(unsigned long)&player,"exact fifty1/30 camera calls carry actual actor identity");
    v=vector_at(&player,0x298);CHECK(!memcmp(&v,&player.position,12),"post-reset player position snapshot+298");
    v=vector_at(&player,0x2a4);CHECK(v.x==0 && v.y==0 && v.z==0,"post-reset vector+2a4 clears");
    memcpy(&active,(const unsigned char *)&player+0x2b0,2);CHECK(active==1 && *((const unsigned char *)&player+0x2b2)==*((const unsigned char *)&before+0x2b2),"16-bit active write preserves adjoining actor bytes");
    for(i=0;i<sizeof(player);++i)if(!changed_range(i))CHECK(((unsigned char *)&player)[i]==((unsigned char *)&before)[i],"unowned actor bytes preserved through real camera settling");
    CHECK(starts[1].metadata[0]==0x12345678 && starts[1].metadata[1]==0xabcdef,"reset preserves borrowed start metadata");
}
typedef struct Capture {BHEntity *player;BHVec3 output,current,previous;unsigned long count,token;} Capture;
/* Isolated inspection double; leaves eye unchanged and never substitutes a
   camera mode, rotation owner, world trace or runtime collision behavior. */
static void capture_pose(BHVec3 *eye,const BHVec3 *target,unsigned long token,void *user) {
    Capture *capture=(Capture *)user;BHCameraState *state=bh_player_camera_state(capture->player);
    (void)eye;(void)target;
    if(!capture->count){capture->output=state->position;capture->current=state->current.position;capture->previous=state->previous.position;}
    ++capture->count;capture->token=token;
}
static void histories_and_multiplayer(void) {
    BHEntity player;BHCameraStateContext camera;BHPlayerCameraStart start;BHPlayerCameraContext context;Capture capture;
    BHCameraState *state;unsigned long n;double shift;BHVec3 v;
    memset(&start,0,sizeof(start));start.position.x=10;start.position.y=20;start.position.z=30;
    context.player=&player;context.starts=&start;context.start_count=1;context.player_count=2;context.camera=&camera;
    for(n=0;n<5;++n) {
        prepare(&player,&camera);state=bh_player_camera_state(&player);state->current.mode=1;
        memset(&capture,0,sizeof(capture));capture.player=&player;
        bh_camera_state_bind_correction(&camera,capture_pose,&capture);context.local_player_index=(long)n;
        CHECK(bh_player_camera_reset(&context,0),"real multiplayer reset with contextual inspection callback");shift=1-n*.25;
        CHECK(close_value(player.position.x,10+shift) && player.position.y==20 && player.position.z==30,"multiplayer local-index adjustment retains original signed constants");
        CHECK(close_value(capture.output.x,70+8+shift) && capture.output.y==103 && capture.output.z==115,"published camera eye translated before synchronous source updates");
        CHECK(close_value(capture.current.x,10+8+shift) && capture.current.y==43 && capture.current.z==55,"current eye history translated before same-mode settling");
        CHECK(close_value(capture.previous.x,40+8+shift) && capture.previous.y==73 && capture.previous.z==85,"previous eye history receives same translation even when mode unchanged");
        CHECK(capture.count==50 && capture.token==(unsigned long)&player,"same mode does exactly fifty actual source callbacks with real actor identity");
        v=state->previous.position;CHECK(!memcmp(&v,&capture.previous,12),"inactive previous mode history stays translated after settling");
    }
}
static void rotated_and_guards(void) {
    BHEntity player,before;BHCameraStateContext camera,saved;BHPlayerCameraStart start;BHPlayerCameraContext context;
    BHMat34 m;BHVec3 x,y,z,pose;double c,s,cy,sy,cx,sx,cz,sz,expected[3][3];unsigned long i,j,n;unsigned long infinity=0x7f800000;
    memset(&start,0,sizeof(start));context.player=&player;context.starts=&start;context.start_count=1;context.player_count=1;context.local_player_index=0;context.camera=&camera;
    for(n=0;n<32;++n) {
        prepare(&player,&camera);start.position.x=(float)n;start.position.y=(float)(n*-.5);start.position.z=(float)(n*.25);
        start.angles.x=(float)(n*.03-.4);start.angles.y=(float)(.3-n*.015);start.angles.z=(float)(n*.01-.2);
        CHECK(bh_player_camera_reset(&context,0),"rotated source player reset");
        cy=cos(start.angles.x);sy=sin(start.angles.x);cx=cos(start.angles.y);sx=sin(start.angles.y);cz=cos(start.angles.z);sz=sin(start.angles.z);
        expected[0][0]=cy*cz+sy*sx*sz;expected[0][1]=-cy*sz+sy*sx*cz;expected[0][2]=sy*cx;
        expected[1][0]=cx*sz;expected[1][1]=cx*cz;expected[1][2]=-sx;
        expected[2][0]=-sy*cz+cy*sx*sz;expected[2][1]=sy*sz+cy*sx*cz;expected[2][2]=cy*cx;
        memcpy(&m,(unsigned char *)&player+0x84,sizeof(m));x=vector_at(&player,0x18);y=vector_at(&player,0x24);z=vector_at(&player,0x30);pose=vector_at(&player,0x3c);
        CHECK(!memcmp(&pose,&start.angles,12),"original binary32 start angles publish to actor");
        for(i=0;i<3;++i)for(j=0;j<3;++j)CHECK(close_value(m.m[i][j],expected[i][j]),"independent expanded Ry Rx Rz world matrix");
        for(i=0;i<3;++i)CHECK(close_value(((float *)&x)[i],expected[i][0]) && close_value(((float *)&y)[i],expected[i][1]) && close_value(((float *)&z)[i],expected[i][2]),"actor basis columns agree with independently expanded matrix");
    }
    prepare(&player,&camera);before=player;saved=camera;
    CHECK(!bh_player_camera_reset(&context,1) && !memcmp(&before,&player,sizeof(player)) && !memcmp(&saved,&camera,sizeof(camera)),"start index guard preserves actor and camera context");
    memcpy(&start.angles.x,&infinity,4);
    CHECK(!bh_player_camera_reset(&context,0) && !memcmp(&before,&player,sizeof(player)),"nonfinite start pose rejects before actor mutation");
    CHECK(!bh_player_camera_reset(0,0) && !bh_player_camera_state(0),"null reset and field binding guards");
    (void)c;(void)s;
}
int bh_player_camera_checkpoint(unsigned long *count) {
    BHMathEnvironment environment;checks=0;failures=0;if(!bh_math_environment_enter(&environment))return 1;
    identity_reset();histories_and_multiplayer();rotated_and_guards();bh_math_environment_leave(&environment);if(count)*count=checks;
    printf("{\"scenario\":\"player-camera\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
