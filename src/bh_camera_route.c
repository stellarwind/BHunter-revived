#include "bh_camera_route.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>

static float stored(double value){float out;bh_round_float(&out,value);return out;}
static int fail(BHCameraRouteContext *c,BHCameraRouteError error){if(c)c->error=error;return 0;}
static int finite_vector(const BHVec3 *v){return _finite(v->x) && _finite(v->y) && _finite(v->z);}
static long entry_port(const BHWorldGeometry *room,const BHWorldGeometry *previous) {
    unsigned long i;long found=-1;
    for(i=0;i<room->navigation.count;++i)if(room->navigation.portals[i].neighbor==&previous->navigation)found=(long)i;
    return found;
}
static int point(const BHWorldGeometry *room,long slot,BHVec3 *center,BHVec3 *normal) {
    return slot>=0 && (unsigned long)slot<room->navigation.count &&
        bh_portal_center(room->navigation.portals+slot,center) &&
        bh_portal_normal(room->navigation.portals+slot,normal);
}
static void project(const BHVec3 *from,const BHVec3 *toward,const BHVec3 *normal,BHVec3 *out,int terminal) {
    double dx=(double)toward->x-from->x,dy=(double)toward->y-from->y,dz=(double)toward->z-from->z,dot;
    if(terminal)dz=stored(dz);
    dot=(dx*normal->x+dy*normal->y)+dz*normal->z;
    out->x=stored((double)from->x+dot*normal->x);
    out->y=stored(dot*normal->y+from->y);out->z=stored(dot*normal->z+from->z);
}
static double length(const BHVec3 *a,const BHVec3 *b,int terminal_last) {
    double x=(double)a->x-b->x,y=(double)a->y-b->y,z=(double)a->z-b->z;
    if(terminal_last)z=stored(z);
    return sqrt((x*stored(x)+y*stored(y))+z*stored(z));
}
static void detour(const BHWorldGeometry *room,BHVec3 *a,BHVec3 *b) {
    BHVec3 center;
    center.x=stored((double)stored((double)room->minimum.x*.5)-(double)room->maximum.x*(-.5));
    center.y=stored((double)stored((double)room->minimum.y*.5)-(double)room->maximum.y*(-.5));
    center.z=stored((double)stored((double)room->minimum.z*.5)-(double)room->maximum.z*(-.5));
    *a=*b=center;a->x=stored((double)a->x-10);a->z=stored((double)a->z-10);
    b->x=stored((double)b->x-(-10));b->z=stored((double)b->z-(-10));
}
int __cdecl bh_camera_route_advance(BHCameraRouteContext *c,BHCameraRouteState *s) {
    BHCameraRouteState next;BHWorldGeometry *room,*destination,*previous;
    BHVec3 incoming_normal,outgoing_normal;long current_index,destination_index,incoming,outgoing=-1,whole;
    unsigned long i;double sum,rate,time;float rounded_time;int branch;
    if(!c || !s || !c->navigation || !c->navigation->nodes || !c->first || !c->scene_count ||
       c->scene_count>65535 || !c->waypoints || !c->waypoint_count ||
       !_finite(s->phase) || s->phase< -2147483648.0 || s->phase>=2147483648.0 ||
       !_finite(s->parameter_rate) || s->parameter_rate<=0)
        return fail(c,BH_CAMERA_ROUTE_ARGUMENT);
    if(c->selected_waypoint>=c->waypoint_count || !finite_vector(c->waypoints+c->selected_waypoint))
        return fail(c,BH_CAMERA_ROUTE_WAYPOINT);
    room=s->next;previous=s->current;
    current_index=bh_navigation_find(c->navigation,room);
    if(current_index<0 || bh_navigation_find(c->navigation,previous)<0 || room->navigation.count>6)
        return fail(c,BH_CAMERA_ROUTE_TOPOLOGY);
    incoming=entry_port(room,previous);if(incoming<0)return fail(c,BH_CAMERA_ROUTE_PORTAL);
    next=*s;next.current=room;
    if((unsigned long)current_index>=c->navigation->junction_count) {
        branch=0;
        for(i=0;i<room->navigation.count;++i)if(room->navigation.portals[i].neighbor &&
            room->navigation.portals[i].neighbor!=&previous->navigation)outgoing=(long)i;
        if(outgoing<0)return fail(c,BH_CAMERA_ROUTE_PORTAL);
        next.next=(BHWorldGeometry *)room->navigation.portals[outgoing].neighbor;
    } else {
        destination=bh_world_locate(c->first,c->scene_count,NULL,c->waypoints+c->selected_waypoint,0);
        if(!destination)return fail(c,BH_CAMERA_ROUTE_WAYPOINT);
        if(destination==room) {
            branch=2;next.waypoint_index=c->selected_waypoint;
            if(!point(room,incoming,next.curve,&incoming_normal))return fail(c,BH_CAMERA_ROUTE_PORTAL);
            next.curve[2]=next.curve[0];next.curve[3]=c->waypoints[next.waypoint_index];
            project(next.curve,next.curve+3,&incoming_normal,next.curve+1,1);
        } else {
            branch=1;destination_index=bh_navigation_find(c->navigation,destination);
            if(destination_index<0 || (unsigned long)destination_index>=c->navigation->junction_count ||
               c->navigation->routes.node_count!=c->navigation->junction_count || !c->navigation->routes.cells)
                return fail(c,BH_CAMERA_ROUTE_TOPOLOGY);
            outgoing=c->navigation->routes.cells[(unsigned long)destination_index*c->navigation->junction_count+current_index].exit_slot;
            if(outgoing<0 || (unsigned long)outgoing>=room->navigation.count || !room->navigation.portals[outgoing].neighbor)
                return fail(c,BH_CAMERA_ROUTE_PORTAL);
            next.next=(BHWorldGeometry *)room->navigation.portals[outgoing].neighbor;
        }
    }
    if(branch!=2) {
        if(bh_navigation_find(c->navigation,next.next)<0 ||
           !point(room,incoming,next.curve,&incoming_normal) ||
           !point(room,outgoing,next.curve+3,&outgoing_normal))return fail(c,BH_CAMERA_ROUTE_PORTAL);
        project(next.curve,next.curve+3,&incoming_normal,next.curve+1,0);
        project(next.curve+3,next.curve,&outgoing_normal,next.curve+2,0);
        if(branch==1 && incoming==outgoing) {
            if(!finite_vector(&room->minimum) || !finite_vector(&room->maximum))return fail(c,BH_CAMERA_ROUTE_TOPOLOGY);
            detour(room,next.curve+1,next.curve+2);
        }
    }
    whole=(long)s->phase;time=((double)s->phase-whole)/s->parameter_rate;
    sum=length(next.curve,next.curve+1,0)+length(next.curve+3,next.curve+2,branch==2);
    if(branch==0 || branch==2)sum=stored(sum);
    rate=4.666666666666667/sum;next.parameter_rate=stored(rate);
    if(branch==2)next.phase=stored(time*next.parameter_rate-(-2.0));
    else {rounded_time=stored(time);next.phase=stored((double)rounded_time*(branch==0?rate:next.parameter_rate)-(-1.0));}
    *s=next;c->error=BH_CAMERA_ROUTE_OK;return 1;
}
