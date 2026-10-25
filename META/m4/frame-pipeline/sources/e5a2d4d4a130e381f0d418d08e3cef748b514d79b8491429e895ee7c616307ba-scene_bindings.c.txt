#include "../src/bh_scene_bindings.h"
#include "../src/bh_native_primitive.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks;static int failures;
static void check(int ok,const char *label,int line) {
    ++checks;if(!ok){++failures;if(failures<16)
        printf("FAIL scene bindings %d: %s\n",line,label);}
}
#define CHECK(x,s) check((x)!=0,s,__LINE__)

int bh_scene_bindings_checkpoint(const char *archive,unsigned long *count) {
    BHWorldGeometry worlds[2],foreign;BHSceneBindingInput inputs[2];
    BHPickupRoom rooms[2];BHEntityScheduleRoom scheduled[2];
    BHEntity actor,pickup;
    BHSceneBindings bindings;BHRenderResources resources;BHModelContext models;
    BHModelInstanceContext prototype;BHTextureContext textures;
    BHMainCamera camera;BHViewCamera view_camera;BHViewConfig viewport;BHClipContext clip;
    BHEntityScheduleContext schedule_context;BHDrawQueue queue;
    unsigned long saved_flags;
    (void)archive;checks=0;failures=0;
    memset(worlds,0,sizeof(worlds));memset(&foreign,0,sizeof(foreign));
    memset(inputs,0,sizeof(inputs));memset(&actor,0,sizeof(actor));
    memset(&pickup,0,sizeof(pickup));memset(rooms,0xa5,sizeof(rooms));
    memset(scheduled,0xa5,sizeof(scheduled));memset(&bindings,0,sizeof(bindings));
    memset(&resources,0,sizeof(resources));memset(&models,0,sizeof(models));
    memset(&textures,0,sizeof(textures));memset(&prototype,0,sizeof(prototype));
    memset(&camera,0,sizeof(camera));memset(&view_camera,0,sizeof(view_camera));
    memset(&viewport,0,sizeof(viewport));
    memset(&clip,0,sizeof(clip));memset(&queue,0,sizeof(queue));
    memset(&schedule_context,0,sizeof(schedule_context));
    CHECK(bh_model_context_init(&models,&textures,NULL),"real model context initialized");
    prototype.camera=&camera;prototype.config=&viewport;prototype.clip=&clip;
    prototype.emit=bh_native_primitive_emit;
    CHECK(bh_render_resources_init(&resources,&models,&prototype,NULL),
          "real empty resource owner initialized");
    schedule_context.queue=&queue;schedule_context.viewport=&viewport;
    schedule_context.camera=&view_camera;
    worlds[0].navigation.next=&worlds[1].navigation;
    worlds[1].navigation.next=&worlds[0].navigation;
    worlds[0].previous=&worlds[1];worlds[1].previous=&worlds[0];
    worlds[0].navigation.count=1;worlds[1].owner=worlds;
    worlds[0].navigation.portals[0].neighbor=&worlds[1].navigation;
    inputs[0].world=worlds;inputs[1].world=worlds+1;
    inputs[0].actors=&actor;inputs[0].pickups=&pickup;
    inputs[0].resource_flags=0x40;inputs[0].angles.y=1.25f;
    CHECK(bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                 &resources,&schedule_context),
          "source room arrays bind explicit scene ring and sentinel");
    CHECK(bindings.first==worlds && !bindings.cached &&
          bindings.scene_count==2 && bindings.schedule_context==&schedule_context,
          "scene owner sees world head, empty cached room and schedule context");
    CHECK(bh_scene_bindings_schedule(&bindings,worlds+1)==scheduled+1 &&
          bh_scene_bindings_schedule(&bindings,&foreign)==NULL,
          "bounded world identity lookup");
    CHECK(scheduled[0].room==rooms && scheduled[0].next==scheduled+1 &&
          scheduled[0].portal_targets[0]==scheduled+1 &&
          scheduled[1].next==scheduled && scheduled[1].linked_active==1 &&
          scheduled[0].linked_active==0,
          "constructor owner and scene ring provide linked successor and sentinel");
    CHECK(rooms[0].actors==&actor && rooms[0].pickups==&pickup &&
          scheduled[0].resource_flags==0x40 && scheduled[0].angles.y==1.25f,
          "borrowed actor/pickup heads and explicit resource metadata preserved");
    CHECK(!bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                  &resources,&schedule_context),
          "populated owner rejects reinitialization");
    bh_scene_bindings_dispose(&bindings);
    memset(rooms,0xa5,sizeof(rooms));memset(scheduled,0xa5,sizeof(scheduled));
    saved_flags=worlds[0].query_flags;worlds[0].query_flags=4;
    CHECK(!bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                  &resources,&schedule_context) &&
          bindings.error==BH_BINDINGS_RESOURCE &&
          ((unsigned char *)rooms)[0]==0xa5,
          "missing real draw resource rejects before output publication");
    worlds[0].query_flags=saved_flags;
    worlds[0].navigation.portals[0].neighbor=&foreign.navigation;
    CHECK(!bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                  &resources,&schedule_context) &&
          bindings.error==BH_BINDINGS_TOPOLOGY,
          "foreign portal neighbor rejects");
    worlds[0].navigation.portals[0].neighbor=&worlds[1].navigation;
    inputs[1].world=worlds;
    CHECK(!bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                  &resources,&schedule_context) &&
          bindings.error==BH_BINDINGS_TOPOLOGY,
          "duplicate room identity rejects");
    inputs[1].world=worlds+1;
    worlds[0].owner=worlds+1;
    CHECK(!bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                  &resources,&schedule_context) &&
          bindings.error==BH_BINDINGS_TOPOLOGY,
          "cyclic owner relationship without root sentinel rejects");
    worlds[0].owner=NULL;
    CHECK(bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
                                 &resources,&schedule_context),
          "root sentinel bounds a cyclic scene ring");
    bh_scene_bindings_dispose(&bindings);
    bh_render_resources_dispose(&resources);
    *count=checks;
    printf("{\"scenario\":\"scene-bindings\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",
           checks,failures);
    return failures!=0;
}
#ifdef BH_SCENE_BINDINGS_STANDALONE
int main(void) {
    unsigned long count=0;
    return bh_scene_bindings_checkpoint(NULL,&count);
}
#endif
