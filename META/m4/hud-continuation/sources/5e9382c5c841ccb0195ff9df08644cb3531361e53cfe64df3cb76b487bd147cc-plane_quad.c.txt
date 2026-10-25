#include "../src/bh_plane_quad.h"
#include "../src/bh_fp.h"
#include "plane_quad_cases.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *s,int line){++checks;if(!ok){++failures;if(failures<20)printf("FAIL plane quad %d: %s\n",line,s);}}
static int close_to(double a,double b){return fabs(a-b)<4e-5*(1+fabs(b));}
typedef struct Fixture {
    BHSpriteQuadContext draw;BHMainCamera camera;BHClipContext clip;
    BHLightConfig directional;BHLightContext lights;BHLight light;BHTextureImage image;
    BHClipVertex observed[16];unsigned long count,calls,color;int reject;
    BHDrawCommand *mutate;
} Fixture;
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user) {
    Fixture *f=(Fixture *)user;BHClipVertex *p=ring;unsigned long n=0;
    CHECK(image==&f->image,"image identity retained");
    do{CHECK(p->next->previous==p && p->previous->next==p,"reciprocal clipped winding");if(n<16)f->observed[n]=*p;++n;p=p->next;}while(p!=ring && n<16);
    CHECK(p==ring && n>=3 && n<=16,"bounded convex result");f->count=n;f->color=color;++f->calls;
    if(f->mutate)f->mutate->next=0x12345678UL;
    return !f->reject;
}
static void setup(Fixture *f){memset(f,0,sizeof(*f));f->camera.projected.m[0][0]=f->camera.projected.m[1][1]=f->camera.projected.m[2][2]=1;
    bh_clip_init(&f->clip,1,100,-1,1,-1,1);bh_clip_set_camera(&f->clip,&f->camera.projected);
    f->draw.camera=&f->camera;f->draw.clip=&f->clip;f->draw.directional=&f->directional;f->draw.world_lights=&f->lights;
    f->draw.emit=capture;f->draw.emit_user=f;f->draw.base_color=0xff000000UL;
}
static int draw(Fixture *f,const BHVec3 *center,const BHVec3 *normal,const BHVec3 *up,float w,float h,float angle,unsigned char flags,float modulation){
    return bh_plane_quad_draw(&f->draw,&f->image,center,normal,up,w,h,angle,flags,modulation);
}
static void geometry(void){Fixture f;BHVec3 p={0,0,4},n={0,0,-1},up={0,1,0};unsigned long flags,i;
    const double x[4]={1,1,-1,-1},y[4]={1,-1,-1,1};
    for(flags=0;flags<16;++flags){setup(&f);CHECK(draw(&f,&p,&n,&up,2,2,0,(unsigned char)flags,1),"all UV/front flags draw");
        CHECK(f.count==4 && f.calls==1 && f.color==0xffffffffUL && f.draw.emitted==1,"one white quad");
        for(i=0;i<4;++i){double u=i==1 || i==2?1:0,v=i>=2?1:0,a;
            if(flags&1)u=1-u;if(flags&2)v=1-v;if(flags&4){a=u;u=v;v=a;}
            CHECK(close_to(f.observed[i].local.x,x[i]) && close_to(f.observed[i].local.y,y[i]) && f.observed[i].local.z==4,"independent world-plane corners");
            CHECK(close_to(f.observed[i].sx,x[i]/4) && close_to(f.observed[i].sy,y[i]/4) && f.observed[i].point.z==4,"real clipper projection");
            CHECK(close_to(f.observed[i].u,u) && close_to(f.observed[i].v,v),"independent UV flip/transposition table");
        }
    }
    setup(&f);n.z=1;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && !f.calls && !f.draw.emitted,"strict backface rejection");
    CHECK(draw(&f,&p,&n,&up,2,2,0,8,.5f) && f.calls==1 && f.color==0xff7f7f7fUL,"backface flag admits and gray conversion truncates");
    CHECK(f.observed[0].local.x==-1 && f.observed[1].local.x==1 && f.observed[1].local.y==1,"backface reverses ring starting at same corner");
    n.z=-1;CHECK(draw(&f,&p,&n,&up,2,4,1.5707963267948966f,0,1),"quarter-turn basis rotation");
    CHECK(close_to(f.observed[0].local.x,-2) && close_to(f.observed[0].local.y,1) && close_to(f.observed[1].local.x,2) && close_to(f.observed[1].local.y,1),"analytical rotated rectangle");
}
static void clipping(void){Fixture f;BHVec3 p={3.5f,0,4},n={0,0,-1},up={0,1,0};unsigned long before;
    setup(&f);CHECK(draw(&f,&p,&n,&up,4,2,0,0,1) && f.clip.used>0 && f.calls==1,"quad passes real side clipping");
    CHECK(f.observed[0].sx<=1,"side intersection bounded");before=f.calls;p.x=100;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.calls==before && !f.draw.emitted,"side cull emits nothing");
    p.x=0;p.z=.5f;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.calls==before,"near cull emits nothing");
    p.z=101;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.calls==before,"far cull emits nothing");
    p.z=4;f.reject=1;CHECK(!draw(&f,&p,&n,&up,2,2,0,0,1) && f.draw.error==BH_SPRITE_QUAD_EMIT && !f.draw.emitted,"real downstream error propagated");
    f.reject=0;up.z=1;up.y=1;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && close_to(f.observed[0].local.y,1),"Gram-Schmidt removes normal component");
    up.y=0;CHECK(draw(&f,&p,&n,&up,2,2,0,0,1),"parallel up retains original degenerate quad");
}
static void lighting(void){Fixture f;BHVec3 p={0,0,4},n={0,0,-1},up={0,1,0};
    setup(&f);f.directional.enabled=1;f.directional.direction.z=-1;
    f.directional.directional[0]=100.75f;f.directional.directional[1]=300;f.directional.directional[2]=-10;
    f.directional.ambient[0]=1.5f;f.directional.ambient[1]=4.5f;f.directional.ambient[2]=1.25f;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,.5f),"directional and ambient path");
    CHECK(f.color==0xff339800UL,"two integer conversions precede channel clamp (102*.5,304*.5,-8*.5)");
    f.directional.enabled=0;f.draw.point_enabled=1;f.lights.lights=&f.light;
    f.light.type=1;f.light.color=0x206080UL;f.light.intensity=1;f.light.effective_intensity=1;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,1),"world light owner called");
    CHECK(f.color==0xff206080UL,"actual ambient world-light contribution");
    f.directional.enabled=1;f.directional.directional[0]=f.directional.directional[1]=f.directional.directional[2]=0;
    f.directional.ambient[0]=f.directional.ambient[1]=f.directional.ambient[2]=0;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,.5f),"dynamic plus directional modulation path");
    CHECK(f.color==0xff081820UL,"dynamic scale occurs in real light owner and again in directional channel conversion");
    f.directional.enabled=0;f.light.type=2;f.light.direction.z=1;f.light.intensity=1;f.light.effective_intensity=1;f.light.color=0x204060;
    CHECK(draw(&f,&p,&n,&up,2,2,0,0,1) && f.color==0xff204060UL,"front face reverses normal for world directional light");
    n.z=1;CHECK(draw(&f,&p,&n,&up,2,2,0,8,1) && f.color==0xff204060UL,"admitted back face uses unflipped negated normal");
}
static BHTextureImage *__cdecl lookup(unsigned long h,void *user){return h==17?&((Fixture *)user)->image:NULL;}
static void queued(void) {
    Fixture f;BHDrawQueue q;BHQuadQueueContext submit;BHVec3 p={0,0,4},n={0,0,-1},up={0,1,0};unsigned long flags,next;
    memset(&q,0,sizeof(q));setup(&f);submit.draw=&f.draw;submit.lookup=lookup;submit.lookup_user=&f;
    CHECK(bh_draw_queue_init(&q,12,NULL),"real oriented queue dependency");
    for(flags=0;flags<16;++flags) {
        bh_draw_queue_begin(&q);setup(&f);
        CHECK(bh_draw_oriented(&q,3,17,&p,&up,&n,2,2,0,flags|0x100,.5f)==1 &&
              bh_plane_quad_execute(&submit,q.commands+9,&next) && next==5,"complete producer/consumer uses correct bucket next");
        CHECK(f.count==4 && f.color==0xff7f7f7fUL,"consumer carries missed15th modulation argument and truncates flags byte");
        CHECK(f.observed[0].u==((flags&4)?((flags&2)?1:0):((flags&1)?1:0)) &&
              f.observed[0].v==((flags&4)?((flags&1)?1:0):((flags&2)?1:0)),"queued full image UV map all flags");
    }
    bh_draw_queue_begin(&q);setup(&f);CHECK(bh_draw_oriented(&q,3,17,&p,&up,&n,2,2,0,0,1)==1,"post-call next fixture queued");
    f.mutate=q.commands+9;CHECK(bh_plane_quad_execute(&submit,q.commands+9,&next) && next==0x12345678UL,"next link loads after downstream draw callback");
    f.mutate=NULL;f.reject=1;q.commands[9].next=33;
    CHECK(!bh_plane_quad_execute(&submit,q.commands+9,&next) && next==33 && f.draw.error==BH_SPRITE_QUAD_EMIT,"emitter error still publishes next");
    q.commands[9].resource=18;next=0;
    CHECK(!bh_plane_quad_execute(&submit,q.commands+9,&next) && next==33,"missing image fails and publishes next");
    q.commands[9].kind=BH_DRAW_QUAD;
    CHECK(!bh_plane_quad_execute(&submit,q.commands+9,&next),"wrong command kind rejects");
    bh_draw_queue_dispose(&q);
}
static void golden(void) {
    Fixture f;BHVec3 p,n,up;float dimensions[2];unsigned long i,j,k,bits;
    memcpy(&n,bh_plane_case_normal,12);memcpy(&up,bh_plane_case_up,12);memcpy(dimensions,bh_plane_case_size,8);
    for(k=0;k<sizeof(bh_plane_geometry_cases)/sizeof(bh_plane_geometry_cases[0]);++k) {
        setup(&f);memcpy(&p,bh_plane_geometry_cases[k].center,12);f.camera.position=p;
        memset(&f.camera.projected,0,sizeof(f.camera.projected));
        f.camera.projected.m[0][0]=f.camera.projected.m[1][1]=1.0f/1073741824.0f;f.camera.projected.m[2][3]=4;
        CHECK(bh_clip_set_camera(&f.clip,&f.camera.projected),"static golden projection binding");
        CHECK(draw(&f,&p,&n,&up,dimensions[0],dimensions[1],0,0,1) && f.count==4,"cancellation-sensitive plane quad rendered without clip");
        for(i=0;i<4;++i)for(j=0;j<3;++j) {
            memcpy(&bits,((float *)&f.observed[i].local)+j,4);
            CHECK(bits==bh_plane_geometry_cases[k].expected[i][j],"independent00456d30 exact mixed corner spills");
        }
    }
}
int bh_plane_quad_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    geometry();clipping();lighting();queued();golden();CHECK(bh_math_environment_supported(),"PC53 remains established");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"plane-quad\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_PLANE_QUAD_STANDALONE
int main(void){unsigned long n;return bh_plane_quad_checkpoint(&n);}
#endif
