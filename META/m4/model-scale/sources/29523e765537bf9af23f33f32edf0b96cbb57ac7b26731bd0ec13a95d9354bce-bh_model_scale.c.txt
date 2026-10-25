#include "bh_model_scale.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>

static int negative_or_unordered(double n) {return n<0.0 || _isnan(n);}
static int edge_index(const BHModel *m,long e) {return e>=0 && (unsigned long)e<m->edge_count;}
long __cdecl bh_model_reverse_edges(BHModel *m,long first,long count) {
    long left,right,after,remaining,i;BHBspEdge saved;
    if(count==0)return first;
    if(!m || count<0 || (unsigned long)count>m->edge_count || m->edge_count>65535 || !m->edges)return LONG_MIN;
    after=first;
    for(i=0;i<count;++i){if(!edge_index(m,after))return LONG_MIN;after=m->edges[after].next;}
    /* Stackless equivalent with the original innermost-first swap order; the source
       loader permits65535 slots, exceeding a safe recursive native stack. */
    for(remaining=count/2;remaining>0;--remaining){
        left=first;for(i=1;i<remaining;++i)left=m->edges[left].next;
        right=left;for(i=0;i<count-remaining*2+1;++i)right=m->edges[right].next;
        memcpy(&saved,m->edges+left,sizeof(saved));
        m->edges[left].vertex=m->edges[right].vertex;
        memcpy(&m->edges[left].u,&m->edges[right].u,8);
        m->edges[right].vertex=saved.vertex;
        memcpy(&m->edges[right].u,&saved.u,8);
    }
    return after;
}
static int bindings(BHModel *m,BHModelDrawCache *const *caches,unsigned long count,int reflect) {
    unsigned long i,j,n;long e;BHModelDrawCache *c;
    if(m->vertex_count>65535 || m->edge_count>65535 || m->face_count>65535 ||
       !m->node_count || m->node_count>65535 || !m->nodes ||
       (m->vertex_count && !m->vertices) || (reflect && m->face_count && !m->faces) ||
       (reflect && m->edge_count && !m->edges) || count>65535 || (count && !caches))return 0;
    for(i=0;i<count;++i){
        c=caches[i];if(!c)return 0;
        for(j=0;j<i;++j)if(caches[j]==c)return 0;
        if(c->model && c->model!=m)return 0;
        if((c->vertices || c->face_frames || c->tasks) &&
           (c->model!=m || !c->allocator.release))return 0;
    }
    if(reflect)for(i=0;i<m->face_count;++i){
        e=m->faces[i].first_edge;n=0;
        while(e>=0){if(!edge_index(m,e) || ++n>m->edge_count)return 0;e=m->edges[e].next;}
    }
    return 1;
}
static double magnitude(double value) {return negative_or_unordered(value)?-value:value;}
static double extent(float lo,float hi) {
    double a=magnitude(lo),b=magnitude(hi);return a<b?b:a;
}
int __cdecl bh_model_scale(BHModel *m,float sx,float sy,float sz,
                          BHModelDrawCache *const *caches,unsigned long count) {
    unsigned long i,j;int reflect;BHBspNode *n;float px,py,pz;double x,y,z,oldx,oldy,oldz,plane,det;
    long e,length;
    if(!m)return 0;
    if(m->frame)return 1;
    det=((double)sz*sy)*sx;reflect=negative_or_unordered(det);
    if(!bindings(m,caches,count,reflect))return 0;
    for(i=0;i<m->vertex_count;++i){
        bh_round_float(&m->vertices[i].x,(double)sx*m->vertices[i].x);
        bh_round_float(&m->vertices[i].y,(double)sy*m->vertices[i].y);
        bh_round_float(&m->vertices[i].z,(double)sz*m->vertices[i].z);
    }
    for(i=0;i<m->node_count;++i){
        n=m->nodes+i;oldx=n->normal[0];oldy=n->normal[1];oldz=n->normal[2];plane=-(double)n->plane;
        bh_round_float(&n->normal[0],oldx/sx);
        bh_round_float(&n->normal[1],oldy/sy);
        bh_round_float(&n->normal[2],oldz/sz);
        bh_round_float(&px,oldx*plane);bh_round_float(&py,oldy*plane);bh_round_float(&pz,oldz*plane);
        bh_vec3_normalize((BHVec3 *)n->normal);
        x=((double)px*sx)*n->normal[0];z=((double)pz*sz)*n->normal[2];y=((double)py*sy)*n->normal[1];
        bh_round_float(&n->plane,-((z+x)+y));
        bh_round_float(n->minimum,(double)sx*n->minimum[0]);
        bh_round_float(n->maximum,(double)sx*n->maximum[0]);
        bh_round_float(n->minimum+1,(double)sy*n->minimum[1]);
        bh_round_float(n->maximum+1,(double)sy*n->maximum[1]);
        bh_round_float(n->minimum+2,(double)sz*n->minimum[2]);
        bh_round_float(n->maximum+2,(double)sz*n->maximum[2]);
        for(j=0;j<3;++j)if(negative_or_unordered(j==0?sx:j==1?sy:sz)){
            float swap=n->minimum[j];n->minimum[j]=n->maximum[j];n->maximum[j]=swap;
        }
    }
    if(reflect)for(i=0;i<m->face_count;++i){
        length=0;e=m->faces[i].first_edge;
        while(e>=0){++length;e=m->edges[e].next;}
        bh_model_reverse_edges(m,m->faces[i].first_edge,length);
    }
    for(i=0;i<count;++i)bh_model_draw_cache_dispose(caches[i]);
    memset(m->runtime,0,sizeof(m->runtime));
    x=extent(m->nodes[0].minimum[0],m->nodes[0].maximum[0]);
    y=extent(m->nodes[0].minimum[1],m->nodes[0].maximum[1]);
    z=extent(m->nodes[0].minimum[2],m->nodes[0].maximum[2]);
    bh_round_float(&m->radius,sqrt((x*x+y*y)+z*z));
    return 1;
}
