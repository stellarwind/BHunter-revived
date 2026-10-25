#include "bh_rail_paths.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#undef isdigit
typedef char rail_segment_size[sizeof(BHRailSegment)==44&&offsetof(BHRailSegment,visited)==40?1:-1];
static void *__cdecl allocate(unsigned long n,void *u){return malloc(n);}
static void __cdecl release(void *p,void *u){free(p);}
static int fail(BHRailPaths *p){if(p)++p->failures;return 0;}
int __cdecl bh_rail_paths_init(BHRailPaths *p,const BHSceneAllocator *a){
    if(!p||(a&&(!a->allocate||!a->release)))return 0;
    memset(p,0,sizeof(*p));if(a)p->allocator=*a;
    else{p->allocator.allocate=allocate;p->allocator.release=release;}
    p->segments=(BHRailSegment*)p->allocator.allocate(200UL*sizeof(BHRailSegment),p->allocator.user);
    if(!p->segments)return fail(p);memset(p->segments,0,200UL*sizeof(BHRailSegment));return 1;
}
void __cdecl bh_rail_paths_dispose(BHRailPaths *p){
    if(!p)return;if(p->segments)p->allocator.release(p->segments,p->allocator.user);
    p->segments=NULL;p->count=p->chain_count=0;
}
static int valid(const BHRailPaths *p){return p&&p->segments&&p->count>=0&&p->count<=200&&p->chain_count>=0&&p->chain_count<=p->count;}
static double metric(const BHVec3 *a,const BHVec3 *b){
    double x=(double)a->x-b->x,y=(double)a->y-b->y,z=(double)a->z-b->z;float rx,rz;
    bh_round_float(&rx,x);bh_round_float(&rz,z);return (x*rx+y*y)+z*rz;
}
int __cdecl bh_rail_near(const BHVec3 *a,const BHVec3 *b){
    double d;if(!a||!b)return 0;d=metric(a,b);return _isnan(d)||d<0.25;
}
static int alias(const BHRailPaths *p,const void *v){
    unsigned long at=(unsigned long)v,base=(unsigned long)p->segments;
    return at>=base&&at<base+200UL*sizeof(BHRailSegment);
}
static void transform(BHVec3 *out,const BHVec3 *v,const BHVec3 *position,double sine,double cosine){
    bh_round_float(&out->x,(double)v->x*cosine+(double)v->z*sine);out->y=v->y;
    bh_round_float(&out->z,(double)v->z*cosine-(double)v->x*sine);
    bh_round_float(&out->x,(double)out->x+position->x);
    bh_round_float(&out->y,(double)out->y+position->y);
    bh_round_float(&out->z,(double)out->z+position->z);
}
int __cdecl bh_rail_add(BHRailPaths *p,const BHVec3 *start,const BHVec3 *end,const BHVec3 *position,const BHVec3 *rotation){
    BHRailSegment *s;double sine,cosine;
    if(!valid(p)||!start||!end||!position||!rotation||!bh_math_environment_supported())return fail(p);
    if(p->count==200){++p->dropped;return 1;}
    if(!_finite(rotation->x)||fabs(rotation->x)>1000000||alias(p,start)||alias(p,end)||alias(p,position)||alias(p,rotation))return fail(p);
    sine=sin((double)rotation->x);cosine=cos((double)rotation->x);s=p->segments+p->count++;
    transform(&s->start,start,position,sine,cosine);transform(&s->end,end,position,sine,cosine);
    s->next=s->previous=NULL;s->visited=0;++p->additions;return 1;
}
static int part(BHRailPaths *p,const BHVec3 *position,const BHVec3 *rotation,
    float x1,float y1,float z1,float x2,float y2,float z2){
    BHVec3 a,b;a.x=x1;a.y=y1;a.z=z1;b.x=x2;b.y=y2;b.z=z2;
    return bh_rail_add(p,&a,&b,position,rotation);
}
int __cdecl bh_rail_placement(BHRailPaths *p,const char *name,const BHVec3 *position,const BHVec3 *rotation){
    if(!p||!name||!position||!rotation)return fail(p);++p->placements;
    if(_strnicmp(name,"RC_",3)||!isdigit((int)(signed char)name[3]))return 1;
    switch(name[3]){
    case '0':return part(p,position,rotation,5,0,0,5,0,10);
    case '1':return part(p,position,rotation,5,0,0,5,0,20);
    case '2':return part(p,position,rotation,5,-10,0,5,0,20);
    case '3':return part(p,position,rotation,5,0,0,5,-10,10);
    case '4':return part(p,position,rotation,5,-20,10,5,0,20);
    case '5':return part(p,position,rotation,5,0,0,5,-20,0);
    case '6':return part(p,position,rotation,5,-20,10,5,0,20);
    case '7':
        if(!part(p,position,rotation,5,0,0,10,-6.66f,18))return 0;
        if(!part(p,position,rotation,10,-6.66f,18,22,-13.33f,30))return 0;
        return part(p,position,rotation,22,-13.33f,30,40,-20,35);
    case '8':
        if(!part(p,position,rotation,35,0,0,30,-6.66f,18))return 0;
        if(!part(p,position,rotation,30,-6.66f,18,18,-13.33f,30))return 0;
        return part(p,position,rotation,18,-13.33f,30,0,-20,35);
    case '9':
        if(!part(p,position,rotation,5,0,0,10,0,18))return 0;
        if(!part(p,position,rotation,10,0,18,22,0,30))return 0;
        return part(p,position,rotation,22,0,30,40,0,35);
    }return 1;
}
static int pointer(const BHRailPaths *p,const BHRailSegment *v){
    unsigned long at=(unsigned long)v,base=(unsigned long)p->segments;
    return !v||(at>=base&&at-base<(unsigned long)p->count*sizeof(*v)&&(at-base)%sizeof(*v)==0);
}
int __cdecl bh_rail_link(BHRailPaths *p){
    long i;BHRailSegment temporary,*head,*current,*candidate;BHVec3 point;int found;
    if(!valid(p)||!bh_math_environment_supported())return fail(p);
    for(i=0;i<p->count;++i)if(!pointer(p,p->segments[i].next)||!pointer(p,p->segments[i].previous))return fail(p);
    for(;;){
        for(i=0;i<p->count&&p->segments[i].visited;++i){}
        if(i==p->count)break;if(p->chain_count>=p->count)return fail(p);
        head=p->segments+p->chain_count;candidate=p->segments+i;
        temporary=*head;*head=*candidate;*candidate=temporary;
        if(candidate->next)candidate->next->previous=candidate;
        if(candidate->previous)candidate->previous->next=candidate;
        ++p->chain_count;head->visited=1;current=head;
        for(;;){found=0;
            for(i=0;i<p->count;++i){candidate=p->segments+i;if(candidate->visited)continue;
                if(bh_rail_near(&current->end,&candidate->start)&&!bh_rail_near(&current->start,&candidate->end)){
                    candidate->visited=1;found=1;break;
                }
                if(bh_rail_near(&current->end,&candidate->end)&&!bh_rail_near(&current->start,&candidate->start)){
                    candidate->visited=1;point=candidate->start;candidate->start=candidate->end;candidate->end=point;found=1;break;
                }
            }
            if(!found)break;candidate->previous=current;current->next=candidate;current=candidate;
        }
        if(bh_rail_near(&current->end,&head->start)){current->next=head;head->previous=current;}
    }
    for(i=0;i<p->count;++i){current=p->segments+i;
        bh_round_float(&current->length,sqrt(metric(&current->end,&current->start)));
        bh_round_float(&current->rise,(double)current->end.y-current->start.y);
    }return 1;
}
