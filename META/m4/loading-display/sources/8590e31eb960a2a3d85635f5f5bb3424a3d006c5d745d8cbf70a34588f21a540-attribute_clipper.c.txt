#include "../src/bh_attribute_clipper.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<18)printf("FAIL attribute clipper %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<=7e-5*(1+fabs(b));}
static float get_scalar(const BHClipVertex *v){float result;memcpy(&result,&v->unknown28,4);return result;}
static void set_scalar(BHClipVertex *v,float value){memcpy(&v->unknown28,&value,4);}
typedef struct AV {double x,y,z,u,v,sx,sy,s;unsigned long projected;} AV;
/* Independent array half-plane oracle uses edge parameter and reciprocal
   attributes, rather than the original extrema chain and weighted splices. */
static AV edge(AV a,AV b,double t,int depth) {
    AV r;double q;
    r.x=a.x+t*(b.x-a.x);r.y=a.y+t*(b.y-a.y);r.s=a.s+t*(b.s-a.s);
    r.sx=a.sx+t*(b.sx-a.sx);r.sy=a.sy+t*(b.sy-a.sy);r.projected=0;
    if(depth){r.z=a.z+t*(b.z-a.z);r.u=a.u+t*(b.u-a.u);r.v=a.v+t*(b.v-a.v);}
    else{q=1/a.z+t*(1/b.z-1/a.z);r.z=1/q;r.u=(a.u/a.z+t*(b.u/b.z-a.u/a.z))/q;r.v=(a.v/a.z+t*(b.v/b.z-a.v/a.z))/q;}
    return r;
}
static double coord(AV a,unsigned long axis){return axis==0?a.z:axis==1?a.sx:a.sy;}
static unsigned long oracle_plane(AV *points,unsigned long count,unsigned long axis,double bound,int low) {
    AV next[100],a,b,r;double va,vb,t;unsigned long i,n=0;int ia,ib;
    if(!count)return 0;a=points[count-1];va=coord(a,axis);ia=low?va>=bound:va<=bound;
    for(i=0;i<count;++i){b=points[i];vb=coord(b,axis);ib=low?vb>=bound:vb<=bound;
        if(ia!=ib){t=(bound-va)/(vb-va);r=edge(a,b,t,axis==0);if(axis==0)r.z=bound;else if(axis==1)r.sx=bound;else r.sy=bound;next[n++]=r;}
        if(ib)next[n++]=b;a=b;va=vb;ia=ib;
    }memcpy(points,next,n*sizeof(*points));return n;
}
static unsigned long oracle(AV *points,unsigned long count,const BHAttributeClipContext *c,int project_depth) {
    unsigned long i;
    if(project_depth){count=oracle_plane(points,count,0,c->near_z,1);count=oracle_plane(points,count,0,c->far_z,0);
        for(i=0;i<count;++i)if(!points[i].projected){points[i].sx=points[i].x/points[i].z;points[i].sy=points[i].y/points[i].z;points[i].projected=1;}}
    count=oracle_plane(points,count,1,c->left,1);count=oracle_plane(points,count,1,c->right,0);
    count=oracle_plane(points,count,2,c->top,1);return oracle_plane(points,count,2,c->bottom,0);
}
static void ring(BHClipVertex *vertices,unsigned long count,const AV *points,int reverse,AV *expected) {
    unsigned long i,j;memset(vertices,0x35,count*sizeof(*vertices));
    for(i=0;i<count;++i){j=reverse?count-1-i:i;vertices[i].point.x=(float)points[j].x;vertices[i].point.y=(float)points[j].y;vertices[i].point.z=(float)points[j].z;
        vertices[i].u=(float)points[j].u;vertices[i].v=(float)points[j].v;vertices[i].sx=(float)points[j].sx;vertices[i].sy=(float)points[j].sy;
        set_scalar(vertices+i,(float)points[j].s);vertices[i].projected=points[j].projected;
        vertices[i].next=vertices+(i+1)%count;vertices[i].previous=vertices+(i+count-1)%count;
        if(expected){expected[i].x=vertices[i].point.x;expected[i].y=vertices[i].point.y;expected[i].z=vertices[i].point.z;expected[i].u=vertices[i].u;expected[i].v=vertices[i].v;expected[i].sx=vertices[i].sx;expected[i].sy=vertices[i].sy;expected[i].s=get_scalar(vertices+i);expected[i].projected=vertices[i].projected;}
    }
}
static void compare(BHAttributeClipContext *c,BHClipVertex *head,const AV *expected,unsigned long count,int depth) {
    BHClipVertex *p=head;unsigned long n=0,i;int found;
    if(head)do{CHECK(p->next->previous==p && p->previous->next==p,"output reciprocal links");
        CHECK(c->left-1e-5<=p->sx && p->sx<=c->right+1e-5 && c->top-1e-5<=p->sy && p->sy<=c->bottom+1e-5,"output satisfies all four screen bounds");
        if(depth)CHECK(p->point.z>=c->near_z-1e-5 && p->point.z<=c->far_z+1e-5,"output satisfies both depth bounds");
        found=0;for(i=0;i<count;++i)if(close_value(p->sx,expected[i].sx) && close_value(p->sy,expected[i].sy) && close_value(p->point.z,expected[i].z) && close_value(p->u,expected[i].u) && close_value(p->v,expected[i].v) && close_value(get_scalar(p),expected[i].s))found=1;
        CHECK(found,"XYZ UV and scalar agree with independent edge-parametric oracle");
        ++n;p=p->next;
    }while(p!=head && n<100);
    CHECK(n==count && (!head || p==head),"independent oracle count and bounded ring closure");
}
static void varied(void) {
    BHAttributeClipContext c;BHClipVertex vertices[32],*head;AV points[32],expected[100];
    unsigned long n,i,count,ne,mode;double angle,radius,tx,ty;int reverse;
    for(n=0;n<120;++n){count=3+n%13;radius=2+(n%11)*.7;tx=(int)(n%9)-4;ty=(int)(n%7)-3;
        for(i=0;i<count;++i){angle=6.283185307179586*i/count+n*.01;
            points[i].x=tx+radius*cos(angle);points[i].y=ty+radius*.7*sin(angle);points[i].z=4+.2*points[i].x+.3*points[i].y;
            points[i].u=2*points[i].x+3*points[i].y+7;points[i].v=-points[i].x+4*points[i].y-3;points[i].s=.5+.03*points[i].x-.02*points[i].y;
            points[i].sx=points[i].x/points[i].z;points[i].sy=points[i].y/points[i].z;points[i].projected=0;
        }
        for(mode=0;mode<2;++mode)for(reverse=0;reverse<2;++reverse){bh_attribute_clip_init(&c,2,6,-.7f,.8f,-.6f,.9f);
            ring(vertices,count,points,reverse,expected);ne=oracle(expected,count,&c,(int)mode);
            head=bh_attribute_clip_polygon(&c,vertices+n%count,(int)mode);
            CHECK(c.error==BH_CLIP_OK && c.used<=12 && !(c.used&1),"six-plane owner bounded paired scratch count");compare(&c,head,expected,ne,(int)mode);
            for(i=0;i<count;++i)CHECK(!memcmp(&vertices[i].local.x,"5555",4) && !memcmp(&vertices[i].frame,"5555",4),"unused original local/frame payload untouched");
        }
    }
}
static void controls(void) {
    BHAttributeClipContext c,saved;BHClipVertex vertices[8],before[8],*head,*p;AV a[8],expected[100];unsigned long i,ne,n;float nan;unsigned long word=0x7fc00001UL;
    memset(a,0,sizeof(a));a[0].sx=-2;a[0].sy=-.5;a[0].z=2;a[0].u=a[0].s=0;
    a[1].sx=2;a[1].sy=-.5;a[1].z=6;a[1].u=a[1].s=100;
    a[2].sx=2;a[2].sy=.5;a[2].z=6;a[2].u=a[2].s=100;
    bh_attribute_clip_init(&c,1,10,0,10,-10,10);ring(vertices,3,a,0,expected);head=bh_attribute_clip_polygon(&c,vertices,0);
    CHECK(head && c.used==2,"isolated side cut");p=head;n=0;
    do{if(p->sx==0){CHECK(close_value(p->point.z,3) && close_value(p->u,25) && close_value(get_scalar(p),50),"perspective UV/depth differs from affine scalar28 at screen midpoint");++n;}p=p->next;}while(p!=head);
    CHECK(n==2,"both screen crossing intersections checked");
    memset(a,0,sizeof(a));for(i=0;i<4;++i){a[i].x=i==0 || i==3?-20:20;a[i].y=i<2?-20:20;a[i].z=5+.2*a[i].x+.2*a[i].y;a[i].u=a[i].x+3;a[i].v=a[i].y-2;a[i].s=.1*a[i].x+.2*a[i].y;}
    bh_attribute_clip_init(&c,1,9,-1,1,-1,1);memset(c.scratch,0x35,sizeof(c.scratch));ring(vertices,4,a,0,expected);
    ne=oracle(expected,4,&c,1);head=bh_attribute_clip_polygon(&c,vertices,1);CHECK(head && c.used==12,"one full fixture exercises near/far plus all four side cuts and twelve scratch records");compare(&c,head,expected,ne,1);
    for(i=0;i<c.used;++i)CHECK(!memcmp(&c.scratch[i].local.x,"5555",4) && !memcmp(&c.scratch[i].frame,"5555",4),"all scratch nodes retain untouched local and frame bytes");
    CHECK(!bh_attribute_clip_polygon(&c,head,1) && c.error==BH_CLIP_ARGUMENT,"scratch output lifetime cannot become next input");
    bh_attribute_clip_init(&c,2,6,-100,100,-100,100);ring(vertices,4,a,0,expected);ne=oracle(expected,4,&c,1);head=bh_attribute_clip_polygon(&c,vertices,-9);
    CHECK(c.error==BH_CLIP_OK && c.used==4,"any nonzero projection flag performs both depth cuts");compare(&c,head,expected,ne,1);
    for(i=0;i<c.used;++i)CHECK(c.scratch[i].projected==1,"depth-generated vertices projected and cache marked");
    for(i=0;i<3;++i){a[i].x=100;a[i].y=200;a[i].z=1;a[i].sx=(double)i*.1;a[i].sy=.1;a[i].projected=0;a[i].u=0;a[i].v=0;a[i].s=1;}
    ring(vertices,3,a,0,expected);memcpy(before,vertices,3*sizeof(*vertices));head=bh_attribute_clip_polygon(&c,vertices,0);
    CHECK(head==vertices && !c.used && !memcmp(vertices,before,3*sizeof(*vertices)),"zero flag skips near/far and projection despite depth outside range and clear cache");
    for(i=0;i<3;++i)a[i].projected=17,a[i].z=4;
    ring(vertices,3,a,0,expected);memcpy(before,vertices,3*sizeof(*vertices));head=bh_attribute_clip_polygon(&c,vertices,1);
    CHECK(head==vertices && !memcmp(vertices,before,3*sizeof(*vertices)),"nonzero per-vertex projection cache honors supplied coordinates");
    for(i=0;i<6;++i){ring(vertices,3,a,0,expected);for(n=0;n<3;++n){if(i==0)vertices[n].point.z=1;if(i==1)vertices[n].point.z=7;if(i==2)vertices[n].sx=-101;if(i==3)vertices[n].sx=101;if(i==4)vertices[n].sy=-101;if(i==5)vertices[n].sy=101;}
        CHECK(!bh_attribute_clip_polygon(&c,vertices,1) && c.error==BH_CLIP_OK && !c.used,"whole polygon rejected by each of six independent bounds");}
    for(i=0;i<3;++i)a[i].z=2;ring(vertices,3,a,0,expected);CHECK(bh_attribute_clip_polygon(&c,vertices,1)==vertices && !c.used,"near equality inclusive");
    for(i=0;i<3;++i)a[i].z=6;ring(vertices,3,a,0,expected);CHECK(bh_attribute_clip_polygon(&c,vertices,1)==vertices && !c.used,"far equality inclusive");
    ring(vertices,3,a,0,expected);vertices[1].previous=vertices+2;CHECK(!bh_attribute_clip_polygon(&c,vertices,1) && c.error==BH_CLIP_TOPOLOGY,"malformed ring guard");
    ring(vertices,3,a,0,expected);memcpy(&nan,&word,4);set_scalar(vertices,nan);CHECK(!bh_attribute_clip_polygon(&c,vertices,1) && c.error==BH_CLIP_NUMERICAL,"scalar28 is read as float and nonfinite guard applies");
    saved=c;CHECK(!bh_attribute_clip_init(&c,0,6,-1,1,-1,1) && !memcmp(&c,&saved,sizeof(c)),"invalid init retains context");
}
int bh_attribute_clipper_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    varied();controls();CHECK(bh_math_environment_supported(),"attribute clipper preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"attribute-clipper\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_ATTRIBUTE_CLIPPER_STANDALONE
int main(void){unsigned long count;return bh_attribute_clipper_checkpoint(&count);}
#endif
