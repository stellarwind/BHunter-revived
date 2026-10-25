#include "../src/bh_route_query.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL route query line %d: %s\n",line,label);}}
static void ring(BHWorldGeometry *p,unsigned long count) {
    unsigned long i;memset(p,0,count*sizeof(*p));
    for(i=0;i<count;++i){p[i].navigation.next=&p[(i+1)%count].navigation;p[i].minimum.x=p[i].minimum.y=p[i].minimum.z=-1;p[i].maximum.x=p[i].maximum.y=p[i].maximum.z=1;}
}
static void ownership(void) {
    BHWorldGeometry p[4];BHVec3 point={0,0,0};unsigned long bits=0x3f7fffffUL;
    ring(p,3);p[0].navigation.portals[0].neighbor=&p[1].navigation;
    CHECK(bh_world_locate(p,3,p,&point,0)==p+1,"portal neighbor wins before containing current node");
    CHECK(bh_world_locate(p,3,NULL,&point,0)==p,"without hint scan starts at scene first");
    CHECK(bh_world_locate(p,3,p+2,&point,0)==p+2,"hint changes ring scan origin");
    p[1].owner=p;CHECK(bh_world_locate(p,3,p,&point,0)==p,"owned child excluded before current fallback");
    point.x=point.y=point.z=1;CHECK(bh_world_locate(p,3,NULL,&point,0)==p,"all maximum boundaries inclusive");
    point.x=point.y=point.z=-1;CHECK(bh_world_locate(p,3,NULL,&point,-0.0f)==p,"minimum boundaries and signed zero margin");
    point.x=1.25f;point.y=point.z=0;
    CHECK(!bh_world_locate(p,3,NULL,&point,0) && bh_world_locate(p,3,NULL,&point,0.25f)==p,"positive margin expands all axis tests");
    point.x=0.75f;CHECK(!bh_world_locate(p,3,NULL,&point,-0.5f),"negative margin shrinks original bounds");
    CHECK(!bh_world_locate(NULL,3,p,&point,0),"missing global-equivalent scene returns null even with hint");
    CHECK(!bh_world_locate(p,3,p+3,&point,0),"foreign hint rejected");
    p[0].navigation.portals[0].neighbor=&p[3].navigation;CHECK(!bh_world_locate(p,3,p,&point,0),"foreign hinted portal target rejected safely");
    p[2].navigation.next=&p[1].navigation;CHECK(!bh_world_locate(p,3,NULL,&point,0),"malformed scene traversal bounded");
    ring(p,1);p[0].minimum.x=1;p[0].maximum.x=2;memcpy(&point.x,&bits,4);point.y=point.z=0;
    CHECK(!bh_world_locate(p,1,NULL,&point,3.0f/67108864.0f),"margin sum compared before binary32 rounding");
    point.x=1.5f;CHECK(bh_world_locate(p,1,NULL,&point,-0.4f)==p && !bh_world_locate(p,1,NULL,&point,-0.6f),"signed inset preserves both side comparisons");
}
static void portal_geometry(void) {
    BHPortal p;BHVec3 out,old,normal;unsigned long direction,i,word;
    memset(&p,0,sizeof(p));p.plane=-17;p.scratch[0]=2;p.scratch[1]=4;p.scratch[2]=6;p.scratch[3]=10;
    p.overlap[0]=p.overlap[1]=100;p.overlap[2]=p.overlap[3]=200;
    for(direction=0;direction<=6;++direction)if(direction!=3) {
        float *v=(float*)&out,*n=(float*)&normal;unsigned long axis=direction&3;
        p.direction=direction;CHECK(bh_portal_center(&p,&out) && bh_portal_normal(&p,&normal),"portal geometry direction supported");
        CHECK(v[axis]==-17 && n[axis]==((direction&4)?-1:1),"integer plane and signed axis normal");
        for(i=0;i<3;++i)if(i!=axis){memcpy(&word,n+i,4);CHECK(word==0,"other normal axes are exact positive zero");}
        if(axis==0)CHECK(out.y==3 && out.z==8,"X portal reads scratch centers");
        if(axis==1)CHECK(out.x==3 && out.z==8,"Y portal reads scratch centers");
        if(axis==2)CHECK(out.x==3 && out.y==8,"Z portal reads scratch centers");
    }
    old=out;p.direction=3;CHECK(!bh_portal_center(&p,&out) && !memcmp(&old,&out,12),"invalid direction leaves center unchanged");
    old=normal;CHECK(!bh_portal_normal(&p,&normal) && !memcmp(&old,&normal,12),"invalid direction leaves normal unchanged");
    p.direction=0;p.plane=2147483647L;p.scratch[0]=p.scratch[1]=FLT_MAX;
    CHECK(bh_portal_center(&p,&out) && out.x==2147483648.0f && out.y==FLT_MAX,"plane conversion and midpoint avoid premature float sum overflow");
}
static void planning(void) {
    BHWorldGeometry p[2];BHNavigation n;BHRoutePlan plan,old;BHVec3 lookup={10,0,0},goal={10,2,3};BHPortal *port;
    ring(p,2);p[1].minimum.x=9;p[1].maximum.x=11;
    p[0].navigation.portals[0].neighbor=&p[1].navigation;p[1].navigation.portals[0].neighbor=&p[0].navigation;
    port=p[0].navigation.portals;port->plane=1;port->direction=0;port->scratch[0]=port->scratch[2]=-1;port->scratch[1]=port->scratch[3]=1;
    bh_navigation_init(&n,NULL);CHECK(bh_navigation_build(&n,p,2,NULL),"real navigation dependency for route planner");
    CHECK(bh_route_plan(&n,p,2,p,&lookup,&goal,&plan) && plan.next==p+1 && plan.exit_slot==0,"junction local route chooses real next owner");
    CHECK(!memcmp(&plan.goal,&goal,12) && plan.portal_center.x==1 && plan.portal_center.y==0 && plan.portal_center.z==0,"separate lookup and projection goals retained");
    CHECK(plan.projected.x==10 && plan.projected.y==0 && plan.projected.z==0 && !memcmp(&plan.portal_center,&plan.anchor,12),"normal-line projection and duplicated anchor");
    CHECK(fabs(plan.parameter_rate-(float)(4.666666666666667/(sqrt(94.0)+9)))<0.0000001,"distance-sum parameter rate");
    old=plan;lookup.x=100;CHECK(!bh_route_plan(&n,p,2,p,&lookup,&goal,&plan) && !memcmp(&old,&plan,sizeof(plan)),"missing destination rejects transactionally");
    lookup.x=0;CHECK(!bh_route_plan(&n,p,2,p,&lookup,&goal,&plan) && !memcmp(&old,&plan,sizeof(plan)),"unassigned diagonal route is not a guessed exit");
    lookup.x=10;goal.x=1;goal.y=goal.z=0;
    CHECK(bh_route_plan(&n,p,2,p,&lookup,&goal,&plan) && !_finite(plan.parameter_rate) && plan.parameter_rate>0,"zero-length original parameter rate remains positive infinity");
    bh_navigation_dispose(&n);_clearfp();
}
int bh_route_query_checkpoint(unsigned long *count) {
    checks=0;failures=0;ownership();portal_geometry();planning();*count=checks;
    printf("{\"scenario\":\"route-query\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
