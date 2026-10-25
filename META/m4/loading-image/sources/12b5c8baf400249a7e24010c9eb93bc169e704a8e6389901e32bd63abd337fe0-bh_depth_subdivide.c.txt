#include "bh_depth_subdivide.h"
#include "bh_round.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
typedef char depth_layout_z[offsetof(BHClipVertex,point)+offsetof(BHVec3,z)==0x14?1:-1];
typedef char depth_layout_u[offsetof(BHClipVertex,u)==0x18?1:-1];
typedef char depth_layout_scalar[offsetof(BHClipVertex,unknown28)==0x28?1:-1];
typedef char depth_layout_links[offsetof(BHClipVertex,next)==0x2c && offsetof(BHClipVertex,previous)==0x30?1:-1];
static int fail(BHDepthSplitContext *c,BHDepthSplitError e){if(c)c->error=e;return 0;}
static float scalar(const BHClipVertex *v){float value;memcpy(&value,&v->unknown28,4);return value;}
static int valid_ring(BHDepthSplitContext *c,BHClipVertex *ring,int attribute) {
    BHClipVertex *p=ring;unsigned long n=0;
    if(!p)return fail(c,BH_DEPTH_SPLIT_ARGUMENT);
    do {
        if(!p || !p->next || !p->previous || p->next->previous!=p || p->previous->next!=p || ++n>1024)
            return fail(c,BH_DEPTH_SPLIT_TOPOLOGY);
        if(!_finite(p->point.z) || p->point.z<=0 || !_finite(p->u) || !_finite(p->v) ||
           !_finite(p->sx) || !_finite(p->sy) || (attribute && !_finite(scalar(p))))return fail(c,BH_DEPTH_SPLIT_NUMERICAL);
        p=p->next;
    }while(p!=ring);
    return n>=3 ? 1:fail(c,BH_DEPTH_SPLIT_TOPOLOGY);
}
int __cdecl bh_depth_split_init(BHDepthSplitContext *c,float left,float right,float top,float bottom,BHDepthLeaf leaf,void *user) {
    if(!c || !leaf || !_finite(left) || !_finite(right) || !_finite(top) || !_finite(bottom) || left>right || top>bottom)return 0;
    memset(c,0,sizeof(*c));c->left=left;c->right=right;c->top=top;c->bottom=bottom;c->leaf=leaf;c->user=user;return 1;
}
/* a is below the cut, b is at/above it. The original plain owner retains
   delta for UV but stores delta*mid for screen division. The scalar owner
   stores delta before BOTH divisions and retains that stored delta*mid. */
static int intersection(BHDepthSplitContext *c,BHClipVertex *out,const BHClipVertex *a,
    const BHClipVertex *b,float mid,int attribute) {
    double wa=(double)b->point.z-mid,wb=(double)mid-a->point.z,delta=(double)b->point.z-a->point.z;
    double divisor,x,y;float stored,value;
    memset(out,0,sizeof(*out));out->point.z=mid;
    if(attribute){bh_round_float(&stored,delta);delta=stored;}
    bh_round_float(&out->u,((double)a->u*wa+(double)b->u*wb)/delta);
    bh_round_float(&out->v,((double)a->v*wa+(double)b->v*wb)/delta);
    if(attribute){bh_round_float(&value,((double)scalar(a)*wa+(double)scalar(b)*wb)/delta);memcpy(&out->unknown28,&value,4);}
    divisor=delta*mid;
    if(!attribute){bh_round_float(&stored,divisor);divisor=stored;}
    if(!_finite(divisor) || divisor==0)return fail(c,BH_DEPTH_SPLIT_NUMERICAL);
    x=(((double)a->sx*wa)*a->point.z+((double)b->sx*wb)*b->point.z)/divisor;
    y=(((double)a->sy*wa)*a->point.z+((double)b->sy*wb)*b->point.z)/divisor;
    bh_round_float(&out->sx,x);bh_round_float(&out->sy,y);
    /* Scalar owner compares retained x to left after FST; all other bounds
       use the stored binary32 value, as do both plain-owner x comparisons. */
    if((attribute?x:(double)out->sx)<c->left)out->sx=c->left;
    else if(out->sx>c->right)out->sx=c->right;
    if(out->sy<c->top)out->sy=c->top;else if(out->sy>c->bottom)out->sy=c->bottom;
    if(!_finite(out->u) || !_finite(out->v) || !_finite(out->sx) || !_finite(out->sy) ||
        (attribute && !_finite(scalar(out))))return fail(c,BH_DEPTH_SPLIT_NUMERICAL);
    return 1;
}
static int split(BHDepthSplitContext *c,BHClipVertex *ring,int attribute,unsigned long depth) {
    BHClipVertex *p,*minimum=ring,*maximum=ring,*a,*b,*d,*e;BHClipVertex low,high;double sum;float mid;int ok;
    if(depth>256)return fail(c,BH_DEPTH_SPLIT_CAPACITY);if(depth>c->deepest)c->deepest=depth;
    p=ring->next;while(p!=ring){if(p->point.z<minimum->point.z)minimum=p;if(maximum->point.z<p->point.z)maximum=p;p=p->next;}
    sum=(double)minimum->point.z+maximum->point.z;
    if(((double)maximum->point.z-minimum->point.z)*16.0<sum) {
        if(c->leaves==65536)return fail(c,BH_DEPTH_SPLIT_CAPACITY);
        if(!c->leaf(ring,c->user))return fail(c,BH_DEPTH_SPLIT_LEAF);++c->leaves;return 1;
    }
    bh_round_float(&mid,sum*.5);
    if(!_finite(mid) || mid<=minimum->point.z || mid>=maximum->point.z)return fail(c,BH_DEPTH_SPLIT_NUMERICAL);
    a=ring;while(mid<=a->point.z || a->next->point.z<mid)a=a->next;b=a->next;
    e=a;while(e->point.z<mid || mid<=e->next->point.z)e=e->next;d=e->next;
    if(!intersection(c,&low,a,b,mid,attribute) || !intersection(c,&high,d,e,mid,attribute))return 0;
    ++c->splits;
    a->next=&low;low.previous=a;low.next=&high;high.previous=&low;high.next=d;d->previous=&high;
    ok=split(c,&low,attribute,depth+1);
    if(ok){high.next=&low;b->previous=&low;low.previous=&high;e->next=&high;low.next=b;high.previous=e;ok=split(c,&low,attribute,depth+1);}
    a->next=b;b->previous=a;e->next=d;d->previous=e;
    return ok;
}
static int run(BHDepthSplitContext *c,BHClipVertex *ring,int attribute) {
    if(!c || !c->leaf || !_finite(c->left) || !_finite(c->right) || !_finite(c->top) || !_finite(c->bottom) ||
        c->left>c->right || c->top>c->bottom)return fail(c,BH_DEPTH_SPLIT_ARGUMENT);
    c->error=BH_DEPTH_SPLIT_OK;c->splits=c->leaves=c->deepest=0;
    if(!valid_ring(c,ring,attribute))return 0;return split(c,ring,attribute,0);
}
int __cdecl bh_depth_subdivide(BHDepthSplitContext *c,BHClipVertex *ring){return run(c,ring,0);}
int __cdecl bh_depth_subdivide_scalar(BHDepthSplitContext *c,BHClipVertex *ring){return run(c,ring,1);}
