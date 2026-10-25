#include "../src/bh_draw_controls.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;if(failures<20)printf("FAIL draw-controls: %s\n",s);}} while(0)
static unsigned long bits(float value) {unsigned long word;memcpy(&word,&value,4);return word;}
static void producers(void) {
    BHDrawQueue queue;BHDrawCommand *state,*rectangle;unsigned long i;
    CHECK(bh_draw_queue_init(&queue,20,NULL),"queue initialized");
    CHECK(bh_draw_controls_state(&queue,0,1)==1 &&
          bh_draw_controls_clip_rect(&queue,3,-2.5f,3.25f,12.5f,-7.0f)==1,
          "state and rectangle producers append");
    state=queue.commands+9;rectangle=queue.commands+10;
    CHECK(state->kind==BH_DRAW_TEXTURE_MASK && state->resource==1 &&
          rectangle->kind==BH_DRAW_CLIP_RECT,"original command selectors retained");
    CHECK(rectangle->words[0]==bits(-2.5f) && rectangle->words[1]==bits(3.25f) &&
          rectangle->words[2]==bits(12.5f) && rectangle->words[3]==bits(-7),
          "rectangle float payloads copied verbatim");
    CHECK(queue.commands[1].next==9 && queue.commands[4].next==10,
          "bucket zero and three map to distinct original headers");
    CHECK(bh_draw_queue_begin(&queue),"mapping queue reset");
    for(i=0;i<9;++i)CHECK(bh_draw_controls_state(&queue,i,(long)i)==1,
                           "all nine bucket append paths accepted");
    CHECK(queue.commands[0].next==14 && queue.commands[1].next==9 &&
          queue.commands[2].next==10 && queue.commands[3].next==11 &&
          queue.commands[4].next==12 && queue.commands[5].next==13 &&
          queue.commands[6].next==15 && queue.commands[7].next==16 &&
          queue.commands[8].next==17,"original nine-bucket mapping and links");
    CHECK(bh_draw_queue_dispose(&queue),"producer queue disposed");
    CHECK(bh_draw_queue_init(&queue,9,NULL),"zero-capacity payload queue initialized");
    CHECK(bh_draw_controls_state(&queue,0,1)==0 && queue.error==BH_DRAW_FULL &&
          bh_draw_controls_clip_rect(&queue,0,0,0,1,1)==0 && queue.count==9,
          "full queue drops both producer commands without changing count");
    CHECK(bh_draw_queue_dispose(&queue),"full queue disposed");
    CHECK(bh_draw_controls_state(NULL,0,1)==-1 &&
          bh_draw_controls_clip_rect(NULL,0,0,0,1,1)==-1,
          "invalid producer bindings reject");
}
static void consumers(void) {
    BHDrawQueue q;BHDrawControlContext c;BHViewConfig view;
    BHClipContext model;BHScreenClipContext screen;BHAttributeClipContext attribute;
    BHRenderResources resources;unsigned long next=BH_DRAW_END;
    memset(&c,0,sizeof(c));memset(&resources,0,sizeof(resources));
    CHECK(bh_draw_queue_init(&q,20,NULL),"consumer queue initialized");
    CHECK(bh_view_config_init(&view,100,80),"view initialized");
    CHECK(bh_clip_init(&model,1,10,0,100,0,80),"model clip initialized");
    CHECK(bh_screen_clip_init(&screen,1,10,0,100,0,80),"screen clip initialized");
    CHECK(bh_attribute_clip_init(&attribute,1,10,0,100,0,80),"attribute clip initialized");
    c.view=&view;c.resources=&resources;c.model_clip=&model;
    c.screen_clip=&screen;c.attribute_clip=&attribute;
    CHECK(bh_draw_controls_state(&q,0,3)==1 &&
          bh_draw_controls_clip_rect(&q,0,10,10,20,20)==1 &&
          bh_draw_controls_clip_rect(&q,0,-10,-20,200,200)==1,
          "state and consecutive rectangles enqueued");
    CHECK(bh_draw_controls_execute(&c,&q,q.commands+9,&next) &&
          next==10 && c.texture_mask==3 && resources.prototype.texture_mask==3,
          "state consumer publishes next and source owner mask");
    CHECK(bh_draw_controls_execute(&c,&q,q.commands+10,&next) && next==11 &&
          view.left==0 && view.right==100,
          "first rectangle coalesces against immediate following rectangle");
    CHECK(bh_draw_controls_execute(&c,&q,q.commands+11,&next) &&
          next==2 && view.left==0 && view.top==0 &&
          view.right==100 && view.bottom==80,
          "last rectangle applies asymmetric viewport clamp");
    CHECK(model.left==0 && model.right==100 && screen.top==0 &&
          screen.bottom==80 && attribute.left==0 && attribute.bottom==80,
          "all three recovered clipping consumers receive bounds");
    CHECK(bh_draw_queue_begin(&q),"consumer queue reset");
    CHECK(bh_draw_controls_clip_rect(&q,3,110,90,-30,-20)==1,
          "inverted rectangle payload accepted");
    CHECK(bh_draw_controls_execute(&c,&q,q.commands+9,&next) &&
          view.left==110 && view.right==80 && view.top==90 && view.bottom==70,
          "asymmetric original clamp retains inverted bounds");
    CHECK(!bh_draw_controls_execute(&c,&q,NULL,&next) &&
          !bh_draw_controls_execute(&c,&q,q.commands+9,NULL),
          "consumer rejects missing command and output");
    q.commands[9].next=q.count+1;
    CHECK(!bh_draw_controls_execute(&c,&q,q.commands+9,&next),
          "malformed next link rejected before viewport mutation");
    CHECK(bh_draw_queue_dispose(&q),"consumer queue disposed");
}
static void real_mask(const char *archive) {
    BHSceneView view;BHTextureContext textures;BHModelContext models;
    BHRenderResources resources;BHModelInstanceContext prototype;
    BHMainCamera camera;BHClipContext clip;BHNativePrimitive native;
    BHDrawControlContext controls;const BHDrawResource *descriptor;
    unsigned long handle;
    if(!archive)return;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));
    memset(&models,0,sizeof(models));memset(&resources,0,sizeof(resources));
    memset(&prototype,0,sizeof(prototype));memset(&camera,0,sizeof(camera));
    memset(&native,0,sizeof(native));memset(&controls,0,sizeof(controls));
    CHECK(bh_scene_view_init(&view,64,64),"real-resource scene target initialized");
    if(!view.device)return;
    CHECK(bh_vfile_mount(archive),"real-resource archive mounted as data");
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) &&
          bh_texture_formats(&textures) &&
          bh_texture_set_path(&textures,"TEXTURES",2),"real texture source bindings");
    CHECK(bh_model_context_init(&models,&textures,NULL),"real model context initialized");
    CHECK(bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) &&
          bh_clip_init(&clip,view.projection.near_z,view.projection.far_z,
              view.projection.left,view.projection.right,
              view.projection.top,view.projection.bottom),"real camera/clip bound");
    native.textures=&textures;native.near_z=view.projection.near_z;
    prototype.camera=&camera;prototype.config=&view.projection;
    prototype.clip=&clip;prototype.emit=bh_native_primitive_emit;
    prototype.emit_user=&native;
    CHECK(bh_render_resources_init(&resources,&models,&prototype,NULL),
          "real resource owner initialized");
    handle=bh_render_resources_load_model(&resources,"BSP\\PU_9A.BSP");
    descriptor=bh_render_resources_lookup(handle,&resources);
    CHECK(handle && descriptor && descriptor->tag==BH_RESOURCE_TAG_MODEL,
          "real BSP loaded into model resource owner");
    controls.resources=&resources;
    CHECK(bh_draw_controls_texture_mask(&controls,7) && controls.texture_mask==7 &&
          resources.prototype.texture_mask==7 && descriptor &&
          descriptor->data.model.instance->texture_mask==7,
          "00458300 source binding propagates to real owned model and prototype");
    bh_render_resources_dispose(&resources);
    bh_texture_context_dispose(&textures);
    bh_scene_view_dispose(&view);
}
int bh_draw_controls_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment fp;
    if(!bh_math_environment_enter(&fp))return 2;
    producers();consumers();real_mask(archive);
    bh_math_environment_leave(&fp);
    if(count)*count=checks;
    printf("{\"scenario\":\"draw-controls\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_DRAW_CONTROLS_STANDALONE
int main(int argc,char **argv) {return bh_draw_controls_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif
