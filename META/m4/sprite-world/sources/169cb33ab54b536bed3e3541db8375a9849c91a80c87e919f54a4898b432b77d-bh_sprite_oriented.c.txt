#include "bh_sprite_oriented.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

typedef char path_size[sizeof(BHSpriteOrientedVertex)==0x54?1:-1];
typedef char path_prev[offsetof(BHSpriteOrientedVertex,previous)==0x3c?1:-1];
typedef char path_next[offsetof(BHSpriteOrientedVertex,next)==0x40?1:-1];
typedef char path_flags[offsetof(BHSpriteOrientedVertex,flags)==0x44?1:-1];
typedef char path_width[offsetof(BHSpriteOrientedVertex,width)==0x4c?1:-1];

static int fail(BHSpriteOrientedContext *c,BHSpriteOrientedError e) {c->error=e;return 0;}
static int in_pool(const BHSpriteOrientedContext *c,const BHSpriteOrientedVertex *p) {
    return p>=c->path && p<c->path+c->used;
}
static float interpolate(float a,float b,float za,float zb,float near_z) {
    float out;
    bh_round_float(&out,(((double)near_z-zb)*a+((double)za-near_z)*b)/((double)za-zb));
    return out;
}
static BHSpriteOrientedVertex *intersection(BHSpriteOrientedContext *c,
    BHSpriteOrientedVertex *a,BHSpriteOrientedVertex *b) {
    BHSpriteOrientedVertex *v;
    float az=a->point.z,bz=b->point.z;
    if(c->used>=c->capacity || az==bz)return NULL;
    v=c->path+c->used++;
    v->point.x=interpolate(a->point.x,b->point.x,az,bz,c->near_z);
    v->point.y=interpolate(a->point.y,b->point.y,az,bz,c->near_z);
    v->point.z=c->near_z;
    v->texture_start=interpolate(a->texture_start,b->texture_start,az,bz,c->near_z);
    v->width=interpolate(a->width,b->width,az,bz,c->near_z);
    v->texture_end=interpolate(a->texture_end,b->texture_end,az,bz,c->near_z);
    v->flags=a->flags;
    v->previous=v->next=NULL;
    return v;
}
static void set_scalar(BHClipVertex *v,float value){memcpy(&v->unknown28,&value,4);}
static void set_vertex(BHClipVertex *v,float x,float y,float z,float u,float t,float scalar){
    memset(v,0,sizeof(*v));v->sx=x;v->sy=y;v->point.z=z;v->u=u;v->v=t;set_scalar(v,scalar);
}
static int polygon(BHSpriteOrientedContext *c,BHClipVertex *vertices,const unsigned char *indices,unsigned long count){
    unsigned long i;BHClipVertex *ring;
    for(i=0;i<count;++i){BHClipVertex *v=vertices+indices[i];v->next=vertices+indices[(i+1)%count];v->previous=vertices+indices[(i+count-1)%count];}
    ring=bh_attribute_clip_polygon(c->clip,vertices+indices[0],0);
    if(!ring){if(c->clip->error!=BH_CLIP_OK)return fail(c,BH_SPRITE_ORIENTED_CLIP);return 1;}
    if(!c->emit(ring,c->image,c->color,c->emit_user))return fail(c,BH_SPRITE_ORIENTED_EMIT);
    ++c->polygons_emitted;return 1;
}
static int emit_segment(BHSpriteOrientedContext *c,BHSpriteOrientedVertex *a,BHSpriteOrientedVertex *b){
    BHClipVertex v[5];float near_t,far_t,near_side,far_side,middle;
    unsigned long flags=a->flags;
    unsigned char quad[4]={0,1,3,2};
    unsigned char narrow_far_0[3]={4,0,1},narrow_far_1[3]={4,1,3},narrow_far_2[3]={0,4,2};
    unsigned char narrow_near_0[3]={4,2,0},narrow_near_1[3]={4,3,2},narrow_near_2[3]={4,1,3};
    if(!_finite(a->texture_end) || (double)a->texture_end< -2147483648.0 ||
       (double)a->texture_end>=2147483648.0)return fail(c,BH_SPRITE_ORIENTED_NUMERICAL);
    near_t=a->texture_end-(float)(int)a->texture_end;
    far_t=b->texture_end-(float)(int)a->texture_end;
    if(flags&1){near_t=1.0f-near_t;far_t=1.0f-far_t;}
    near_side=(flags&2)?1.0f:0.0f;far_side=1.0f-near_side;
    if(flags&4){
        set_vertex(v+0,a->corners[0].x,a->corners[0].y,a->point.z,near_side,near_t,a->texture_start);
        set_vertex(v+1,a->corners[1].x,a->corners[1].y,a->point.z,far_side,near_t,a->texture_start);
        set_vertex(v+2,a->corners[3].x,a->corners[3].y,b->point.z,near_side,far_t,b->texture_start);
        set_vertex(v+3,a->corners[2].x,a->corners[2].y,b->point.z,far_side,far_t,b->texture_start);
    }else{
        set_vertex(v+0,a->corners[0].x,a->corners[0].y,a->point.z,near_t,near_side,a->texture_start);
        set_vertex(v+1,a->corners[1].x,a->corners[1].y,a->point.z,near_t,far_side,a->texture_start);
        set_vertex(v+2,a->corners[3].x,a->corners[3].y,b->point.z,far_t,near_side,b->texture_start);
        set_vertex(v+3,a->corners[2].x,a->corners[2].y,b->point.z,far_t,far_side,b->texture_start);
    }
    if(a->width<=2.0f*b->width){
        if(b->width<=a->width)return polygon(c,v,quad,4);
        /* Original 00455f70 introduces a far-edge midpoint and emits three
           rings for a widening segment. */
        middle=0.5f;
        set_vertex(v+4,(v[2].sx+v[3].sx)*middle,(v[2].sy+v[3].sy)*middle,b->point.z,
            (v[2].u+v[3].u)*middle,(v[2].v+v[3].v)*middle,b->texture_start);
        return polygon(c,v,narrow_far_0,3) && polygon(c,v,narrow_far_1,3) && polygon(c,v,narrow_far_2,3);
    }
    /* A sharply narrowing segment uses a near-edge midpoint. */
    middle=0.5f;
    set_vertex(v+4,(v[0].sx+v[1].sx)*middle,(v[0].sy+v[1].sy)*middle,a->point.z,
        (v[0].u+v[1].u)*middle,(v[0].v+v[1].v)*middle,a->texture_start);
    return polygon(c,v,narrow_near_0,3) && polygon(c,v,narrow_near_1,3) && polygon(c,v,narrow_near_2,3);
}
int __cdecl bh_sprite_oriented_ribbon(BHSpriteOrientedContext *c,BHSpriteOrientedVertex *head){
    BHSpriteOrientedVertex *p,*q;unsigned long n=0;
    if(!c || !head || !c->path || !c->used || c->used>c->capacity || !c->clip || !c->emit ||
       !_finite(c->projection_scale))return c?fail(c,BH_SPRITE_ORIENTED_ARGUMENT):0;
    for(p=head;p;p=p->next){
        if(!in_pool(c,p) || ++n>c->used || !_finite(p->point.x) || !_finite(p->point.y) || !_finite(p->point.z) || p->point.z==0 || !_finite(p->width) || !_finite(p->texture_start) || !_finite(p->texture_end))return fail(c,BH_SPRITE_ORIENTED_TOPOLOGY);
        bh_round_float(&p->point.x,(double)p->point.x/p->point.z);
        bh_round_float(&p->point.y,(double)p->point.y/p->point.z);
        bh_round_float(&p->width,((double)c->projection_scale*0.5/p->point.z)*p->width);
    }
    for(p=head;p && p->next;p=p->next){
        BHVec3 dir;float dx,dy,aw,bw;
        q=p->next;dir.x=q->point.x-p->point.x;dir.y=q->point.y-p->point.y;dir.z=0;
        if(!bh_vec3_normalize(&dir))return fail(c,BH_SPRITE_ORIENTED_NUMERICAL);
        dx=-dir.y;dy=dir.x;aw=p->width;bw=q->width;
        p->corners[0].x=p->point.x-dx*aw;p->corners[0].y=p->point.y-dy*aw;p->corners[0].z=0;
        p->corners[1].x=p->point.x+dx*aw;p->corners[1].y=p->point.y+dy*aw;p->corners[1].z=0;
        p->corners[2].x=q->point.x+dx*bw;p->corners[2].y=q->point.y+dy*bw;p->corners[2].z=0;
        p->corners[3].x=q->point.x-dx*bw;p->corners[3].y=q->point.y-dy*bw;p->corners[3].z=0;
    }
    for(p=head;p && p->next && p->next->next;p=p->next){
        BHVec3 center,u,v,sum;double dot,den,scale,offset_z;float offset_x,offset_y;
        q=p->next;
        bh_round_float(&center.x,((double)p->corners[3].x+p->corners[2].x)*0.5);
        bh_round_float(&center.y,((double)p->corners[3].y+p->corners[2].y)*0.5);
        bh_round_float(&center.z,((double)p->corners[3].z+p->corners[2].z)*0.5);
        bh_round_float(&u.x,(double)p->corners[2].x-center.x);
        bh_round_float(&u.y,(double)p->corners[2].y-center.y);
        bh_round_float(&u.z,(double)p->corners[2].z-center.z);
        bh_round_float(&v.x,(double)q->corners[1].x-center.x);
        bh_round_float(&v.y,(double)q->corners[1].y-center.y);
        bh_round_float(&v.z,(double)q->corners[1].z-center.z);
        den=sqrt((double)u.x*u.x+(double)u.y*u.y)*sqrt((double)v.x*v.x+(double)v.y*v.y);
        if(den==0)return fail(c,BH_SPRITE_ORIENTED_NUMERICAL);
        dot=(((double)u.z*v.z+(double)u.y*v.y)+(double)u.x*v.x)/den;
        if(dot< -0.8)dot=(float)-0.8;
        scale=1.0/(dot+1.0);
        bh_round_float(&sum.x,(double)u.x+v.x);
        bh_round_float(&sum.y,(double)u.y+v.y);
        bh_round_float(&sum.z,(double)u.z+v.z);
        bh_round_float(&offset_x,(double)sum.x*scale);
        bh_round_float(&offset_y,(double)sum.y*scale);
        offset_z=(double)sum.z*scale;
        bh_round_float(&p->corners[2].x,(double)center.x+offset_x);
        bh_round_float(&p->corners[2].y,(double)center.y+offset_y);
        bh_round_float(&p->corners[2].z,(double)center.z+offset_z);
        q->corners[1]=p->corners[2];
        bh_round_float(&p->corners[3].x,(double)center.x-offset_x);
        bh_round_float(&p->corners[3].y,(double)center.y-offset_y);
        bh_round_float(&p->corners[3].z,(double)center.z-offset_z);
        q->corners[0]=p->corners[3];
    }
    for(p=head;p && p->next;p=p->next)if(!emit_segment(c,p,p->next))return 0;
    ++c->paths_emitted;return 1;
}
int __cdecl bh_sprite_oriented_draw(BHSpriteOrientedContext *c){
    BHSpriteOrientedVertex *p,*next,*first,*v;unsigned long original,steps=0,back_steps;
    if(!c || !c->path || !c->capacity || c->used>c->capacity || !c->clip || !c->emit ||
       !_finite(c->near_z) || c->near_z<=0 || !_finite(c->projection_scale))return c?fail(c,BH_SPRITE_ORIENTED_ARGUMENT):0;
    c->error=BH_SPRITE_ORIENTED_OK;c->paths_emitted=c->polygons_emitted=0;
    if(!c->used)return 1;
    original=c->used;p=c->path;
    while(p){
        if(!in_pool(c,p) || ++steps>original+c->capacity)return fail(c,BH_SPRITE_ORIENTED_TOPOLOGY);
        next=p->next;if(!next)break;
        if(!in_pool(c,next) || !_finite(p->point.z) || !_finite(next->point.z))return fail(c,BH_SPRITE_ORIENTED_TOPOLOGY);
        if(p->point.z<c->near_z && next->point.z>=c->near_z){
            v=intersection(c,p,next);if(!v)return fail(c,BH_SPRITE_ORIENTED_CAPACITY);
            v->next=next;next->previous=v;p=next;
        }else if(p->point.z>=c->near_z && next->point.z<c->near_z){
            v=intersection(c,p,next);if(!v)return fail(c,BH_SPRITE_ORIENTED_CAPACITY);
            v->previous=p;p->next=v;next->previous=NULL;
            first=p;back_steps=0;while(first->previous){if(!in_pool(c,first->previous) || ++back_steps>c->used)return fail(c,BH_SPRITE_ORIENTED_TOPOLOGY);first=first->previous;}
            if(!bh_sprite_oriented_ribbon(c,first))return 0;
            c->used=original;p=next;
        }else p=next;
    }
    if(p && p->previous && p->point.z>=c->near_z){
        first=p;back_steps=0;while(first->previous){if(!in_pool(c,first->previous) || ++back_steps>c->used)return fail(c,BH_SPRITE_ORIENTED_TOPOLOGY);first=first->previous;}
        if(!bh_sprite_oriented_ribbon(c,first))return 0;
    }
    c->used=0;return 1;
}
