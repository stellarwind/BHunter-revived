#include "../src/bh_horizon_queue.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL horizon: %s\n",s);}}while(0)
static unsigned long bits(float f){unsigned long n;memcpy(&n,&f,4);return n;}
typedef struct Capture {BHHorizonContext *context;BHTextureImage *image;unsigned long calls,count[8],gradient[8],fail_at;BHClipVertex v[8][4];int valid;} Capture;
static void *__cdecl allocate(unsigned long n,void *u){return malloc(n);}
static void __cdecl release(void *p,void *u){free(p);}
/* Isolated recording leaf, never a shipping software raster implementation. */
static int __cdecl capture(BHClipVertex *ring,BHTextureImage *image,int gradient,const float rgb[3],void *user) {
    Capture *c=(Capture *)user;BHClipVertex *p=ring;unsigned long n=0,i=c->calls++;
    if(i>=8 || image!=c->image || c->context->state->depth!=2 || rgb!=c->context->rgb){c->valid=0;return 0;}
    do{if(n==4 || p->next->previous!=p || p->previous->next!=p){c->valid=0;return 0;}c->v[i][n++]=*p;p=p->next;}while(p!=ring);
    c->gradient[i]=gradient;c->count[i]=n;return c->fail_at!=i+1;
}
static BHTextureImage *__cdecl lookup(unsigned long handle,void *user){return handle==42?(BHTextureImage *)user:NULL;}
static void setup(BHTextureContext *textures,BHHorizonColor *color,BHRenderState *state,BHMainCamera *camera,BHAttributeClipContext *clip,BHHorizonContext *h,Capture *got,BHTextureImage *image) {
    memset(textures,0,sizeof(*textures));textures->software=1;textures->allocator.allocate=allocate;textures->allocator.release=release;
    textures->display_right[0]=textures->display_right[2]=3;textures->display_right[1]=2;textures->display_left[0]=11;textures->display_left[1]=5;
    memset(camera,0,sizeof(*camera));camera->position.x=10;camera->position.z=20;
    camera->projected.m[0][0]=camera->projected.m[1][1]=camera->projected.m[2][2]=1;camera->projected.m[2][3]=100;
    memset(image,0,sizeof(*image));memset(got,0,sizeof(*got));got->valid=1;got->context=h;got->image=image;
    CHECK(bh_horizon_color_init(color,textures) && bh_render_state_init(state,textures) && bh_render_depth(state,1),"real software color and state definitions");
    CHECK(bh_attribute_clip_init(clip,1,1000,-1000,1000,-1000,1000) && bh_horizon_init(h,camera,clip,state,color,capture,got),"real attribute clipper with explicit recording leaf");
}
static void geometry(void) {
    BHTextureContext textures;BHHorizonColor color;BHRenderState state;BHMainCamera camera;BHAttributeClipContext clip;BHHorizonContext h;Capture got;BHTextureImage image;
    unsigned long i;setup(&textures,&color,&state,&camera,&clip,&h,&got,&image);
    CHECK(bh_horizon_draw(&h,&image,50,5,25,12.5f,0xff804000UL),"complete software branch with recording leaf");
    CHECK(h.attempted==8 && h.emitted==8 && got.calls==8 && got.valid && state.depth==1 && !state.barriers && !textures.frame,"all polygons and depth lifetime without native barrier");
    for(i=0;i<8;++i)CHECK(got.count[i]==(i<4?3:4) && got.gradient[i]==(i>=4),"software lower plain versus upper gradient dispatch and winding");
    CHECK(got.v[0][0].point.x==-2.5f && got.v[0][0].point.y==-2.5f && got.v[0][0].point.z==132.5f &&
        got.v[0][1].point.x==22.5f && got.v[0][2].point.x==10 && got.v[0][2].point.y==-5 && got.v[0][2].point.z==120,
        "first triangle independently expected innerA innerB center");
    CHECK(got.v[3][0].point.x==-2.5f && got.v[3][0].point.y==-7.5f && got.v[3][0].point.z==107.5f,
        "negative inner corner retains signed height component");
    CHECK(got.v[4][0].point.x==-15 && got.v[4][0].point.y==0 && got.v[4][0].point.z==145 &&
        got.v[4][1].point.x==35 && got.v[4][2].point.x==22.5f && got.v[4][3].point.x==-2.5f,
        "first outer quad independently expected outerA outerB innerB innerA");
    CHECK(got.v[6][0].point.x==35 && got.v[6][0].point.y==-10 && got.v[6][0].point.z==95,"negative outer corner height and projection");
    CHECK(got.v[4][0].unknown28==0x3f800000UL && got.v[4][1].unknown28==0x3f800000UL && !got.v[4][2].unknown28 && !got.v[4][3].unknown28,"float scalar coverage outer1 inner0");
    CHECK(bits(got.v[0][0].u)==0xbd4cccccUL && bits(got.v[0][0].v)==0x3f266666UL &&
        bits(got.v[0][1].u)==0x3ee66666UL && bits(got.v[4][0].u)==0xbe999999UL,"world-space X/Z UV and binary32 inverse scale");
    CHECK(h.rgb[0]==129 && h.rgb[1]==64 && !h.rgb[2] && color.color==0xff804000UL && color.fills==1 && color.indexed,"signed-trunc RGB differs from packed unsigned table extraction");
    got.calls=0;h.color_mode=2;
    CHECK(bh_horizon_draw(&h,&image,50,5,25,12.5f,0xff804000UL) && color.packed && color.fills==2&&textures.software==1,"packed output depth selects its table independently of software backend Boolean");
    got.calls=0;got.fail_at=1;
    CHECK(!bh_horizon_draw(&h,&image,50,5,25,12.5f,0x804000) && h.error==BH_HORIZON_EMIT && got.calls==8 && h.emitted==7 && state.depth==1,"reported leaf failure still completes subsequent faces and restores depth");
    got.calls=0;got.fail_at=0;CHECK(bh_attribute_clip_init(&clip,1000,2000,-1000,1000,-1000,1000),"full rejection clip bounds");
    CHECK(bh_horizon_draw(&h,&image,50,5,25,12.5f,0x804000) && h.attempted==8 && !h.emitted && !got.calls && state.depth==1,"real depth clip rejection completes without invoking leaf");
    CHECK(!bh_horizon_draw(&h,&image,0,5,25,12.5f,0) && !bh_horizon_draw(&h,&image,50,5,0,12.5f,0),"zero divisions rejected as explicit source guards");
    bh_horizon_color_dispose(&color);
}
static void consumer(void) {
    BHTextureContext textures;BHHorizonColor color;BHRenderState state;BHMainCamera camera;BHAttributeClipContext clip;BHHorizonContext h;Capture got;BHTextureImage image;
    BHHorizonQueueContext binding;BHDrawQueue q;unsigned long next=99;
    setup(&textures,&color,&state,&camera,&clip,&h,&got,&image);binding.draw=&h;binding.lookup=lookup;binding.lookup_user=&image;
    CHECK(bh_draw_queue_init(&q,10,NULL) && bh_horizon_queue(&q,42,50,5,25,12.5f,0x804000)==1,"real horizon queue producer");
    CHECK(bh_horizon_queue_execute(&binding,&q,q.commands+9,&next) && next==1 && got.calls==8 && state.mode==0 && state.depth==0,"consumer actual mode0 barrier then full horizon then next load");
    got.calls=0;q.commands[9].resource=100;next=99;
    CHECK(!bh_horizon_queue_execute(&binding,&q,q.commands+9,&next) && next==1 && !got.calls,"missing image handle reports failure but forwards link");
    q.commands[9].next=100;next=99;
    CHECK(!bh_horizon_queue_execute(&binding,&q,q.commands+9,&next) && next==99,"invalid source successor rejected before barrier");
    bh_draw_queue_dispose(&q);bh_horizon_color_dispose(&color);
}
int bh_horizon_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    geometry();consumer();CHECK(bh_math_environment_supported(),"required PC53 profile retained");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"horizon\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);return failures!=0;
}
#ifdef BH_HORIZON_STANDALONE
int main(void){unsigned long count;return bh_horizon_checkpoint(&count);}
#endif
