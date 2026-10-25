#include "../src/bh_camera_route.h"
#include "../src/bh_camera_curve.h"
#include "../src/bh_fp.h"
#include "route_query_cases.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL camera-route: %s\n",s);}} while(0)
static int close_to(float actual,double expected){return fabs((double)actual-expected)<.0001;}
static void ring(BHWorldGeometry *w,unsigned long count) {
    unsigned long i;memset(w,0,count*sizeof(*w));for(i=0;i<count;++i){
        w[i].navigation.next=&w[(i+1)%count].navigation;w[i].previous=w+(i+count-1)%count;
        w[i].minimum.x=(float)i*10-1;w[i].maximum.x=(float)i*10+1;
        w[i].minimum.y=w[i].minimum.z=-1;w[i].maximum.y=w[i].maximum.z=1;}
}
static void connect(BHWorldGeometry *a,BHWorldGeometry *b,long plane,unsigned long direction) {
    BHPortal *p=a->navigation.portals+a->navigation.count++;memset(p,0,sizeof(*p));
    p->neighbor=&b->navigation;p->plane=plane;p->direction=direction;
}
static void state(BHCameraRouteState *s,BHWorldGeometry *previous,BHWorldGeometry *next) {
    memset(s,0,sizeof(*s));s->current=previous;s->next=next;s->phase=1.25f;s->parameter_rate=.25f;s->waypoint_index=999;
}
static void context(BHCameraRouteContext *c,const BHNavigation *n,BHWorldGeometry *w,unsigned long count,const BHVec3 *goal) {
    memset(c,0,sizeof(*c));c->navigation=n;c->first=w;c->scene_count=count;c->waypoints=goal;c->waypoint_count=1;
}
static void chain(void) {
    BHWorldGeometry w[3];BHNavigation n;BHCameraRouteContext c;BHCameraRouteState s,old;BHVec3 goal={20,0,0};
    ring(w,3);connect(w,w+1,1,0);connect(w+1,w,9,4);connect(w+1,w+2,11,0);connect(w+2,w+1,19,4);
    CHECK(bh_navigation_init(&n,NULL) && bh_navigation_build(&n,w,3,NULL),"real navigation dependency constructs corridor graph");
    context(&c,&n,w,3,&goal);state(&s,w,w+1);
    CHECK(bh_camera_route_advance(&c,&s) && s.current==w+1 && s.next==w+2 && s.waypoint_index==999,
        "corridor promotes room and preserves waypoint selection");
    CHECK(s.curve[0].x==9 && s.curve[1].x==11 && s.curve[2].x==9 && s.curve[3].x==11,
        "corridor portal normals construct all four controls");
    CHECK(close_to(s.parameter_rate,7.0/6) && close_to(s.phase,1+7.0/6),"corridor remaps fractional phase with retained rate");
    state(&s,w+1,w+2);
    CHECK(bh_camera_route_advance(&c,&s) && s.current==w+2 && s.next==w+2 && s.waypoint_index==0,
        "destination room publishes selected real waypoint index");
    CHECK(s.curve[0].x==19 && s.curve[1].x==20 && s.curve[2].x==19 && s.curve[3].x==20 &&
        close_to(s.parameter_rate,7.0/3) && close_to(s.phase,2+7.0/3),"destination final approach controls and phase2 offset");
    state(&s,w,w+1);old=s;c.selected_waypoint=1;
    CHECK(!bh_camera_route_advance(&c,&s) && c.error==BH_CAMERA_ROUTE_WAYPOINT && !memcmp(&s,&old,sizeof(s)),
        "invalid waypoint index preserves route state");
    c.selected_waypoint=0;w[1].navigation.portals[0].neighbor=NULL;
    CHECK(!bh_camera_route_advance(&c,&s) && c.error==BH_CAMERA_ROUTE_PORTAL && !memcmp(&s,&old,sizeof(s)),
        "missing incoming portal cannot invent entry slot");
    w[1].navigation.portals[0].neighbor=&w[0].navigation;
    w[1].navigation.portals[0].plane=w[1].navigation.portals[1].plane=10;
    CHECK(bh_camera_route_advance(&c,&s) && !_finite(s.parameter_rate) && s.parameter_rate>0 && !_finite(s.phase),
        "zero control lengths retain original masked infinity behavior");
    _clearfp();bh_navigation_dispose(&n);
}
static void junctions(void) {
    BHWorldGeometry w[4];BHNavigation n;BHCameraRouteContext c;BHCameraRouteState s;BHVec3 goal={30,0,0};
    ring(w,4);connect(w,w+1,1,0);connect(w,w+2,-1,4);connect(w,w+3,1,2);
    connect(w+1,w,9,4);connect(w+2,w,19,4);connect(w+3,w,29,4);
    CHECK(bh_navigation_init(&n,NULL) && bh_navigation_build(&n,w,4,NULL),"actual source junction matrix construction");
    context(&c,&n,w,4,&goal);state(&s,w+1,w);
    CHECK(bh_camera_route_advance(&c,&s) && s.current==w && s.next==w+3 &&
        s.curve[0].x==1 && s.curve[3].z==1,"destination-major real routing cell selects correct exit");
    CHECK(s.curve[1].x==0 && s.curve[1].z==0 && s.curve[2].x==0 && s.curve[2].z==0 &&
        close_to(s.parameter_rate,7.0/3),"perpendicular portal projections and length-derived rate");
    bh_navigation_dispose(&n);
    ring(w,2);connect(w,w+1,1,0);connect(w+1,w,9,4);goal.x=10;
    CHECK(bh_navigation_init(&n,NULL) && bh_navigation_build(&n,w,2,NULL),"same-portal graph constructed");
    context(&c,&n,w,2,&goal);state(&s,w+1,w);
    CHECK(bh_camera_route_advance(&c,&s) && s.next==w+1 &&
        s.curve[1].x==-10 && s.curve[1].y==0 && s.curve[1].z==-10 &&
        s.curve[2].x==10 && s.curve[2].y==0 && s.curve[2].z==10,
        "same incoming and outgoing portal uses actual room center detour");
    CHECK(close_to(s.parameter_rate,4.666666666666667/(sqrt(221.0)+sqrt(181.0))),"detour rate uses both curved endpoint distances");
    bh_navigation_dispose(&n);
}
int bh_camera_route_real_check(const BHNavigation *n,BHWorldGeometry *world,unsigned long world_count,unsigned long *count) {
    BHCameraRouteContext c;BHCameraRouteState s;BHVec3 goal,point;unsigned long row,i,before=checks,old_failures=failures;long incoming;
    for(row=0;row<4;++row) {
        const unsigned long *expected=route_query_cases[row];BHWorldGeometry *room=n->nodes[expected[0]].placement;
        memcpy(&goal,expected+4,12);incoming=-1;
        for(i=0;i<room->navigation.count;++i)if(room->navigation.portals[i].neighbor && i!=expected[2]){incoming=(long)i;break;}
        CHECK(incoming>=0,"real route case has distinct incoming portal");if(incoming<0)continue;
        context(&c,n,world,world_count,&goal);state(&s,(BHWorldGeometry *)room->navigation.portals[incoming].neighbor,room);
        CHECK(bh_camera_route_advance(&c,&s),"real TEST.LEV graph advances camera route");
        CHECK(s.current==room && s.next==n->nodes[expected[3]].placement && !memcmp(s.curve+3,expected+7,12),
            "captured actual destination route and exit center reproduced");
        bh_camera_curve(.5f,&point,s.curve,s.curve+1,s.curve+2,s.curve+3);
        CHECK(_finite(s.parameter_rate) && s.parameter_rate>0 && _finite(point.x) && _finite(point.y) && _finite(point.z),
            "real controls feed complete recovered interpolation owner");
    }
    for(i=n->junction_count;i<n->node_count;++i) {
        BHWorldGeometry *room=n->nodes[i].placement;unsigned long j;BHWorldGeometry *previous=NULL,*following=NULL;
        for(j=0;j<room->navigation.count;++j)if(room->navigation.portals[j].neighbor){if(!previous)previous=(BHWorldGeometry *)room->navigation.portals[j].neighbor;else following=(BHWorldGeometry *)room->navigation.portals[j].neighbor;}
        if(!previous || !following || previous==following)continue;
        context(&c,n,world,world_count,&goal);state(&s,previous,room);
        CHECK(bh_camera_route_advance(&c,&s) && s.current==room && s.next==following,"actual source corridor chooses other real connected placement");break;
    }
    CHECK(i<n->node_count,"real corridor fixture found");
    {
        BHWorldGeometry *room=n->nodes[route_query_cases[0][1]].placement;
        memcpy(&goal,route_query_cases[0]+4,12);incoming=-1;
        for(i=0;i<room->navigation.count;++i)if(room->navigation.portals[i].neighbor){incoming=(long)i;break;}
        CHECK(incoming>=0,"actual selected goal room has incoming portal");
        if(incoming>=0){context(&c,n,world,world_count,&goal);state(&s,(BHWorldGeometry *)room->navigation.portals[incoming].neighbor,room);
            CHECK(bh_camera_route_advance(&c,&s) && s.current==room && s.next==room && s.waypoint_index==0,
                "actual selected goal room completes waypoint handoff");
            CHECK(!memcmp(s.curve+3,&goal,12) && s.phase>=2 && _finite(s.parameter_rate) && s.parameter_rate>0,
                "actual waypoint endpoint and phase2 approach retained");}
    }
    if(count)*count=checks-before;return failures==old_failures;
}
int bh_camera_route_tests(void) {
    checks=failures=0;chain();junctions();printf("camera-route: %lu checks, %lu failures\n",checks,failures);return failures==0;
}
int bh_camera_route_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;if(!bh_math_environment_enter(&fp))return 2;
    bh_camera_route_tests();bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"camera-route\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_CAMERA_ROUTE_STANDALONE
int main(void){unsigned long count=0;return bh_camera_route_checkpoint(&count);}
#endif
