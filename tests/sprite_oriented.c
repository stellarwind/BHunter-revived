#include "../src/bh_sprite_oriented.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x,message) do {++checks;if(!(x)){fprintf(stderr,"sprite oriented: %s\n",message);++failed;}} while(0)
typedef struct Capture {unsigned long calls,count[8];float x[8][8],y[8][8],z[8][8],u[8][8],v[8][8],scalar[8][8];BHTextureImage *image;unsigned long color;int reject;} Capture;
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *user){
    Capture *c=(Capture *)user;BHClipVertex *v=ring;unsigned long call=c->calls++,n=0;
    if(call>=8 || c->reject)return 0;c->image=image;c->color=color;
    do {if(n>=8)return 0;c->x[call][n]=v->sx;c->y[call][n]=v->sy;c->z[call][n]=v->point.z;c->u[call][n]=v->u;c->v[call][n]=v->v;memcpy(&c->scalar[call][n],&v->unknown28,4);++n;v=v->next;}while(v!=ring);
    c->count[call]=n;return 1;
}
static int close(float a,float b){return fabs((double)a-b)<0.0001;}
static int contains_point(const Capture *c,float x,float z){unsigned long i,j;for(i=0;i<c->calls;++i)for(j=0;j<c->count[i];++j)if(close(c->x[i][j],x)&&close(c->z[i][j],z))return 1;return 0;}
static void setup(BHSpriteOrientedContext *c,BHSpriteOrientedVertex *pool,BHAttributeClipContext *clip,BHTextureImage *image,Capture *capture_state){
    memset(c,0,sizeof(*c));memset(pool,0,8*sizeof(*pool));memset(capture_state,0,sizeof(*capture_state));
    bh_attribute_clip_init(clip,1,100,-10,10,-10,10);
    c->path=pool;c->capacity=8;c->near_z=1;c->projection_scale=2;c->clip=clip;c->image=image;c->color=0x12345678UL;c->emit=capture;c->emit_user=capture_state;
}
int bh_sprite_oriented_checkpoint(unsigned long *count){
    BHSpriteOrientedContext c;BHSpriteOrientedVertex pool[8];BHAttributeClipContext clip;BHTextureImage image;Capture got;BHMathEnvironment fp;unsigned long checks=0,failed=0;
    if(!bh_math_environment_enter(&fp))return 2;
    memset(&image,0,sizeof(image));setup(&c,pool,&clip,&image,&got);
    CHECK(bh_sprite_oriented_draw(&c) && !got.calls && c.used==0,"empty path is inert");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=pool[1].width=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_ribbon(&c,pool) && got.calls==1 && c.used==2,"direct ribbon owner emits without consuming source pool count");
    CHECK(close(pool[1].point.x,1) && close(pool[0].width,.5f),"direct ribbon owner stores projected path and width");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=pool[1].width=1;
    pool[0].texture_start=.2f;pool[1].texture_start=.8f;pool[0].texture_end=0;pool[1].texture_end=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && c.used==0 && c.paths_emitted==1 && c.polygons_emitted==1,"visible edge emits one path and polygon");
    CHECK(got.calls==1 && got.count[0]==4 && got.image==&image && got.color==c.color,"synchronous primitive payload");
    CHECK(close(got.x[0][0],0) && close(got.y[0][0],-.5f) && close(got.x[0][2],1) && close(got.z[0][2],2),"projected ribbon corners and depth");
    CHECK(close(got.scalar[0][0],.2f) && close(got.scalar[0][2],.8f) && close(got.u[0][2],1),"brightness and longitudinal UV survive polygon callback");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=.5f;pool[1].point.z=2;pool[1].point.x=2;pool[0].width=1;pool[1].width=1;
    pool[0].texture_start=.2f;pool[1].texture_start=.8f;pool[0].texture_end=0;pool[1].texture_end=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==1 && c.used==0,"entry crossing inserts a near-plane path point");
    CHECK(close(got.z[0][0],1) && close(got.scalar[0][0],.4f) && close(got.x[0][0],2.0f/3.0f),"intersection interpolates position and scalar before projection");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=2;pool[1].point.z=.5f;pool[1].point.x=2;pool[0].width=pool[1].width=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==3 && c.used==0,"exit crossing emits the visible prefix with width taper");
    CHECK(contains_point(&got,4.0f/3.0f,1),"exit intersection retains near depth and projected endpoint");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=pool[1].width=1;
    pool[0].next=pool+1;pool[1].previous=pool;pool[0].flags=7;pool[0].texture_end=.25f;pool[1].texture_end=1.25f;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==1,"UV flags retain one ribbon polygon");
    CHECK(close(got.u[0][0],1) && close(got.v[0][0],.75f) && close(got.v[0][2],-.25f),"flip and axis exchange map UVs per vertex");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=1;pool[1].width=3;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==3 && c.polygons_emitted==3,"widening ribbon splits into three clipped polygons");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=3;pool[1].width=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==3 && c.polygons_emitted==3,"narrowing ribbon splits into three clipped polygons");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=pool[2].point.z=2;
    pool[1].point.x=2;pool[2].point.x=2;pool[2].point.y=2;
    pool[0].width=pool[1].width=pool[2].width=1;
    pool[0].next=pool+1;pool[1].next=pool+2;pool[1].previous=pool;pool[2].previous=pool+1;c.used=3;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==2 && c.paths_emitted==1,"three point path produces two joined segments");
    CHECK(close(got.x[0][2],.5f) && close(got.y[0][2],.5f) && close(got.x[1][1],.5f) && close(got.y[1][1],.5f),"miter shares the outgoing and incoming joint corner");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=pool[2].point.z=2;
    pool[1].point.x=2;pool[2].point.x=.1f;pool[2].point.y=.2f;
    pool[0].width=pool[1].width=pool[2].width=1;
    pool[0].next=pool+1;pool[1].next=pool+2;pool[1].previous=pool;pool[2].previous=pool+1;c.used=3;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==2,"near reversal remains a bounded miter path");
    {
        double length=sqrt(1.9*1.9+.2*.2);
        double expected_x=1.0+(-.5*.2/length)*5.0;
        double expected_y=(.5-.5*1.9/length)*5.0;
        CHECK(fabs(pool[0].corners[2].x-expected_x)<.001 && fabs(pool[0].corners[2].y-expected_y)<.001,
            "independent reversal oracle verifies cosine clamp and shared corner");
        CHECK(close(pool[0].corners[2].x,pool[1].corners[1].x) && close(pool[0].corners[2].y,pool[1].corners[1].y),
            "miter corner is shared across adjoining segment records");
    }
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=2;pool[1].point.z=.5f;pool[2].point.z=pool[3].point.z=2;
    pool[0].point.x=0;pool[1].point.x=2;pool[2].point.x=4;pool[3].point.x=6;
    pool[0].width=pool[1].width=pool[2].width=pool[3].width=1;
    pool[0].next=pool+1;pool[1].next=pool+2;pool[2].next=pool+3;
    pool[1].previous=pool;pool[2].previous=pool+1;pool[3].previous=pool+2;c.used=4;
    CHECK(bh_sprite_oriented_draw(&c) && got.calls==5 && c.paths_emitted==2 && c.used==0,"near excursion divides ribbon into two visible chains");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=.5f;pool[0].next=pool+1;pool[1].previous=pool;c.used=2;
    CHECK(bh_sprite_oriented_draw(&c) && !got.calls && c.used==0,"fully near-clipped path emits no primitive");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=.5f;pool[1].point.z=2;pool[0].next=pool+1;pool[1].previous=pool;c.used=c.capacity=2;
    CHECK(!bh_sprite_oriented_draw(&c) && c.error==BH_SPRITE_ORIENTED_CAPACITY && !got.calls,"crossing without scratch capacity reports failure before emission");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[0].next=pool+1;pool[1].previous=pool+1;c.used=2;
    CHECK(!bh_sprite_oriented_draw(&c) && c.error==BH_SPRITE_ORIENTED_TOPOLOGY && !got.calls,"cyclic previous links fail with a bounded walk");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=pool[1].width=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;got.reject=1;
    CHECK(!bh_sprite_oriented_draw(&c) && c.error==BH_SPRITE_ORIENTED_EMIT && got.calls==1,"primitive callback failure is surfaced");
    setup(&c,pool,&clip,&image,&got);
    pool[0].point.z=pool[1].point.z=2;pool[1].point.x=2;pool[0].width=pool[1].width=1;
    pool[0].next=pool+1;pool[1].previous=pool;c.used=2;pool[0].texture_end=2147483648.0f;
    CHECK(!bh_sprite_oriented_draw(&c) && c.error==BH_SPRITE_ORIENTED_NUMERICAL && !got.calls,"texture coordinate conversion rejects out-of-range input");
    CHECK(bh_math_environment_supported(),"oriented sprite owners preserve caller PC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"sprite-oriented\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failed);
    return failed?1:0;
}
#ifdef BH_SPRITE_ORIENTED_TEST_MAIN
int main(void){unsigned long count=0;return bh_sprite_oriented_checkpoint(&count);}
#endif
