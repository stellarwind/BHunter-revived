#include "../src/bh_screen_clipper.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL screen clipper %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<2e-5*(1+fabs(b));}
typedef struct XY {double x,y;} XY;
/* Independent array Sutherland-Hodgman oracle: no extreme-chain/ring splice. */
static unsigned long oracle(XY *points,unsigned long count,double left,double right,double top,double bottom) {
    XY next[80],a,b;double bound,va,vb,t;unsigned long plane,i,used;int ia,ib;
    for(plane=0;plane<4;++plane){used=0;bound=plane==0?left:plane==1?right:plane==2?top:bottom;
        if(!count)return 0;a=points[count-1];va=plane<2?a.x:a.y;ia=(plane==0 || plane==2)?va>=bound:va<=bound;
        for(i=0;i<count;++i){b=points[i];vb=plane<2?b.x:b.y;ib=(plane==0 || plane==2)?vb>=bound:vb<=bound;
            if(ia!=ib){t=(bound-va)/(vb-va);next[used].x=a.x+t*(b.x-a.x);next[used].y=a.y+t*(b.y-a.y);++used;}
            if(ib)next[used++]=b;a=b;va=vb;ia=ib;
        }count=used;memcpy(points,next,count*sizeof(*points));
    }return count;
}
static void ring(BHClipVertex *vertices,unsigned long count,const XY *points,int reverse) {
    unsigned long i,j;memset(vertices,0x35,count*sizeof(*vertices));
    for(i=0;i<count;++i){j=reverse?count-1-i:i;vertices[i].sx=(float)points[j].x;vertices[i].sy=(float)points[j].y;
        vertices[i].u=(float)(2*vertices[i].sx+3*vertices[i].sy+7);vertices[i].v=(float)(-vertices[i].sx+4*vertices[i].sy-3);
        vertices[i].point.z=(float)(20+i);vertices[i].projected=40+i;
        vertices[i].next=vertices+(i+1)%count;vertices[i].previous=vertices+(i+count-1)%count;
    }
}
static unsigned long inspect(BHScreenClipContext *c,BHClipVertex *head,float depth,XY *actual) {
    BHClipVertex *p=head;unsigned long n=0,i;int scratch;
    if(!head)return 0;
    do{CHECK(p->next->previous==p && p->previous->next==p,"output ring reciprocal links");
        CHECK(c->left-1e-5<=p->sx && p->sx<=c->right+1e-5 && c->top-1e-5<=p->sy && p->sy<=c->bottom+1e-5,"all four inclusive screen bounds");
        CHECK(close_value(p->u,2*p->sx+3*p->sy+7) && close_value(p->v,-p->sx+4*p->sy-3),"independent affine UV field preserved at intersections");
        scratch=0;for(i=0;i<c->used;++i)if(p==c->scratch+i)scratch=1;
        if(scratch)CHECK(p->point.z==depth && p->projected==0,"generated vertices receive uniform depth and cleared projected flag");
        actual[n].x=p->sx;actual[n].y=p->sy;++n;p=p->next;
    }while(p!=head && n<80);
    CHECK(p==head && n<80,"bounded closed output ring");return n;
}
static void compare(const XY *a,unsigned long na,const XY *b,unsigned long nb) {
    unsigned long i,j;int found;CHECK(na==nb,"independent oracle output vertex count");
    for(i=0;i<na;++i){found=0;for(j=0;j<nb;++j)if(close_value(a[i].x,b[j].x) && close_value(a[i].y,b[j].y))found=1;CHECK(found,"ring output intersection belongs to independent array oracle");}
}
static void varied(void) {
    BHScreenClipContext c;BHClipVertex vertices[32],*head;XY source[32],expected[80],actual[80];
    unsigned long n,i,count,ne,na;double angle,radius,tx,ty;int reverse;
    for(n=0;n<160;++n){count=3+n%13;radius=1+(n%11)*.7;tx=(int)(n%9)-4;ty=(int)(n%7)-3;
        for(i=0;i<count;++i){angle=6.283185307179586*i/count+n*.01;source[i].x=(float)(tx+radius*cos(angle));source[i].y=(float)(ty+radius*.7*sin(angle));}
        for(reverse=0;reverse<2;++reverse){CHECK(bh_screen_clip_init(&c,1,100,-2,3,-1,2),"source-only screen bounds adapter");
            ring(vertices,count,source,reverse);for(i=0;i<count;++i){expected[i].x=vertices[i].sx;expected[i].y=vertices[i].sy;}
            ne=oracle(expected,count,-2,3,-1,2);head=bh_screen_clip_polygon(&c,vertices+n%count,7.25f);
            CHECK(c.error==BH_CLIP_OK && c.used<=8 && !(c.used&1),"complete owner succeeds with bounded paired scratch allocation");
            na=inspect(&c,head,7.25f,actual);compare(actual,na,expected,ne);
            for(i=0;i<count;++i)CHECK(vertices[i].point.z==20+i && vertices[i].projected==40+i,"surviving and removed original depth/cache fields untouched");
        }
    }
}
static void boundaries_and_lifetime(void) {
    BHScreenClipContext c,saved;BHClipVertex v[4],before[4],*head,*p;XY quad[4]={{-4,-4},{4,-4},{4,4},{-4,4}},actual[80];unsigned long i,j,marker=0x35353535UL;float nan;
    bh_screen_clip_init(&c,1,10,-1,1,-1,1);ring(v,4,quad,0);memcpy(before,v,sizeof(v));
    CHECK(!bh_screen_clip_polygon(&c,v,.5f) && c.error==BH_CLIP_OK && !c.used && !memcmp(v,before,sizeof(v)),"uniform near depth rejects entire polygon before any interpolation");
    CHECK(!bh_screen_clip_polygon(&c,NULL,11) && c.error==BH_CLIP_OK && !c.used,"uniform far depth rejects before head dereference");
    for(i=0;i<2;++i){ring(v,4,quad,0);memset(c.scratch,0x35,sizeof(c.scratch));head=bh_screen_clip_polygon(&c,v,i?10.0f:1.0f);
        CHECK(head && c.used==8,"inclusive near/far equality with all four screen cuts allocates eight nodes");
        CHECK(inspect(&c,head,i?10.0f:1.0f,actual)==4,"large enclosing quad clips to viewport rectangle");
        for(j=0;j<c.used;++j)CHECK(c.scratch[j].unknown28==marker && !memcmp(&c.scratch[j].frame,&marker,4) && !memcmp(&c.scratch[j].local.x,&marker,4) && !memcmp(&c.scratch[j].point.x,&marker,4),"generated nodes preserve every untouched scratch payload group");
        p=head;CHECK(!bh_screen_clip_polygon(&c,p,5) && c.error==BH_CLIP_ARGUMENT,"borrowed scratch output cannot be reused as new input");
    }
    quad[0].x=quad[3].x=-1;quad[1].x=quad[2].x=1;quad[0].y=quad[1].y=-1;quad[2].y=quad[3].y=1;
    ring(v,4,quad,0);memcpy(before,v,sizeof(v));head=bh_screen_clip_polygon(&c,v,5);
    CHECK(head==v && !c.used && !memcmp(v,before,sizeof(v)),"equal-boundary ring preserves head and all input bytes");
    for(i=0;i<4;++i){ring(v,4,quad,0);for(j=0;j<4;++j){if(i==0)v[j].sx=-3;if(i==1)v[j].sx=3;if(i==2)v[j].sy=-3;if(i==3)v[j].sy=3;}
        CHECK(!bh_screen_clip_polygon(&c,v,5) && c.error==BH_CLIP_OK && !c.used,"entire polygon rejected by each screen plane");}
    ring(v,4,quad,0);v[1].previous=v+3;CHECK(!bh_screen_clip_polygon(&c,v,5) && c.error==BH_CLIP_TOPOLOGY,"malformed reciprocal ring rejected");
    ring(v,4,quad,0);marker=0x7fc00001UL;memcpy(&nan,&marker,4);v[0].u=nan;CHECK(!bh_screen_clip_polygon(&c,v,5) && c.error==BH_CLIP_NUMERICAL,"nonfinite interpolated attribute rejected");
    ring(v,4,quad,0);v[0].point.z=nan;CHECK(bh_screen_clip_polygon(&c,v,5)==v && c.error==BH_CLIP_OK,"unused per-vertex depth is not substituted for uniform depth");
    CHECK(!bh_screen_clip_polygon(&c,v,nan) && c.error==BH_CLIP_NUMERICAL,"nonfinite uniform depth guard");
    saved=c;CHECK(!bh_screen_clip_init(&c,0,10,-1,1,-1,1) && !memcmp(&c,&saved,sizeof(c)),"invalid context init preserves output");
    ring(v,1,quad,0);CHECK(bh_screen_clip_polygon(&c,v,5)==v && !c.used,"inside single-node degenerate ring retained");
}
int bh_screen_clipper_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    varied();boundaries_and_lifetime();CHECK(bh_math_environment_supported(),"screen clipper preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"screen-clipper\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_SCREEN_CLIPPER_STANDALONE
int main(void){unsigned long count;return bh_screen_clipper_checkpoint(&count);}
#endif
