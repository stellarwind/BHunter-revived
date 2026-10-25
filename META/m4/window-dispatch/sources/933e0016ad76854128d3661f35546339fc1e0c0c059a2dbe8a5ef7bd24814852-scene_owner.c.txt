#include "../src/bh_scene_owner.h"
#include "../src/bh_draw_controls.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL scene-owner: %s\n",s);}} while(0)
static float value(const unsigned long *words) {float v;memcpy(&v,words,4);return v;}
static int close_to(float a,float b) {return fabs((double)a-b)<0.002;}

typedef struct Fixture {
    BHWorldGeometry world[2];BHPickupRoom rooms[2];BHEntityScheduleRoom scheduled[2];
    BHSceneBindings bindings;BHEntityScheduleContext scheduler;
    BHDrawQueue queue;BHViewConfig viewport;BHViewCamera camera;BHSceneOwner owner;
} Fixture;
static int setup(Fixture *f,unsigned long capacity) {
    unsigned long i;memset(f,0,sizeof(*f));
    for(i=0;i<2;++i) {
        f->world[i].navigation.next=&f->world[(i+1)%2].navigation;
        f->world[i].previous=&f->world[(i+1)%2];
        f->world[i].minimum.x=i?10:-10;f->world[i].maximum.x=i?20:0;
        f->world[i].minimum.y=f->world[i].minimum.z=-10;
        f->world[i].maximum.y=f->world[i].maximum.z=10;
        f->rooms[i].world=f->world+i;f->scheduled[i].room=f->rooms+i;
    }
    f->bindings.first=f->world;f->bindings.scene_count=2;
    f->bindings.rooms=f->rooms;f->bindings.scheduled=f->scheduled;
    f->bindings.schedule_context=&f->scheduler;
    f->scheduler.viewport=&f->viewport;f->scheduler.camera=&f->camera;
    f->scheduler.queue=&f->queue;f->scheduler.max_rooms=2;
    f->scheduler.max_entities=8;f->scheduler.max_depth=2;
    f->viewport.left=f->viewport.top=-4;
    f->viewport.right=f->viewport.bottom=4;
    f->viewport.width=800;f->viewport.height=600;
    if(!bh_draw_queue_init(&f->queue,capacity,NULL))return 0;
    return bh_scene_owner_init(&f->owner,&f->bindings,800,600);
}
static void cleanup(Fixture *f) {bh_draw_queue_dispose(&f->queue);}
static void matrix_and_reset(void) {
    Fixture f;BHVec3 eye={0,0,0},angles={0,0,0};
    BHDrawControlContext control;unsigned long next=0;
    float focal=(float)(320.0/tan(30.0*0.0174532925199433));
    CHECK(setup(&f,32),"camera fixture setup");
    CHECK(bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,641,481),"odd camera pass");
    CHECK(f.bindings.cached==f.world && f.owner.frame==1 &&
          f.scheduler.frame==1,"initial room and frame");
    CHECK(close_to(f.owner.focal,focal) && close_to(f.owner.camera.projected.m[0][0],focal) &&
          close_to(f.owner.camera.projected.m[1][1],focal) &&
          f.owner.camera.projected.m[0][2]==320 &&
          f.owner.camera.projected.m[1][2]==240,"odd dimensions use truncated halves");
    CHECK(f.owner.camera.position.x==eye.x &&
          f.owner.camera.position.z==eye.z &&
          f.scheduler.camera==&f.owner.camera,"scheduler receives projected camera and eye");
    CHECK(f.viewport.left==0 && f.viewport.right==641 &&
          f.viewport.width==800 && f.viewport.height==600 &&
          f.viewport.near_z==1 && f.viewport.far_z==100,"camera clip preserves target extent");
    CHECK(f.queue.count==11 && f.queue.commands[9].kind==BH_DRAW_CLIP_RECT &&
          f.queue.commands[10].kind==BH_DRAW_CLIP_RECT &&
          value(f.queue.commands[9].words+2)==800 &&
          value(f.queue.commands[10].words+3)==600,"two full target resets after schedule");
    memset(&control,0,sizeof(control));control.view=&f.viewport;
    CHECK(bh_draw_controls_execute(&control,&f.queue,f.queue.commands+10,&next) &&
          f.viewport.right==800 && f.viewport.bottom==600,
          "queued reset applies full target beyond camera extent");
    cleanup(&f);
}
static void cache_and_gate(void) {
    Fixture f;BHVec3 eye={15,0,0},angles={0,0,0};unsigned long frame;
    CHECK(setup(&f,64),"cache fixture setup");
    CHECK(bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.bindings.cached==f.world+1,"exact room lookup");
    bh_draw_queue_begin(&f.queue);eye.x=20.5f;
    CHECK(bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.bindings.cached==f.world+1,"one-unit expanded lookup");
    bh_draw_queue_begin(&f.queue);eye.x=30;
    CHECK(bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.bindings.cached==f.world+1,"fallback retains old search start");
    bh_draw_queue_begin(&f.queue);eye.x=-5;
    CHECK(bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.bindings.cached==f.world,"invalid cached room triggers other room lookup");
    frame=f.owner.frame;f.owner.mode=1;
    CHECK(!bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.owner.error==BH_SCENE_OWNER_INACTIVE && !f.bindings.cached &&
          f.owner.frame==frame,"mode gate clears cache without frame increment");
    f.owner.mode=0;f.bindings.first=0;
    CHECK(!bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.owner.error==BH_SCENE_OWNER_INACTIVE,"missing scene gate");
    cleanup(&f);
}
static void rotation_order(void) {
    Fixture f;BHVec3 eye={0,0,0};
    BHVec3 angles={1.5707963267948966f,1.5707963267948966f,1.5707963267948966f};
    CHECK(setup(&f,32),"rotation fixture setup");
    CHECK(bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480),
          "quarter-turn rotation pass");
    CHECK(close_to(f.owner.view.m[0][0],1) && close_to(f.owner.view.m[1][2],1) &&
          close_to(f.owner.view.m[2][1],-1) && close_to(f.owner.view.m[0][2],0),
          "negative Y then X then Z world rotation order");
    cleanup(&f);
}
static void errors(void) {
    Fixture f;BHVec3 eye={0,0,0},angles={0,0,0};
    CHECK(setup(&f,10),"capacity fixture setup");
    CHECK(!bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.owner.error==BH_SCENE_OWNER_QUEUE && f.owner.frame==1 &&
          f.queue.error==BH_DRAW_FULL,"second reset detects full queue");
    bh_draw_queue_begin(&f.queue);
    CHECK(!bh_scene_owner_step(&f.owner,&eye,&angles,0,1,100,640,480) &&
          f.owner.error==BH_SCENE_OWNER_ARGUMENT && f.owner.frame==1,
          "invalid FOV leaves frame and queue unchanged");
    f.scheduler.max_rooms=0;
    CHECK(!bh_scene_owner_step(&f.owner,&eye,&angles,60,1,100,640,480) &&
          f.owner.error==BH_SCENE_OWNER_SCHEDULE &&
          f.scheduler.error==BH_SCHEDULE_ARGUMENT && f.owner.frame==2,
          "scheduler error follows frame advance");
    cleanup(&f);
}
int bh_scene_owner_tests(void) {
    checks=failures=0;matrix_and_reset();cache_and_gate();rotation_order();errors();
    printf("scene-owner: %lu checks, %lu failures\n",checks,failures);
    return failures==0;
}
int bh_scene_owner_checkpoint(unsigned long *count) {
    bh_scene_owner_tests();
    if(count)*count=checks;
    printf("{\"scenario\":\"scene-owner\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_SCENE_OWNER_STANDALONE
int main(void) {unsigned long count=0;return bh_scene_owner_checkpoint(&count);}
#endif
