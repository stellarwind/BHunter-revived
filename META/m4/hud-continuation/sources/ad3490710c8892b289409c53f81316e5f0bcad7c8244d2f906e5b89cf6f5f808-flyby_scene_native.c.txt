#include "../src/bh_flyby_scene.h"
#include "../src/bh_camera_queue.h"
#include "../src/bh_horizon_queue.h"
#include "../src/bh_horizon_color.h"
#include "../src/bh_pickup_draw.h"
#include "../src/bh_queue_reserve.h"
#include "../src/bh_draw_controls.h"
#include "../src/bh_draw_submit.h"
#include "../src/bh_model_path.h"
#include "../src/bh_blocks.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL flyby-scene-native: %s\n",s);}}while(0)
typedef struct Images {BHTextureImage *items[5];} Images;
static BHTextureImage *__cdecl image(unsigned long handle,void *raw) {
    Images *images=(Images *)raw;return handle<5?images->items[handle]:NULL;
}
static BHTextureImage *solid(BHTextureContext *textures,const char *name,
                              int red,int green,int blue) {
    BHPcx pcx;unsigned char pixels[1024];
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));
    pcx.width=pcx.height=32;pcx.pixels=pixels;pcx.pixel_bytes=1024;
    pcx.palette[3]=(unsigned char)red;pcx.palette[4]=(unsigned char)green;
    pcx.palette[5]=(unsigned char)blue;
    return bh_texture_stage(textures,name,&pcx,0,0);
}
static int equal_files(const char *a,const char *b) {
    FILE *fa=fopen(a,"rb"),*fb=fopen(b,"rb");int x,y;
    if(!fa || !fb){if(fa)fclose(fa);if(fb)fclose(fb);return 0;}
    do{x=fgetc(fa);y=fgetc(fb);if(x!=y){fclose(fa);fclose(fb);return 0;}}
    while(x!=EOF);fclose(fa);fclose(fb);return 1;
}
typedef struct World {
    BHBlockLibrary library;BHSceneLevel level;BHGeometry geometry;
    BHGeometryModel model;BHWorldGeometry nodes[2];int geometry_live,mounted;
} World;
static int world_open(World *w,const char *archive) {
    unsigned long i;BHVFile *file=0;long index;
    memset(w,0,sizeof(*w));bh_blocks_init(&w->library,0);bh_scene_level_init(&w->level,0);
    if(!archive)return 1;
    CHECK(bh_vfile_mount(archive),"original archive mounted only as immutable asset data");
    w->mounted=1;
    CHECK(bh_blocks_load(&w->library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY),
          "real block registry");
    file=bh_vfile_open("TEST.LEV","rb");
    CHECK(file && bh_scene_level_read(&w->level,file),"real TEST.LEV source placements");
    if(file)bh_vfile_close(file);
    if(w->level.placement_count<=5797)return 0;
    index=bh_blocks_find(&w->library,w->level.placements[0].name);
    CHECK(index==129 && !strcmp(w->level.placements[0].name,w->level.placements[5797].name),
          "established two-room asset identity");
    if(index!=129)return 0;
    CHECK(bh_geometry_init(&w->geometry,&w->library.blocks[index].bsp,0),
          "real mutable BSP geometry");w->geometry_live=1;
    CHECK(bh_geometry_model_init(&w->model,&w->geometry,10,6),
          "real portal and model bounds");
    if(!w->model.geometry)return 0;
    for(i=0;i<2;++i){const BHScenePlacement *p=w->level.placements+(i?5797:0);
        w->nodes[i].model=&w->model;w->nodes[i].flags=p->flags;
        w->nodes[i].query_flags=w->model.flags;
        w->nodes[i].position.x=(float)p->x;w->nodes[i].position.y=(float)p->y;
        w->nodes[i].position.z=(float)p->z;
        w->nodes[i].navigation.next=&w->nodes[1-i].navigation;
        w->nodes[i].previous=&w->nodes[1-i];}
    CHECK(bh_world_geometry_build(w->nodes,2) &&
          bh_world_geometry_build(w->nodes+1,2) &&
          bh_portals_match(&w->nodes[0].navigation,2) &&
          bh_portals_match(&w->nodes[1].navigation,2),
          "source reciprocal portal topology from actual BSP");
    return w->nodes[0].navigation.portals[0].neighbor==&w->nodes[1].navigation;
}
static void world_close(World *w) {
    if(w->geometry_live)bh_geometry_dispose(&w->geometry);
    bh_scene_level_dispose(&w->level);bh_blocks_dispose(&w->library);
    if(w->mounted)bh_vfile_shutdown();
}
typedef struct Consumer {
    BHDrawQueue *queue;BHViewConfig *view;BHMainCamera *camera;
    BHRenderState *state;Images *images;
    BHHorizonQueueContext horizon;BHPickupDrawContext rect;
    BHDrawSubmitContext model;BHDrawControlContext controls;
    unsigned long kinds[17];
} Consumer;
static int consume(Consumer *n) {
    unsigned long at=0,next,steps=0;BHDrawQueue *queue=n->queue;
    memset(n->kinds,0,sizeof(n->kinds));
    while(at!=BH_DRAW_END && steps++<queue->count){
        BHDrawCommand *command;
        if(at>=queue->count)return 0;
        command=queue->commands+at;next=command->next;
        if(command->kind>=17)return 0;++n->kinds[command->kind];
        switch(command->kind){
        case BH_DRAW_NOP:break;
        case BH_DRAW_STATE:
            if(!bh_queue_mode_execute(n->state,command,&next))return 0;break;
        case BH_DRAW_CAMERA:
            if(!bh_camera_queue_execute(n->view,n->camera,queue,command,&next))return 0;break;
        case BH_DRAW_HORIZON:
            if(!bh_horizon_queue_execute(&n->horizon,queue,command,&next))return 0;break;
        case BH_DRAW_MODEL_RESOURCE:
            if(!bh_draw_submit_execute(&n->model,command,&next))return 0;break;
        case BH_DRAW_CLIP_RECT:case BH_DRAW_TEXTURE_MASK:
            if(!bh_draw_controls_execute(&n->controls,queue,command,&next))return 0;break;
        case BH_DRAW_RECT:
            if(!bh_pickup_rect_execute(&n->rect,image,n->images,command,&next))return 0;break;
        default:return 0;
        }
        at=next;
    }
    return at==BH_DRAW_END;
}
static void vector_put(BHEntity *entity,unsigned long offset,BHVec3 value) {
    memcpy((unsigned char *)entity+offset,&value,12);
}

/* Two bounded source callback invocations with native queue drain/readback.
   There is no game startup, game frame loop, input device or original code. */
int bh_flyby_scene_native_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment fp;World topology;BHSceneView view;BHTextureContext textures;
    BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;
    BHDrawQueue queue;BHMainCamera camera;BHScreenClipContext screen_clip;
    BHClipContext clip;BHAttributeClipContext attribute;BHHorizonContext horizon;
    BHHorizonNative horizon_native;BHLightConfig light;Images images;Consumer consumer;
    BHModelContext models;BHRenderResources resources;BHModelInstanceContext prototype;
    BHDrawResourceContext resource_draw;BHSceneBindings bindings;BHSceneOwner owner;
    BHSceneBindingInput inputs[2];BHPickupRoom rooms[2];BHEntityScheduleRoom scheduled[2];
    BHEntityScheduleContext scheduler;BHEntityPool entities;BHEntity *flyby;
    BHSoundQueue sound;BHSceneBackground background;BHFlybyScene scene;
    BHPlayerCameraStart starts[23];BHVec3 eye,angles,velocity;
    long modes=0,selected=0,width=320,height=240;float near_z=.01f;
    unsigned long handle=0,phase,prior,pixels[2]={0,0},states[2]={0,0};
    int world_live=0,entities_live=0;
    const char *outputs[2]={"flyby-scene-native-corners.bmp","flyby-scene-native-full.bmp"};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&topology,0,sizeof(topology));memset(&view,0,sizeof(view));
    memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));
    memset(&state,0,sizeof(state));memset(&native,0,sizeof(native));
    memset(&queue,0,sizeof(queue));memset(&camera,0,sizeof(camera));
    memset(&screen_clip,0,sizeof(screen_clip));memset(&clip,0,sizeof(clip));
    memset(&attribute,0,sizeof(attribute));memset(&horizon,0,sizeof(horizon));
    memset(&horizon_native,0,sizeof(horizon_native));memset(&light,0,sizeof(light));
    memset(&images,0,sizeof(images));memset(&consumer,0,sizeof(consumer));
    memset(&models,0,sizeof(models));memset(&resources,0,sizeof(resources));
    memset(&prototype,0,sizeof(prototype));memset(&resource_draw,0,sizeof(resource_draw));
    memset(&bindings,0,sizeof(bindings));memset(&owner,0,sizeof(owner));
    memset(&scheduler,0,sizeof(scheduler));memset(&entities,0,sizeof(entities));
    memset(&sound,0,sizeof(sound));memset(&background,0,sizeof(background));
    memset(&scene,0,sizeof(scene));memset(starts,0,sizeof(starts));
    CHECK(world_open(&topology,archive),"source world fixture opens");world_live=1;
    CHECK(bh_scene_view_init(&view,320,240),"actual offscreen Direct3D2 target");
    if(!view.device)goto done;view.original_depth=1;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,0) &&
          bh_texture_formats(&textures),"real texture owner and device formats");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    if(archive){
        textures.palette_mode=2;
        CHECK(bh_texture_set_path(&textures,"textures",2),"original archive texture path");
        images.items[1]=bh_texture_load(&textures,"bluesky.bmp");
        images.items[2]=bh_texture_load(&textures,"radar.bmp");
        images.items[3]=bh_texture_load(&textures,"clipbrd.bmp");
        images.items[4]=bh_texture_load(&textures,"dots.bmp");
        CHECK(images.items[1] && images.items[2] && images.items[3] && images.items[4],
              "actual startup-named image resources");
    }else{
        images.items[1]=solid(&textures,"FB-SKY.PCX",0,0,255);
        images.items[2]=solid(&textures,"FB-FULL.PCX",255,0,0);
        images.items[3]=solid(&textures,"FB-LEFT.PCX",0,255,0);
        images.items[4]=solid(&textures,"FB-RIGHT.PCX",255,255,0);
        CHECK(images.items[1] && images.items[2] && images.items[3] && images.items[4],
              "isolated native source textures");
    }
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) &&
          !material.failures && bh_render_state_init(&state,&textures),
          "native material and state owners");
    CHECK(bh_view_config_init(&view.projection,320,240) &&
          bh_main_camera_settings(&view.projection,60,.01f,25000) &&
          bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0),
          "source camera projection");
    CHECK(bh_clip_init(&clip,.01f,25000,0,320,0,240) &&
          bh_clip_set_camera(&clip,&camera.projected) &&
          bh_screen_clip_init(&screen_clip,.01f,25000,0,320,0,240) &&
          bh_attribute_clip_init(&attribute,.01f,25000,0,320,0,240),
          "native three-clip source owners");
    native.textures=&textures;native.near_z=.01f;
    horizon_native.textures=&textures;horizon_native.near_z=.01f;
    CHECK(bh_light_configure(&light,0,0,0,0,0,0,0,0),"disabled source light");
    CHECK(bh_horizon_init(&horizon,&camera,&attribute,&state,0,
          bh_horizon_native_emit,&horizon_native),"native horizon/gradient owner");
    CHECK(bh_draw_queue_init(&queue,4096,0),"real source draw queue");
    consumer.queue=&queue;consumer.view=&view.projection;consumer.camera=&camera;
    consumer.state=&state;consumer.images=&images;
    consumer.horizon.draw=&horizon;consumer.horizon.lookup=image;
    consumer.horizon.lookup_user=&images;
    consumer.rect.queue=&queue;consumer.rect.view=&view.projection;
    consumer.rect.camera=&camera;consumer.rect.clip=&screen_clip;
    consumer.rect.emit=bh_native_primitive_emit;consumer.rect.emit_user=&native;
    CHECK(bh_model_context_init(&models,&textures,0) &&
          bh_model_path_set(&models,"BSP\\",BH_BSP_PREFIX_ONLY),
          "real model owner and block path");
    prototype.camera=&camera;prototype.config=&view.projection;prototype.clip=&clip;
    prototype.light_config=&light;prototype.texture_mask=0xffffffffUL;
    prototype.base_color=0xff000000UL;prototype.emit=bh_native_primitive_emit;
    prototype.emit_user=&native;
    CHECK(bh_render_resources_init(&resources,&models,&prototype,0),
          "real model resource owner");
    if(archive){unsigned long i;
        CHECK(bh_texture_policy_set(&textures,1,0xff00ffUL),
              "observed later startup magenta-key model policy");
        handle=bh_render_resources_load_model(&resources,topology.library.blocks[129].name);
        CHECK(handle!=0,"actual room BSP source resource");
        scheduler.queue=&queue;scheduler.camera=&owner.camera;
        scheduler.viewport=&view.projection;scheduler.max_rooms=2;
        scheduler.max_entities=1;scheduler.max_depth=2;
        scheduler.overlay_position=&eye;
        for(i=0;i<2;++i){
            inputs[i].world=topology.nodes+i;inputs[i].resource=handle;
            inputs[i].angles.x=(float)((topology.level.placements[i?5797:0].flags&3UL)*1.5707963267948966);
        }
        CHECK(bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,
              &resources,&scheduler),"actual two-room scene bindings");
    }
    CHECK(bh_scene_owner_init(&owner,&bindings,320,240),
          "source scene owner initialized");
    resource_draw.camera=&camera;resource_draw.config=&view.projection;
    consumer.model.draw=&resource_draw;
    consumer.model.lookup=bh_render_resources_lookup;
    consumer.model.lookup_user=&resources;
    consumer.controls.view=&view.projection;
    consumer.controls.resources=&resources;
    consumer.controls.model_clip=&clip;
    consumer.controls.screen_clip=&screen_clip;
    consumer.controls.attribute_clip=&attribute;
    consumer.controls.texture_mask=0xffffffffUL;
    CHECK(bh_sound_queue_init(&sound,0,0,0) &&
          bh_entities_init(&entities,2,0) &&
          bh_scene_background_init(&background,&queue,handle,handle,4,4),
          "real listener pool and background owners");entities_live=1;
    flyby=bh_entity_acquire(&entities);CHECK(flyby!=0,"live source camera entity");
    if(!flyby)goto done;
    eye.x=archive?-970:0;eye.y=archive?-60:0;eye.z=archive?1460:0;
    angles.x=archive?1.5707963267948966f:0;angles.y=angles.z=0;
    velocity.x=velocity.y=velocity.z=0;
    vector_put(flyby,0x160,eye);vector_put(flyby,0x16c,angles);
    vector_put(flyby,0x178,velocity);
    starts[0].position=eye;
    scene.camera_entity=flyby;scene.entities=&entities;scene.queue=&queue;
    scene.sound=&sound;scene.background=&background;scene.scene=&owner;
    scene.starts=starts;scene.start_count=23;scene.mode_count=&modes;
    scene.selected_waypoint=&selected;scene.display_width=&width;
    scene.display_height=&height;scene.near_z=&near_z;
    scene.horizon_image=1;scene.full_image=2;
    scene.corner_left_image=3;scene.corner_right_image=4;
    if(archive){
        background.slots[0].elapsed=.5f;background.slots[0].duration=10;
        background.slots[0].delay=0;background.slots[0].model=handle;
    }
    CHECK(bh_texture_preallocate(&textures),"actual texture residency slots");
    for(phase=0;phase<2;++phase){unsigned long i;
        selected=phase?17:0;
        for(i=1;i<8;++i){background.slots[i].elapsed=0;background.slots[i].delay=100;}
        CHECK(bh_draw_queue_begin(&queue),"source frame queue reset");
        prior=native.polygons;
        CHECK(bh_flyby_scene_draw(&scene),"whole source flyby scene callback");
        CHECK(sound.listener_active==1 && scene.error==BH_FLYBY_SCENE_OK,
              "complete callback updates actual source listener");
        CHECK(bh_scene_view_begin(&view) && consume(&consumer),
              "all queued kinds drain through actual native source consumers");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,outputs[phase]),
              "offscreen Direct3D2 flush and bitmap readback");
        pixels[phase]=view.pixels;
        CHECK(view.pixels>100 && native.polygons>prior,
              "scene callback produces real native pixels and polygons");
        states[phase]=consumer.kinds[BH_DRAW_STATE];
        CHECK(consumer.kinds[BH_DRAW_CAMERA]==1 &&
              consumer.kinds[BH_DRAW_HORIZON]==1 &&
              consumer.kinds[BH_DRAW_RECT]==(phase?1:2) &&
              states[phase]>=(phase?6:7),
              "camera horizon and correct UV image branch all execute");
        if(archive)CHECK(consumer.kinds[BH_DRAW_MODEL_RESOURCE]>=(phase?2:3),
                         "real world rooms and background model execute");
    }
    CHECK(states[0]==states[1]+1 &&
          !equal_files(outputs[0],outputs[1]) && pixels[0]>0 && pixels[1]>0,
          "corner and full-image native readbacks independently differ");
done:
    bh_sound_queue_dispose(&sound);
    if(entities_live)bh_entities_dispose(&entities);
    bh_draw_queue_dispose(&queue);bh_scene_bindings_dispose(&bindings);
    bh_render_resources_dispose(&resources);bh_render_material_dispose(&material);
    bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    if(world_live)world_close(&topology);
    CHECK(bh_math_environment_supported(),"native composition preserves PC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"flyby-scene-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_STANDALONE
int main(int argc,char **argv) {
    unsigned long count=0;
    return bh_flyby_scene_native_checkpoint(argc>1?argv[1]:0,&count);
}
#endif
