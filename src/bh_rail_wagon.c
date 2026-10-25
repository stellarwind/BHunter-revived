#include "bh_rail_wagon.h"
#include "bh_route_query.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>
#define PI 3.14159265358979323846
typedef struct Private {BHRailWagonContext *context;BHWorldGeometry *world;unsigned long leader_generation,rider_generation;} Private;
typedef char wagon_layout[sizeof(BHRailWagonPose)==48&&offsetof(BHEntity,opaque0)==0x18&&
    offsetof(BHEntity,payload)+4==0xe8&&offsetof(BHRailWagonState,actor_next)==12&&
    offsetof(BHRailWagonState,kind)==20&&offsetof(BHRailWagonState,segment)==64&&
    offsetof(BHRailWagonState,checkpoint)==80&&sizeof(Private)+580<=636?1:-1];
static BHRailWagonState *state(BHEntity *e){return (BHRailWagonState*)(e->payload+4);}
static Private *private_data(BHEntity *e){return (Private*)(e->payload+580);}
BHRailWagonPose *__cdecl bh_rail_wagon_pose(BHEntity *e){return e?(BHRailWagonPose*)e->opaque0:NULL;}
static void failure(BHRailWagonContext *c,BHRailWagonError error){if(c){++c->failures;c->error=error;}}
static int live(BHRailWagonContext *c,BHEntity *e,unsigned long generation){BHEntity *p;unsigned long n=0;
    for(p=c->pool->active;p&&n++<c->pool->capacity;p=p->next)if(p==e)return p->generation==generation;return 0;}
static int member(BHRailWagonContext *c,BHEntity *e){BHEntity *p;unsigned long n=0;
    for(p=c->pool->active;p&&n++<c->pool->capacity;p=p->next)if(p==e)return 1;return 0;}
static int wagon(BHRailWagonContext *c,BHEntity *e){return e&&member(c,e)&&
    (e->step==bh_rail_wagon_tick||e->step==bh_rail_wagon_follow)&&state(e)->kind==9&&private_data(e)->context==c;}
BHRailWagonState *__cdecl bh_rail_wagon_state(BHEntity *e){return e&&(e->step==bh_rail_wagon_tick||e->step==bh_rail_wagon_follow)?state(e):NULL;}
static BHRailWagonContext *owner(BHEntity *e){BHRailWagonContext *c;
    if(!bh_rail_wagon_state(e))return NULL;c=private_data(e)->context;
    if(!c||!c->pool||!wagon(c,e))return NULL;return c;}
static int segment(BHRailWagonContext *c,BHRailSegment *s){unsigned long base=(unsigned long)c->rails->segments,at=(unsigned long)s;
    return s&&at>=base&&at-base<(unsigned long)c->rails->count*sizeof(*s)&&(at-base)%sizeof(*s)==0;}
static BHRailSegment *walk(BHRailWagonContext *c,BHRailSegment *s,int next){BHRailSegment *p;
    if(!segment(c,s)){failure(c,BH_WAGON_TOPOLOGY);return NULL;}p=next?s->next:s->previous;
    if(!segment(c,p)||!_finite(p->length)||p->length<=0){failure(c,BH_WAGON_TOPOLOGY);return NULL;}return p;}
static int ready(BHRailWagonContext *c,BHRailWagonState *s){if(!segment(c,s->segment)||!_finite(s->duration)||s->duration<=0||
    !_finite(s->speed)||s->speed<=0||!_finite(s->elapsed)||!bh_math_environment_supported()){failure(c,BH_WAGON_NUMERICAL);return 0;}return 1;}
static BHPickupRoom *room(BHRailWagonContext *c,BHWorldGeometry *w){unsigned long i;for(i=0;i<c->room_count;++i)if(c->rooms[i].world==w)return c->rooms+i;return NULL;}
static BHEntity *actor_next(BHEntity *e){BHEntity *p;memcpy(&p,e->payload+16,4);return p;}
static BHEntity *actor_previous(BHEntity *e){BHEntity *p;memcpy(&p,e->payload+20,4);return p;}
static void actor_next_set(BHEntity *e,BHEntity *v){memcpy(e->payload+16,&v,4);}
static void actor_previous_set(BHEntity *e,BHEntity *v){memcpy(e->payload+20,&v,4);}
static int unlink_room(BHRailWagonContext *c,BHEntity *e){BHRailWagonState *s=state(e);BHPickupRoom *r=room(c,private_data(e)->world);
    if(!r||(s->actor_previous&&(!member(c,s->actor_previous)||actor_next(s->actor_previous)!=e))||
       (!s->actor_previous&&r->actors!=e)||(s->actor_next&&(!member(c,s->actor_next)||actor_previous(s->actor_next)!=e))){failure(c,BH_WAGON_WORLD);return 0;}
    if(s->actor_previous)actor_next_set(s->actor_previous,s->actor_next);else r->actors=s->actor_next;
    if(s->actor_next)actor_previous_set(s->actor_next,s->actor_previous);return 1;}
static void prepend(BHRailWagonContext *c,BHEntity *e,BHWorldGeometry *w){BHRailWagonState *s=state(e);BHPickupRoom *r=room(c,w);
    private_data(e)->world=w;memcpy(e->opaque2b,&w,4);s->actor_next=r->actors;s->actor_previous=NULL;r->actors=e;
    if(s->actor_next)actor_previous_set(s->actor_next,e);}
static int migrate(BHRailWagonContext *c,BHEntity *e,const BHVec3 *point){BHWorldGeometry *w=bh_world_locate(c->first_world,c->world_count,private_data(e)->world,point,0);
    if(w&&w!=private_data(e)->world){BHPickupRoom *r=room(c,w);if(!r||(r->actors&&!member(c,r->actors))){failure(c,BH_WAGON_WORLD);return 0;}
        if(!unlink_room(c,e))return 0;prepend(c,e,w);++c->migrations;}return 1;}
static void difference(const BHVec3 *a,const BHVec3 *b,BHVec3 *d){bh_round_float(&d->x,(double)a->x-b->x);bh_round_float(&d->y,(double)a->y-b->y);bh_round_float(&d->z,(double)a->z-b->z);}
static void direction(BHRailWagonState *s,BHVec3 *d){difference(s->direction?&s->segment->end:&s->segment->start,s->direction?&s->segment->start:&s->segment->end,d);}
static void interpolate(BHRailSegment *s,int forward,double ratio,BHVec3 *out,BHVec3 *delta){const BHVec3 *a=forward?&s->start:&s->end,*b=forward?&s->end:&s->start;float dx,dy,dz;
    bh_round_float(&dx,(double)b->x-a->x);bh_round_float(&dy,(double)b->y-a->y);bh_round_float(&dz,(double)b->z-a->z);
    bh_round_float(&out->x,((double)b->x-a->x)*ratio+a->x);bh_round_float(&out->y,((double)b->y-a->y)*ratio+a->y);bh_round_float(&out->z,((double)b->z-a->z)*ratio+a->z);
    if(delta){bh_round_float(&delta->x,(double)dx/s->length);bh_round_float(&delta->y,(double)dy/s->length);bh_round_float(&delta->z,(double)dz/s->length);}}
static void draw_point(BHRailWagonState *s,BHVec3 *point,BHVec3 *delta){const BHVec3 *a=s->direction?&s->segment->start:&s->segment->end;double ratio=(double)s->elapsed/s->duration;
    direction(s,delta);bh_round_float(&point->x,(double)delta->x*ratio+a->x);bh_round_float(&point->y,(double)delta->y*ratio+a->y);bh_round_float(&point->z,(double)delta->z*ratio+a->z);}
static void pose_matrix(BHEntity *e,BHMat34 *m){BHRailWagonPose *p=bh_rail_wagon_pose(e);
    m->m[0][0]=p->right.x;m->m[0][1]=p->up.x;m->m[0][2]=p->forward.x;m->m[0][3]=e->position.x;
    m->m[1][0]=p->right.y;m->m[1][1]=p->up.y;m->m[1][2]=p->forward.y;m->m[1][3]=e->position.y;
    m->m[2][0]=p->right.z;m->m[2][1]=p->up.z;m->m[2][2]=p->forward.z;m->m[2][3]=e->position.z;}
static void align(BHEntity *e,const BHVec3 *delta){BHMat34 m;BHVec3 v,unit;BHRailWagonPose *p=bh_rail_wagon_pose(e);double angle;float negative;
    bh_vec3_normalize_to(delta,&p->forward);bh_vec3_cross(&p->up,&p->forward,&p->right);p->right.y=0;bh_vec3_normalize(&p->right);bh_vec3_cross(&p->forward,&p->right,&p->up);
    pose_matrix(e,&m);unit.x=0;unit.y=0;unit.z=1;bh_mat34_transform_direction(&m,&unit,&v);angle=atan2(v.x,v.z);bh_round_float(&p->angles.x,angle);bh_round_float(&negative,-angle);bh_mat34_rotate_world_y(negative,&m);
    bh_mat34_transform_direction(&m,&unit,&v);angle=atan2(-(double)v.y,v.z);bh_round_float(&p->angles.y,angle);bh_round_float(&negative,-angle);bh_mat34_rotate_world_x(negative,&m);
    unit.x=1;unit.z=0;bh_mat34_transform_direction(&m,&unit,&v);bh_round_float(&p->angles.z,atan2(v.y,v.x));}
static void cooldown(BHRailWagonContext *c,BHRailWagonState *s){if(s->sound_cooldown){s->sound_cooldown=(long)((unsigned long)s->sound_cooldown-1);
    if(!s->sound_cooldown&&s->sound_instance!=BH_SOUND_INVALID){bh_sound_stop(c->sound,s->sound_instance);s->sound_instance=BH_SOUND_INVALID;}}}
static double wrapped(double difference){float stored;bh_round_float(&stored,difference);
    if(_isnan(difference)||difference>=PI)return fmod((double)stored+PI,2*PI)-PI;
    if(stored< -PI)return fmod((double)stored-PI,2*PI)+PI;return stored;}
static int rider_update(BHRailWagonContext *c,BHEntity *e){BHRailWagonState *s=state(e);BHRailWagonPose *p=bh_rail_wagon_pose(e),*r;
    BHEntity *rider=s->rider;BHRailSegment *rail=s->segment;BHVec3 velocity,d;double distance,ratio,dx,dy,dz,px,py,pz,complement;float inv,yaw,pitch,roll,stored_y,stored_z;unsigned long clear=0;int crossed=0;
    if(!rider)return 1;if(!live(c,rider,private_data(e)->rider_generation)){failure(c,BH_WAGON_LIFETIME);return 0;}r=bh_rail_wagon_pose(rider);
    direction(s,&d);bh_round_float(&inv,1.0/s->duration);bh_round_float(&velocity.x,(double)d.x*inv);bh_round_float(&velocity.y,(double)d.y*inv);bh_round_float(&velocity.z,(double)d.z*inv);cooldown(c,s);
    distance=(double)s->elapsed*s->speed-(-5.0);
    if(distance>rail->length){distance-=rail->length;rail=walk(c,rail,s->direction);if(!rail)return 0;crossed=1;}
    ratio=distance/rail->length;
    {const BHVec3 *a=s->direction?&rail->end:&rail->start,*b=s->direction?&rail->start:&rail->end;float f;
     bh_round_float(&f,ratio);complement=1.0-(crossed?ratio:(double)f);px=(double)a->x*f+(double)b->x*complement;py=(double)a->y*f+(double)b->y*complement;pz=(double)a->z*f+(double)b->z*complement;}
    bh_round_float(&stored_y,py);bh_round_float(&stored_z,pz);
    bh_round_float(&rider->position.x,px-(double)p->up.x*5.0);bh_round_float(&rider->position.y,(double)stored_y-(double)p->up.y*5.0);bh_round_float(&rider->position.z,(double)stored_z-(double)p->up.z*5.0);
    rider->origin=velocity;r->right=p->right;r->up=p->up;r->forward=p->forward;
    dx=wrapped((double)p->angles.x-r->angles.x);dy=wrapped((double)p->angles.y-r->angles.y);dz=wrapped((double)p->angles.z-r->angles.z);
    bh_round_float(&yaw,dx);bh_round_float(&pitch,dy);bh_round_float(&roll,dz);
    if(_isnan(yaw)||yaw< -3*PI/4||yaw>3*PI/4)r->angles=p->angles;
    else{bh_round_float(&r->angles.x,(double)r->angles.x-yaw*(-0.2));bh_round_float(&r->angles.y,(double)r->angles.y-pitch*(-0.2));bh_round_float(&r->angles.z,(double)r->angles.z+roll);}
    memcpy(rider->payload+(0x23c-0xe4),&clear,4);memcpy(rider->opaque1,&c->pool->origin,12);
    memcpy(rider->opaque1+12,&c->pool->origin,12);memcpy(rider->opaque1+24,&c->pool->origin,12);return 1;}
int __cdecl bh_rail_wagon_set_rider(BHEntity *e,BHEntity *rider,BHRailSegment *checkpoint){BHRailWagonContext *c=owner(e);
    if(!c||(rider&&!member(c,rider))||(checkpoint&&!segment(c,checkpoint)))return 0;
    state(e)->rider=rider;state(e)->checkpoint=checkpoint;private_data(e)->rider_generation=rider?rider->generation:0;return 1;}
void __cdecl bh_rail_wagon_draw(BHEntity *e){BHRailWagonContext *c=owner(e);BHRailWagonState *s;BHVec3 point,delta,velocity;float pitch;double inv;BHRailWagonPose *p;
    if(!c)return;s=state(e);if(!ready(c,s))return;++c->draws;p=bh_rail_wagon_pose(e);
    draw_point(s,&point,&delta);
    if(bh_draw_submit_resource(c->queue,0,c->model,&point,&p->angles,1)<0)failure(c,BH_WAGON_DRAW);
    inv=1.0/s->duration;bh_round_float(&velocity.x,(double)delta.x*inv);bh_round_float(&velocity.y,(double)delta.y*inv);bh_round_float(&velocity.z,(double)delta.z*inv);
    bh_round_float(&pitch,(double)s->speed*0.02);s->sound_cooldown=2;
    if(s->sound_instance==BH_SOUND_INVALID)s->sound_instance=bh_sound_request(c->sound,s->sound_resource,1,pitch,&point,&velocity);
    else if(bh_sound_update(c->sound,s->sound_instance,1,pitch,&point,&velocity)<0)failure(c,BH_WAGON_SOUND);}
void __cdecl bh_rail_wagon_tick(BHEntity *e){BHRailWagonContext *c=owner(e);BHRailWagonState *s;BHVec3 delta;double elapsed,speed;unsigned long steps=0;
    if(!c)return;s=state(e);if(!ready(c,s))return;++c->steps;elapsed=(double)s->elapsed-(-1.0/30.0);bh_round_float(&s->elapsed,elapsed);
    if(!_isnan(elapsed)&&elapsed>=s->duration)do{
        cooldown(c,s);bh_round_float(&s->elapsed,(double)s->elapsed-s->duration);s->segment=walk(c,s->segment,s->direction);if(!s->segment)return;
        direction(s,&delta);e->position=s->direction?s->segment->start:s->segment->end;
        speed=(double)s->speed-(double)s->segment->rise*(s->direction?-0.8:0.8);bh_round_float(&s->speed,speed);if(_isnan(speed)||speed<30)s->speed=30;
        bh_round_float(&s->duration,(double)s->segment->length/s->speed);
        if(s->segment==s->checkpoint){if(s->rider)s->rider=NULL;else s->checkpoint=NULL;}
        if(!migrate(c,e,&e->position))return;align(e,&delta);++c->transitions;
        if(++steps>20000){failure(c,BH_WAGON_TOPOLOGY);return;}
    }while(s->elapsed>=s->duration);
    rider_update(c,e);}
void __cdecl bh_rail_wagon_follow(BHEntity *e){BHRailWagonContext *c=owner(e);BHRailWagonState *s,*leader;BHEntity *parent;BHRailSegment *rail;BHVec3 delta;double elapsed,distance;unsigned long n=0;
    if(!c)return;s=state(e);if(!ready(c,s))return;++c->follows;parent=s->leader;
    if(!wagon(c,parent)||!live(c,parent,private_data(e)->leader_generation)){failure(c,BH_WAGON_LIFETIME);return;}leader=state(parent);if(!ready(c,leader))return;
    elapsed=(double)s->elapsed-(-1.0/30.0);bh_round_float(&s->elapsed,elapsed);
    if(!_isnan(elapsed)&&elapsed<s->duration&&s->speed==leader->speed)return;
    rail=leader->segment;distance=((double)leader->elapsed*rail->length)/leader->duration-s->spacing;
    while(_isnan(distance)||distance<0){rail=walk(c,rail,!s->direction);if(!rail)return;distance+=rail->length;if(++n>20000){failure(c,BH_WAGON_TOPOLOGY);return;}}
    s->speed=leader->speed;bh_round_float(&s->duration,(double)rail->length/s->speed);bh_round_float(&s->elapsed,distance/s->speed);
    if(s->segment!=rail){if(!migrate(c,e,&rail->start))return;s->segment=rail;cooldown(c,s);direction(s,&delta);align(e,&delta);}}
void __cdecl bh_rail_wagon_query(BHEntity *e,void *packet){BHRailWagonContext *c=owner(e);BHRailWagonState *s,*other;long event;BHEntity *target;BHRailSegment *rail;double distance;unsigned long i;BHVec3 point;float ratio;
    if(!c||!packet)return;s=state(e);++c->queries;memcpy(&event,packet,4);
    if(event==4||event==20){if(!unlink_room(c,e))return;bh_sound_stop(c->sound,s->sound_instance);++c->removed;bh_entity_release(c->pool,e);return;}
    if(!ready(c,s))return;
    if(event==3){bh_round_float(&ratio,(double)s->elapsed/s->duration);interpolate(s->segment,s->direction,ratio,&point,NULL);
        bh_round_float(&point.x,(double)point.x-(double)bh_rail_wagon_pose(e)->up.x*5.0);bh_round_float(&point.y,(double)point.y-(double)bh_rail_wagon_pose(e)->up.y*5.0);bh_round_float(&point.z,(double)point.z-(double)bh_rail_wagon_pose(e)->up.z*5.0);memcpy((unsigned char*)packet+4,&point,12);return;}
    if(event!=19)return;memcpy(&target,(unsigned char*)packet+4,4);if(!wagon(c,target)){failure(c,BH_WAGON_LIFETIME);return;}other=state(target);if(!ready(c,other)||other->direction!=s->direction)return;
    rail=s->segment;distance=((double)other->elapsed*other->segment->length)/other->duration-((double)s->elapsed*rail->length)/s->duration;
    for(i=0;i<10;++i){rail=walk(c,rail,!s->direction);if(!rail)return;distance-=rail->length;}
    for(i=0;rail!=other->segment;++i){distance+=rail->length;rail=walk(c,rail,s->direction);if(!rail)return;if(i>=20)return;}
    if(_isnan(distance)||distance<0){distance=-distance;if(s->leader!=e&&(_isnan(distance)||distance<s->spacing))return;
        bh_round_float(&s->spacing,distance);s->leader=target;private_data(e)->leader_generation=target->generation;e->step=bh_rail_wagon_follow;}
    else{if(other->leader!=target&&distance<other->spacing)return;bh_round_float(&other->spacing,distance);other->leader=e;private_data(target)->leader_generation=e->generation;target->step=bh_rail_wagon_follow;}++c->couples;}
BHEntity *__cdecl bh_rail_wagon_spawn(BHRailWagonContext *c,const BHVec3 *position,const BHVec3 *angles){
    BHWorldGeometry *w;BHEntity *e;BHRailWagonState *s;BHRailWagonPose *p;BHMat34 m;BHVec3 delta,offset,unit;long i,best=-1;float chosen=0,closest=9e37f,len,stored_projection;double dx,dy,projection,ratio,distance;unsigned long generation;struct {long event;BHEntity *entity;} packet;
    if(!c||!c->rails||!c->rails->segments||c->rails->count<1||c->rails->count>200||!c->pool||!c->first_world||!c->world_count||!c->rooms||!c->room_count||!c->queue||!c->model||!c->sound||!position||!angles||!bh_math_environment_supported()){failure(c,BH_WAGON_ARGUMENT);return NULL;}
    w=bh_world_locate(c->first_world,c->world_count,NULL,position,0);if(!w)return NULL;
    if(!room(c,w)||(room(c,w)->actors&&!member(c,room(c,w)->actors))){failure(c,BH_WAGON_WORLD);return NULL;}
    for(i=0;i<c->rails->count;++i){BHRailSegment *r=c->rails->segments+i;dx=(double)r->end.x-r->start.x;dy=(double)r->end.y-r->start.y;difference(&r->end,&r->start,&delta);difference(position,&r->start,&offset);
        bh_round_float(&len,(dx*delta.x+dy*delta.y)+(double)delta.z*delta.z);if(len<=0||!_finite(len))continue;
        projection=((double)delta.x*offset.x+(double)delta.y*offset.y+(double)delta.z*offset.z)/len;
        bh_round_float(&stored_projection,projection);if(projection<0||stored_projection>1||!_finite(projection))continue;
        ratio=sqrt(((double)offset.x*offset.x+(double)offset.y*offset.y)+(double)offset.z*offset.z)/sqrt(len);distance=sqrt(ratio*ratio-(double)stored_projection*stored_projection);
        if(distance<closest){bh_round_float(&closest,distance);chosen=stored_projection;best=i;}}
    if(best<0)return NULL;e=bh_entity_acquire(c->pool);if(!e){failure(c,BH_WAGON_POOL);return NULL;}s=state(e);p=bh_rail_wagon_pose(e);private_data(e)->context=c;private_data(e)->leader_generation=e->generation;private_data(e)->rider_generation=0;
    e->position=*position;p->angles=*angles;e->flags|=0x100;e->step=bh_rail_wagon_tick;s->elapsed=chosen;s->duration=1;s->speed=30;s->draw=bh_rail_wagon_draw;
    s->sound_instance=BH_SOUND_INVALID;s->sound_resource=c->sound_resource;s->kind=9;s->subtype=0;s->sound_cooldown=0;s->secondary_instance=BH_SOUND_INVALID;s->leader=e;s->rider=NULL;s->checkpoint=NULL;prepend(c,e,w);
    memset(&m,0,sizeof(m));m.m[0][0]=m.m[1][1]=m.m[2][2]=1;bh_mat34_rotate_y(angles->x,&m);bh_mat34_rotate_x(angles->y,&m);bh_mat34_rotate_z(angles->z,&m);
    unit.x=1;unit.y=unit.z=0;bh_mat34_transform_direction(&m,&unit,&p->right);unit.x=0;unit.y=1;bh_mat34_transform_direction(&m,&unit,&p->up);unit.y=0;unit.z=1;bh_mat34_transform_direction(&m,&unit,&p->forward);
    s->segment=c->rails->segments+best;
    projection=(((double)s->segment->end.y-s->segment->start.y)*p->forward.y+((double)s->segment->end.z-s->segment->start.z)*p->forward.z)+((double)s->segment->end.x-s->segment->start.x)*p->forward.x;
    s->direction=!_isnan(projection)&&projection>=0;if(!s->direction)bh_round_float(&s->elapsed,1.0-s->elapsed);
    generation=e->generation;packet.event=19;packet.entity=e;bh_entities_query(c->pool,0,0,NULL,0,&packet);
    if(!live(c,e,generation)){failure(c,BH_WAGON_LIFETIME);return NULL;}
    bh_entity_set_query(c->pool,e,bh_rail_wagon_query);++c->spawned;return e;
}
