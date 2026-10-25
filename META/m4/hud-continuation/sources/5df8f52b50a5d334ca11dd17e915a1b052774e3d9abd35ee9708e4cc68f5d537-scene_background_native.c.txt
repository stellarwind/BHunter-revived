#include "../src/bh_scene_background.h"
#include "../src/bh_camera_queue.h"
#include "../src/bh_render_resources.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;if(failures<20)printf("FAIL background-native: %s\n",s);}} while(0)
typedef struct Images {BHTextureImage *white,*blue;} Images;
static BHTextureImage *__cdecl image_lookup(unsigned long handle,void *user) {
    Images *images=(Images *)user;
    return handle==1?images->white:handle==2?images->blue:NULL;
}

/* A static source state fixture. No original game loop, constructor callback,
   input, menu, physics stepping or original code is executed. Both BSP names
   and both flame names are statically traced from actual startup assignments. */
int bh_scene_background_native_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment fp;BHSceneView view;BHTextureContext textures;BHModelContext models;
    BHRenderResources resources;BHModelInstanceContext prototype;BHMainCamera camera;
    BHClipContext clip;BHScreenClipContext screen;BHNativePrimitive native;
    BHRenderMaterial material;BHRenderState state;BHDrawQueue queue;BHSceneBackground background;
    BHDrawResourceContext draw;BHDrawSubmitContext submit;
    BHSpriteBillboardContext billboard;BHDrawBillboardSubmitContext billboard_submit;
    const BHDrawResource *first,*second;Images images;BHVec3 eye={0,0,0},angles={0,0,0};
    unsigned long handles[2],mode,index,next,steps,model_commands,flare_commands,camera_commands;
    unsigned long prior,visible_flares;float depth,radius;int live=0;
    const char *names[]={"background-first.bmp","background-second.bmp","background-first-blank.bmp","background-second-blank.bmp"};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&models,0,sizeof(models));
    memset(&resources,0,sizeof(resources));memset(&prototype,0,sizeof(prototype));memset(&camera,0,sizeof(camera));
    memset(&native,0,sizeof(native));memset(&material,0,sizeof(material));memset(&state,0,sizeof(state));
    memset(&queue,0,sizeof(queue));memset(&images,0,sizeof(images));
    if(!archive)goto done;
    CHECK(bh_vfile_mount(archive),"real archive mounted as data");
    CHECK(bh_scene_view_init(&view,160,160),"offscreen source native target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL),"real texture context");if(!textures.device)goto done;live=1;
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    CHECK(bh_texture_formats(&textures) && bh_texture_set_path(&textures,"TEXTURES",2),"real texture formats and archive path");
    CHECK(bh_view_config_init(&view.projection,160,160) && bh_main_camera_settings(&view.projection,60,1,100000),"real projection settings");
    view.original_depth=1;
    CHECK(bh_clip_init(&clip,1,100000,0,160,0,160) && bh_screen_clip_init(&screen,1,100000,0,160,0,160),"real model and billboard clip owners");
    native.textures=&textures;native.near_z=1;
    prototype.camera=&camera;prototype.config=&view.projection;prototype.clip=&clip;
    prototype.texture_mask=0xffffffffUL;prototype.base_color=0xff000000UL;
    prototype.emit=bh_native_primitive_emit;prototype.emit_user=&native;
    CHECK(bh_model_context_init(&models,&textures,NULL) && bh_render_resources_init(&resources,&models,&prototype,NULL),"real model resource owner");
    handles[0]=bh_render_resources_load_model(&resources,"BSP\\M_SPSHIP.BSP");
    handles[1]=bh_render_resources_load_model(&resources,"BSP\\M_SPSH3.BSP");
    first=bh_render_resources_lookup(handles[0],&resources);second=bh_render_resources_lookup(handles[1],&resources);
    CHECK(handles[0] && handles[1] && first && second && first->tag==BH_RESOURCE_TAG_MODEL && second->tag==BH_RESOURCE_TAG_MODEL,
        "both actual startup background models loaded through recovered 00458140");
    if(!first || !second)goto done;
    images.white=bh_texture_load(&textures,"flare_wh.bmp");images.blue=bh_texture_load(&textures,"flare_bl.bmp");
    CHECK(images.white && images.blue,"both actual startup white and blue images resolve BMP to real PCX");if(!images.white || !images.blue)goto done;
    radius=first->data.model.instance->model->radius;
    if(second->data.model.instance->model->radius>radius)radius=second->data.model.instance->model->radius;
    depth=radius*6+200;
    memset(&draw,0,sizeof(draw));draw.camera=&camera;draw.config=&view.projection;
    submit.draw=&draw;submit.lookup=bh_render_resources_lookup;submit.lookup_user=&resources;
    memset(&billboard,0,sizeof(billboard));billboard.camera=&camera;billboard.config=&view.projection;
    billboard.clip=&screen;billboard.base_color=0xff000000UL;billboard.emit=bh_native_primitive_emit;billboard.emit_user=&native;
    billboard_submit.draw=&billboard;billboard_submit.lookup=image_lookup;billboard_submit.lookup_user=&images;
    bh_render_material_initialize(&material,&textures,view.d3d);
    CHECK(material.material && material.handle && !material.failures && bh_render_state_init(&state,&textures),"real native material and render state");
    CHECK(bh_draw_queue_init(&queue,32,NULL),"real command queue allocation");if(!queue.commands)goto done;
    for(mode=0;mode<4;++mode) {
        unsigned long selected=mode&1;int bound=mode<2;
        CHECK(bh_draw_queue_begin(&queue) && bh_scene_background_init(&background,&queue,
            bound?handles[0]:handles[0]+100,bound?handles[1]:handles[1]+100,bound?1:101,bound?2:102),"bind static background source state");
        background.slots[0].elapsed=4.966666666666667f;background.slots[0].duration=10;
        background.slots[0].height=0;background.slots[0].yaw=0;
        background.slots[0].start.x=-100;background.slots[0].start.y=0;background.slots[0].start.z=depth;
        background.slots[0].end.x=100;background.slots[0].end.y=0;background.slots[0].end.z=depth;
        background.slots[0].model=selected?background.second_model:background.first_model;
        srand(1);
        CHECK(bh_camera_queue_pose(&queue,&eye,&angles)==1 && bh_scene_background_step(&background,0,&eye)==1,
            "actual camera and complete background owner produce static scene commands");
        CHECK(queue.count==(selected?13:17),"camera, actual model and exact two/six flare records");
        CHECK(bh_scene_view_begin(&view) && bh_render_mode(&state,0),"begin native scene");
        prior=native.polygons;index=0;steps=model_commands=flare_commands=camera_commands=visible_flares=0;
        while(index!=BH_DRAW_END && steps++<queue.count) {
            BHDrawCommand *command;CHECK(index<queue.count,"bounded command chain");if(index>=queue.count)break;
            command=queue.commands+index;next=command->next;
            if(command->kind==BH_DRAW_CAMERA){CHECK(bh_camera_queue_execute(&view.projection,&camera,&queue,command,&next),"actual queued camera consumer");++camera_commands;}
            else if(command->kind==BH_DRAW_MODEL_RESOURCE){CHECK(bh_draw_submit_execute(&submit,command,&next)==bound,"real model consumer resolves actual binding or rejects absent resource");++model_commands;}
            else if(command->kind==BH_DRAW_BILLBOARD){CHECK(bh_draw_submit_billboard_execute(&billboard_submit,command,&next)==bound,"real flame consumer resolves actual image or rejects absent binding");if(bound)visible_flares+=billboard.emitted;++flare_commands;}
            else if(command->kind==BH_DRAW_STATE)CHECK(bh_render_mode(&state,command->words[0]),"real render state barrier");
            index=next;
        }
        CHECK(index==BH_DRAW_END && camera_commands==1 && model_commands==1 && flare_commands==(selected?2:6),"complete source queue traversal");
        CHECK(bound?native.polygons>prior:native.polygons==prior,"real resource primitives versus invalid-binding blank control");
        if(bound)CHECK(visible_flares>0 && (selected?second:first)->data.model.instance->polygons_emitted>0,"actual background model and actual flare image both emit");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,names[mode]),"flush source residency and read back raster");
        CHECK(bound?view.pixels>0:view.pixels==0,"real background pixels versus blank invalid-binding control");
    }
done:
    bh_draw_queue_dispose(&queue);bh_render_resources_dispose(&resources);bh_render_material_dispose(&material);
    if(live)bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);if(archive)bh_vfile_shutdown();
    CHECK(bh_math_environment_supported(),"source background preserves PC53");bh_math_environment_leave(&fp);
    if(count)*count=checks;
    printf("{\"scenario\":\"scene-background-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_SCENE_BACKGROUND_NATIVE_STANDALONE
int main(int argc,char **argv){unsigned long count=0;return bh_scene_background_native_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
