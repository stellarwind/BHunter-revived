#include "../src/bh_path_queue.h"
#include "../src/bh_queue_reserve.h"
#include "../src/bh_weapon_hud_arc.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
static unsigned long checks,failures,emits;
static BHDrawCommand *mutate_next;static int reject;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL path-queue: %s\n",s);}}while(0)
typedef struct Fixture {
    BHPathQueueContext c;BHViewConfig view;BHMainCamera camera;
    BHSpriteOrientedContext ribbon;BHSpriteOrientedVertex pool[64];
    BHAttributeClipContext clip;BHTextureImage image;BHDrawQueue queue;
} Fixture;
static BHTextureImage *__cdecl lookup(unsigned long id,void *raw){return id==77?(BHTextureImage *)raw:NULL;}
/* Isolated observable polygon sink; all path selection, transform, ribbon
   construction and clipping run their real recovered definitions. */
static int __cdecl emit(BHClipVertex *ring,BHTextureImage *image,unsigned long color,void *raw) {
    BHClipVertex *p=ring;unsigned long n=0;Fixture *f=(Fixture *)raw;
    ++emits;
    if(image!=&f->image || color!=0xff000000UL)++failures;
    do{if(!p || !p->next || !p->previous || p->next->previous!=p || p->previous->next!=p || ++n>12){++failures;return 0;}p=p->next;}while(p!=ring);
    if(mutate_next)mutate_next->next=7;
    return !reject;
}
static int setup(Fixture *f) {
    memset(f,0,sizeof(*f));emits=0;mutate_next=NULL;reject=0;
    if(!bh_view_config_init(&f->view,128,128) || !bh_main_camera_settings(&f->view,90,1,100) ||
       !bh_main_camera_pose(&f->view,&f->camera,0,0,0,0,0,0) ||
       !bh_attribute_clip_init(&f->clip,1,100,0,128,0,128) || !bh_draw_queue_init(&f->queue,64,NULL))return 0;
    f->ribbon.path=f->pool;f->ribbon.capacity=64;f->ribbon.near_z=1;f->ribbon.projection_scale=f->camera.focal;
    f->ribbon.clip=&f->clip;f->ribbon.image=&f->image;f->ribbon.color=0xff000000UL;f->ribbon.emit=emit;f->ribbon.emit_user=f;
    f->c.draw=&f->ribbon;f->c.view=&f->view;f->c.camera=&f->camera;f->c.lookup=lookup;f->c.lookup_user=&f->image;
    return 1;
}
static void put(unsigned long *p,float value){memcpy(p,&value,4);}
static void record(BHDrawCommand *p,float x,float z,float scalar,float index) {
    p->kind=BH_DRAW_PATH_POINT;put(&p->resource,x);put(p->words,0);put(p->words+1,z);
    put(p->words+2,1);put(p->words+3,.75f);put(p->words+4,index);put(p->words+5,scalar);p->words[6]=3;
}
static BHDrawCommand *path(Fixture *f,unsigned long count,float z) {
    BHDrawCommand *h,*p;unsigned long i;
    bh_draw_queue_begin(&f->queue);h=bh_queue_reserve(&f->queue,3);h->kind=BH_DRAW_PATH_HEADER;h->resource=77;h->words[0]=count;
    for(i=0;i<count;++i){p=bh_queue_reserve(&f->queue,3);record(p,(float)i-2,z,10-(float)i*.5f,(float)i);}
    return h;
}
static int close(float a,double b){return fabs((double)a-b)<.00002;}
static void appends(void) {
    Fixture f;BHVec3 p={1,2,3};BHVec3 corners[4];unsigned long i;
    CHECK(setup(&f),"real path fixture dependencies");memset(corners,0xa5,sizeof(corners));
    memcpy(f.pool[0].corners,corners,sizeof(corners));
    memset(&f.camera.projected,0,sizeof(f.camera.projected));
    f.camera.projected.m[0][0]=2;f.camera.projected.m[1][1]=3;f.camera.projected.m[2][2]=4;
    f.camera.projected.m[0][3]=10;f.camera.projected.m[1][3]=20;f.camera.projected.m[2][3]=30;
    CHECK(bh_path_queue_append(&f.c,&p,5,9,.25f,7) && f.ribbon.used==1 && f.c.accepted==1,
        "complete append increments actual shared ribbon pool once");
    CHECK(f.pool[0].point.x==12 && f.pool[0].point.y==26 && f.pool[0].point.z==42 &&
        f.pool[0].width==5 && f.pool[0].flags==9 && f.pool[0].texture_start==.25f && f.pool[0].texture_end==7 &&
        !f.pool[0].previous && !f.pool[0].next && !memcmp(f.pool[0].corners,corners,sizeof(corners)),
        "independent affine transform, exact field order, initial links and untouched corner scratch");
    CHECK(bh_path_queue_append(&f.c,&p,6,4,.5f,8) && f.ribbon.used==2 && f.pool[1].previous==f.pool && f.pool[0].next==f.pool+1 && !f.pool[1].next,
        "append preserves existing count and updates prior real pool link");
    f.ribbon.capacity=2;i=f.ribbon.used;
    CHECK(!bh_path_queue_append(&f.c,&p,1,0,0,0) && f.c.error==BH_PATH_QUEUE_CAPACITY && f.ribbon.used==i,
        "bounded pool failure preserves all previously published vertices");
    bh_draw_queue_dispose(&f.queue);
}
static void selections(void) {
    Fixture f;BHDrawCommand *h;long n;unsigned long bits;float z;
    CHECK(setup(&f),"real adaptive cull fixture");h=path(&f,5,33);
    n=bh_path_queue_select(&f.c,h+1,4,0x3f);
    CHECK(n==2 && f.ribbon.used==2 && f.c.deepest==1,"strict ratio16 equality subdivides into two accepted starts");
    CHECK(close(f.pool[0].point.z,33) && close(f.pool[1].point.z,33) && f.pool[0].texture_end==0 && f.pool[1].texture_end==2,
        "first half precedes second; source point-index scalars and real transforms survive selection");
    memcpy(&bits,&(h+1)->words[1],4);++bits;memcpy(&z,&bits,4);
    f.ribbon.used=f.c.accepted=f.c.culled=f.c.deepest=0;h=path(&f,5,z);
    CHECK(bh_path_queue_select(&f.c,h+1,4,0x3f)==1 && f.c.deepest==0,"one binary32 depth step above ratio16 accepts entire span");
    f.ribbon.used=f.c.accepted=f.c.culled=f.c.deepest=0;h=path(&f,5,-10);
    CHECK(bh_path_queue_select(&f.c,h+1,4,0x3f)==0 && f.c.culled==1 && !f.ribbon.used,"actual sphere owner rejects near-hidden span");
    CHECK(bh_path_queue_select(&f.c,h+1,1,0x3f)==1 && f.ribbon.used==1 && f.pool[0].point.z==-10,
        "single span bypasses culling and appends original start even behind near plane");
    f.ribbon.used=0;h=path(&f,1,5);
    _clear87();
    CHECK(bh_path_queue_select(&f.c,h+1,0,0x3f)==1 && f.ribbon.used==1 && (_status87()&_SW_ZERODIVIDE),
        "visible zero-span acceptance retains actual masked division sticky status without infinite recursion");_clear87();
    f.ribbon.used=0;h=path(&f,5,20);put(h[1].words+5,0);
    CHECK(bh_path_queue_select(&f.c,h+1,4,0x3f)==-1 && f.c.error==BH_PATH_QUEUE_NUMERICAL && !f.ribbon.used,
        "negative radius outside monotone arc contract diagnosed");
    bh_draw_queue_dispose(&f.queue);
}
static void consumers(void) {
    Fixture f;BHDrawCommand *h;unsigned long next,i;BHArcContext arc;
    BHVec3 a={-2,0,10},b={-1,1,12},c={1,1,14},d={2,0,16};
    CHECK(setup(&f),"real queued path consumer fixture");h=path(&f,5,5);next=99;
    CHECK(bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==5 && f.c.accepted==5 && !f.ribbon.used && emits>=4,
        "two independent recursive halves plus final point render actual clipped ribbon and reset its pool");
    h=path(&f,1,5);emits=0;_clear87();
    CHECK(bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==5 && f.c.accepted==3 && !f.ribbon.used && emits==2 &&
        (_status87()&_SW_INVALID),"one-point partial arc retains duplicate starts and original masked unordered miter behavior");_clear87();
    h=path(&f,2,5);emits=0;
    CHECK(bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==5 && f.c.accepted==3 && !f.ribbon.used && emits==2,
        "two-point partial arc preserves zero-span first half and real nonzero final edge");_clear87();
    h=path(&f,5,-10);emits=0;
    CHECK(bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==5 && !f.c.accepted && f.c.culled==2 && !emits,
        "both fully culled halves skip final append and rendering but publish real final continuation");
    h=path(&f,5,5);mutate_next=h+5;
    CHECK(bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==7,"continuation read after synchronous emitter mutation of final point link");mutate_next=NULL;
    h=path(&f,5,5);reject=1;
    CHECK(!bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==5 && f.c.error==BH_PATH_QUEUE_DRAW && f.ribbon.used==5,
        "real draw failure preserves selected pool state and still publishes continuation");reject=0;f.ribbon.used=0;
    h=path(&f,5,5);h->resource=88;
    CHECK(!bh_path_queue_execute(&f.c,&f.queue,h,&next) && f.c.error==BH_PATH_QUEUE_IMAGE && next==5 && !f.ribbon.used,
        "missing borrowed image diagnosed without invented render success");
    h=path(&f,0,5);next=99;
    CHECK(!bh_path_queue_execute(&f.c,&f.queue,h,&next) && f.c.error==BH_PATH_QUEUE_TOPOLOGY && next==99,
        "header-only exhaustion is explicit invalid original negative-span domain");
    h=path(&f,5,5);h[2].kind=BH_DRAW_NOP;
    CHECK(!bh_path_queue_execute(&f.c,&f.queue,h,&next) && f.c.error==BH_PATH_QUEUE_TOPOLOGY,"contiguous typed point ownership checked before traversal");
    memset(&arc,0,sizeof(arc));arc.queue=&f.queue;arc.image=77;srand(917);bh_draw_queue_begin(&f.queue);
    CHECK(bh_arc_queue(&arc,&a,&b,&c,&d,.8f) && arc.path_points==17 && arc.random_calls==62,"real recovered arc producer and exact shared CRT stream consume source command storage");
    h=f.queue.commands+9;
    CHECK(bh_path_queue_execute(&f.c,&f.queue,h,&next) && next==5 && !f.ribbon.used && f.c.accepted>=3,
        "complete producer to adaptive queue consumer to recovered geometry closure");
    /* Original index/radius metadata words remain in command storage after
       consumption; no command or image pointer is retained by the consumer. */
    for(i=1;i<=17;++i)CHECK(h[i].kind==BH_DRAW_PATH_POINT && h[i].words[6]<4,"all original arc point records retained unchanged");
    bh_draw_queue_dispose(&f.queue);
}
int bh_path_queue_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    appends();selections();consumers();CHECK(bh_math_environment_supported(),"PC53 source profile retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"path-queue\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
