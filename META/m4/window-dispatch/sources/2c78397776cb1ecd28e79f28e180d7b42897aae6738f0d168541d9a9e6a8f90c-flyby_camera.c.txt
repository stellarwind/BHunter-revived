#include "../src/bh_flyby_camera.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL flyby-camera: %s\n",s);}} while(0)
static int close_to(float actual,double expected){return fabs((double)actual-expected)<.002;}
typedef struct Fixture {
    BHWorldGeometry world[2];BHNavigation nav;BHCameraRouteContext route;BHCameraStateContext camera;
    BHPlayerCameraStart starts[18];BHVec3 points[18];BHAtlasImage atlas;BHAtlasPage page;
    BHTextQueue text;BHFlybyCredits credits;BHFlybyCameraContext context;BHFlybyCameraState state;
} Fixture;
static int setup(Fixture *f) {
    unsigned long i;memset(f,0,sizeof(*f));
    for(i=0;i<2;++i){f->world[i].navigation.next=&f->world[(i+1)%2].navigation;f->world[i].previous=f->world+(i+1)%2;
        f->world[i].navigation.count=1;f->world[i].navigation.portals[0].neighbor=&f->world[(i+1)%2].navigation;
        f->world[i].navigation.portals[0].plane=i?90:10;f->world[i].navigation.portals[0].direction=i?4:0;
        f->world[i].minimum.x=i?90:-10;f->world[i].maximum.x=i?110:10;
        f->world[i].minimum.y=f->world[i].minimum.z=-50;f->world[i].maximum.y=f->world[i].maximum.z=50;}
    if(!bh_navigation_init(&f->nav,NULL) || !bh_navigation_build(&f->nav,f->world,2,NULL))return 0;
    for(i=0;i<18;++i){f->starts[i].position.x=10;f->starts[i].position.y=20;f->starts[i].position.z=30;f->points[i]=f->starts[i].position;}
    f->starts[1].position.x=100;f->starts[1].position.y=f->starts[1].position.z=0;f->points[1]=f->starts[1].position;
    f->route.navigation=&f->nav;f->route.first=f->world;f->route.scene_count=2;f->route.waypoints=f->points;f->route.waypoint_count=18;
    f->atlas.rectangle_count=96;for(i=0;i<96;++i){f->atlas.rectangles[i].width=8;f->atlas.rectangles[i].height=12;}
    f->page.atlas=&f->atlas;f->page.size=128;bh_text_queue_init(&f->text);f->credits.gate=-1;
    f->context.route=&f->route;f->context.camera=&f->camera;f->context.starts=f->starts;f->context.start_count=18;
    f->context.actor_token=123;f->context.credit_font=&f->page;f->context.text=&f->text;f->context.credits=&f->credits;
    f->state.route.current=f->world;f->state.route.next=f->world+1;
    bh_camera_state_init(&f->state.camera,1);return 1;
}
static void cleanup(Fixture *f){bh_navigation_dispose(&f->nav);}
static void curves(Fixture *f,float phase) {
    unsigned long i;f->state.route.phase=phase;f->state.route.parameter_rate=.01f;
    for(i=0;i<4;++i){f->state.route.curve[i].x=20+(float)i*10;f->state.route.curve[i].y=f->state.route.curve[i].z=0;}
}
static void body(void) {
    Fixture f;unsigned long i;float radius=25,height=10;BHVec3 expected;double t;
    CHECK(setup(&f),"complete source dependency fixture");
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1,"stable orbit step through actual camera owner");
    CHECK(f.state.position.x==4 && f.state.position.y==8 && f.state.position.z==12 &&
        f.state.velocity.x==120 && f.state.velocity.y==240 && f.state.velocity.z==360,"actual .6/.4 filter and thirty-Hz velocity");
    CHECK(close_to(f.state.angles.x,atan2(4.0,12)) && f.state.angles.y==-4 && f.state.angles.z==0,
        "motion-derived yaw pitch and zero old-motion curvature");
    CHECK(f.camera.actor_token==123 && f.state.camera.elapsed==0.03333333333333333 &&
        _finite(f.state.camera.position.x),"real camera mode lifecycle and actor token");cleanup(&f);
    CHECK(setup(&f),"oscillation fixture");memcpy(f.starts[0].metadata,&radius,4);memcpy(f.starts[0].metadata+1,&height,4);
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1,"nonzero orbit amplitudes");t=1.0/30;
    expected.x=(float)(10+25*cos(1+t*.037)*sin(t*.071)*cos(t*.217));
    expected.y=(float)(20+10*((sin(1+t*.087)*cos(t*.041))*cos(t*.173)));
    expected.z=(float)(30+25*((sin(1+t*.057)*cos(t*.091))*cos(t*.13)));
    CHECK(close_to(f.state.position.x,(double)expected.x*.4) && close_to(f.state.position.y,(double)expected.y*.4) &&
        close_to(f.state.position.z,(double)expected.z*.4),"raw-sign independent orbit fixture");cleanup(&f);
    for(i=0;i<3;++i){CHECK(setup(&f),"phase fixture");curves(&f,(float)i+.5f);
        CHECK(bh_flyby_camera_step(&f.context,&f.state)==1,"all three curve phase branches execute");
        CHECK(close_to(f.state.position.x,i==1?14:9) && close_to(f.state.position.y,i==1?0:4) &&
            close_to(f.state.position.z,i==1?0:6),"correct orbit/curve blend at phase midpoint");cleanup(&f);}
}
static void transitions(void) {
    Fixture f;unsigned long selected;
    CHECK(setup(&f),"plan fixture");f.route.selected_waypoint=1;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.next==f.world+1 && f.state.route.phase>0 &&
        f.state.camera.current.mode==10,"real route planner starts selected waypoint transition");cleanup(&f);
    CHECK(setup(&f),"route crossing fixture");f.route.selected_waypoint=1;curves(&f,.99f);f.state.route.parameter_rate=.02f;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.current==f.world+1 &&
        f.state.route.waypoint_index==1 && f.state.route.phase>=2,"phase-one crossing invokes actual route advance and final approach");cleanup(&f);
    CHECK(setup(&f),"route middle crossing fixture");f.route.selected_waypoint=1;curves(&f,1.99f);f.state.route.parameter_rate=.02f;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.current==f.world+1 && f.state.route.waypoint_index==1,
        "phase-two crossing invokes complete route advance");cleanup(&f);
    CHECK(setup(&f),"retained phase-one boundary fixture");f.route.selected_waypoint=1;
    curves(&f,.999999940395355224609375f);f.state.route.parameter_rate=.00000007450580596923828125f;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.waypoint_index==1,
        "retained sum crossing one advances even when stored phase rounds to one");cleanup(&f);
    CHECK(setup(&f),"retained phase-two boundary fixture");f.route.selected_waypoint=1;
    curves(&f,1.99999988079071044921875f);f.state.route.parameter_rate=.0000001490116119384765625f;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.waypoint_index==1,
        "retained sum crossing two advances even when stored phase rounds to two");cleanup(&f);
    CHECK(setup(&f),"retained phase-three boundary fixture");curves(&f,2.9999997615814208984375f);
    f.state.route.parameter_rate=.000000298023223876953125f;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.phase==0,
        "retained sum crossing three resets even when stored phase rounds to three");cleanup(&f);
    for(selected=13;selected<=17;++selected){CHECK(setup(&f),"exit mode fixture");f.route.selected_waypoint=selected;
        f.state.route.waypoint_index=selected;curves(&f,2.99f);f.state.route.parameter_rate=.02f;
        CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.state.route.phase==0 &&
            f.state.camera.current.mode==(selected==13?11:selected<=16?8:10),"all original destination camera mode choices");cleanup(&f);}
}
static void credit_pose(Fixture *f,double yaw) {
    f->state.route.waypoint_index=13;f->route.selected_waypoint=13;
    f->starts[13].position.x=f->starts[13].position.y=f->starts[13].position.z=0;f->points[13]=f->starts[13].position;
    f->state.position.x=(float)(-sin(yaw)*10);f->state.position.z=(float)(-cos(yaw)*10);
    f->state.previous_position=f->state.older_position=f->state.position;
}
static void credits_and_guards(void) {
    Fixture f;BHFlybyCameraState before;
    CHECK(setup(&f),"credits introduction fixture");f.credits.gate=0;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.credits.gate==-1 && f.credits.index==1 &&
        bh_text_queue_count(&f.text)==6,"complete original introduction emits clear and five text lines");
    CHECK(f.text.commands[0].kind==BH_TEXT_COMMAND_CLEAR && !strcmp((char *)f.text.commands[1].text,"BHUNTER") &&
        f.text.commands[1].position.xy[0]==36 && f.text.commands[1].position.xy[1]==1 &&
        f.text.commands[2].position.xy[1]==22,"real font width centers text and blank line adds seven pixels");
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && bh_text_queue_count(&f.text)==6,"introduction gate suppresses repeated text");cleanup(&f);
    CHECK(setup(&f),"credits arm fixture");credit_pose(&f,2.3);f.credits.gate=0;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.credits.gate==1 && bh_text_queue_count(&f.text)==0 &&
        fabs(f.state.orbit_time-.05)<1e-12,"actual yaw arm window and special fifty-millisecond orbit time");cleanup(&f);
    CHECK(setup(&f),"credits trigger fixture");credit_pose(&f,-.5);f.credits.gate=1;f.credits.index=14;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==1 && f.credits.gate==0 && f.credits.index==0 &&
        bh_text_queue_count(&f.text)==5 && !strcmp((char *)f.text.commands[1].text,"THANKS TO:"),"actual yaw trigger publishes last literal credits and wraps index");cleanup(&f);
    CHECK(setup(&f),"queue full fixture");f.credits.gate=0;f.text.write=255;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==0 && f.credits.gate==-1 && f.credits.index==1 &&
        f.state.position.x==4 && bh_text_queue_count(&f.text)==255,"full text ring still advances real camera and original credits gate");
    before=f.state;f.state.route.phase=4;before=f.state;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==-1 && !memcmp(&f.state,&before,sizeof(before)),"invalid phase rejects before state mutation");cleanup(&f);
    CHECK(setup(&f),"partially full text ring fixture");f.credits.gate=0;f.text.write=254;
    CHECK(bh_flyby_camera_step(&f.context,&f.state)==0 && bh_text_queue_count(&f.text)==255 &&
        f.text.commands[254].kind==BH_TEXT_COMMAND_CLEAR && f.credits.gate==-1 && f.credits.index==1,
        "last free record holds clear while every text line drops and credits advance");cleanup(&f);
}
int bh_flyby_camera_tests(void) {
    checks=failures=0;body();transitions();credits_and_guards();
    printf("flyby-camera: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_flyby_camera_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;if(!bh_math_environment_enter(&fp))return 2;
    bh_flyby_camera_tests();bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"flyby-camera\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_FLYBY_CAMERA_STANDALONE
int main(void){unsigned long count=0;return bh_flyby_camera_checkpoint(&count);}
#endif
