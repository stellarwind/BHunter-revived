#include "../src/bh_camera_queue.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL camera-queue: %s\n",s);}} while(0)
static int close_to(float a,double b) {return fabs((double)a-b)<0.002;}

static void queue_and_pose(void) {
    BHDrawQueue q;BHMainCamera camera;BHViewConfig view;
    BHVec3 position={10,20,30},angles={0,0,0};unsigned long next=BH_DRAW_END;
    float expected[6]={10,20,30,0,0,0};
    memset(&camera,0,sizeof(camera));
    CHECK(bh_draw_queue_init(&q,12,NULL),"queue initialized");
    CHECK(bh_view_config_init(&view,640,480) && bh_main_camera_settings(&view,60,.01,25000),"real projection settings");
    CHECK(bh_camera_queue_pose(&q,&position,&angles)==1,"pose producer");
    CHECK(q.commands[0].next==9 && q.commands[0].resource==9 && q.commands[9].next==1,
        "header zero inserts before numbered drawing buckets");
    CHECK(q.commands[9].kind==BH_DRAW_CAMERA && !memcmp(&q.commands[9].resource,expected,24),
        "six exact binary32 stores at original command offsets");
    position.x=999;angles.y=1;
    CHECK(bh_camera_queue_execute(&view,&camera,&q,q.commands+9,&next) && next==1,
        "consumer owns copied pose and forwards next link");
    CHECK(close_to(camera.position.x,10) && close_to(camera.position.y,20) && close_to(camera.position.z,30) &&
        camera.view.m[0][3]==-10 && camera.view.m[1][3]==-20 && camera.view.m[2][3]==-30,
        "real pose owner restores translation and eye");
    CHECK(close_to(camera.focal,320.0/tan(3.141592653589793/6)) &&
        close_to(camera.projected.m[0][3],-10*camera.focal-30*320.0),
        "real projection owner builds translated projection");
    position.x=4;position.y=5;position.z=6;angles.x=1.5707963267948966f;angles.y=angles.z=0;
    CHECK(bh_camera_queue_pose(&q,&position,&angles)==1 && q.commands[9].next==10 && q.commands[10].next==1,
        "camera commands retain insertion order");
    CHECK(bh_camera_queue_execute(&view,&camera,&q,q.commands+10,&next) &&
        close_to(camera.view.m[0][2],-1) && close_to(camera.view.m[2][0],1) &&
        close_to(camera.view.m[0][3],6) && close_to(camera.view.m[2][3],-4),
        "quarter-turn yaw uses recovered negative world rotation");
    CHECK(close_to(camera.position.x,4) && close_to(camera.position.y,5) && close_to(camera.position.z,6),
        "rotated inverse recovers eye");
    CHECK(bh_draw_queue_dispose(&q),"queue disposed");
}

static void controls(void) {
    BHDrawQueue q;BHMainCamera camera,before;BHViewConfig view;BHVec3 p={0,0,0},a={0,0,0};
    unsigned long next=123,nan_bits=0x7fc00123UL;
    memset(&camera,0,sizeof(camera));
    CHECK(bh_draw_queue_init(&q,10,NULL) && bh_view_config_init(&view,640,480),"control fixture");
    CHECK(bh_camera_queue_pose(&q,&p,&a)==1 && bh_camera_queue_pose(&q,&p,&a)==0 &&
        q.count==10 && q.error==BH_DRAW_FULL,"full queue drops camera command");
    CHECK(bh_draw_queue_begin(&q),"reset control queue");
    memcpy(&p.y,&nan_bits,4);
    CHECK(bh_camera_queue_pose(&q,&p,&a)==1 && q.commands[9].words[0]==nan_bits,
        "producer preserves nonfinite payload bits");
    before=camera;
    CHECK(!bh_camera_queue_execute(&view,&camera,&q,q.commands+9,&next) && !memcmp(&camera,&before,sizeof(camera)) && next==1,
        "actual pose owner rejects nonfinite input without camera mutation");
    q.commands[9].next=100;next=123;
    CHECK(!bh_camera_queue_execute(&view,&camera,&q,q.commands+9,&next) && next==123,
        "invalid next rejected before applying pose");
    CHECK(bh_draw_queue_begin(&q),"topology reset");q.commands[0].resource=100;
    CHECK(bh_camera_queue_pose(&q,&p,&a)==-1 && q.error==BH_DRAW_TOPOLOGY && q.count==9,
        "invalid tail cannot allocate command");
    CHECK(bh_camera_queue_pose(NULL,&p,&a)==-1 && bh_camera_queue_pose(&q,NULL,&a)==-1,
        "missing producer bindings rejected");
    CHECK(!bh_camera_queue_execute(&view,&camera,&q,NULL,&next) &&
        !bh_camera_queue_execute(&view,&camera,&q,q.commands+1,NULL),"missing consumer bindings rejected");
    CHECK(bh_draw_queue_dispose(&q),"controls disposed");
}
int bh_camera_queue_tests(void) {
    checks=failures=0;queue_and_pose();controls();
    printf("camera-queue: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_camera_queue_checkpoint(unsigned long *count) {
    bh_camera_queue_tests();if(count)*count=checks;
    printf("{\"scenario\":\"camera-queue\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_CAMERA_QUEUE_STANDALONE
int main(void) {unsigned long count=0;return bh_camera_queue_checkpoint(&count);}
#endif
