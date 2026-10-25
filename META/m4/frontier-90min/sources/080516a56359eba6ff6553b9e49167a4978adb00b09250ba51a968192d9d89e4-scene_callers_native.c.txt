#include "../src/bh_scene_owner.h"
#include "../src/bh_camera_queue.h"
#include "../src/bh_camera_state.h"
#include "../src/bh_player_camera.h"
#include "../src/bh_sound_queue.h"
#include "../src/bh_blocks.h"
#include "../src/bh_model_path.h"
#include "../src/bh_main_camera.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_draw_controls.h"
#include "../src/bh_draw_submit.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* This is a two-placement source scene from TEST.LEV. The pair is the first
   captured reciprocal X portal (placements 0 and 5797, model slot 129).
   Grid 10 is independently recorded in world_geometry_cases.h. No script
   resource flags or entity callbacks are inferred from the original EXE. */
static unsigned long checks;static int failures;
static void check(int yes,const char *what,int line) {
    ++checks;if(!yes){++failures;if(failures<24)printf("FAIL scene owner native %d: %s\n",line,what);}
}
#define CHECK(x,s) check((x)!=0,s,__LINE__)

static int drain(BHDrawQueue *queue,BHDrawSubmitContext *submit,
                 BHDrawControlContext *controls,BHRenderState *state,BHViewConfig *projection,BHMainCamera *camera,
                 unsigned long *models,unsigned long *clips) {
    unsigned long index=0,next,steps=0;*models=*clips=0;
    while(index!=BH_DRAW_END && steps++<queue->count) {
        const BHDrawCommand *command;
        if(index>=queue->count)return 0;
        command=queue->commands+index;next=command->next;
        switch(command->kind) {
        case BH_DRAW_NOP:break;
        case BH_DRAW_CAMERA:
            if(!bh_camera_queue_execute(projection,camera,queue,command,&next))return 0;
            break;
        case BH_DRAW_STATE:if(!bh_render_mode(state,command->words[0]))return 0;break;
        case BH_DRAW_MODEL_RESOURCE:
            if(!bh_draw_submit_execute(submit,command,&next))return 0;
            ++*models;break;
        case BH_DRAW_CLIP_RECT:
            if(!bh_draw_controls_execute(controls,queue,command,&next))return 0;
            ++*clips;break;
        case BH_DRAW_TEXTURE_MASK:
            if(!bh_draw_controls_execute(controls,queue,command,&next))return 0;
            break;
        default:return 0;
        }
        index=next;
    }
    return index==BH_DRAW_END;
}

int bh_scene_callers_native_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment fp;BHBlockLibrary library;BHSceneLevel level;BHVFile *file=NULL;
    BHGeometry geometry;BHGeometryModel model;BHWorldGeometry world[2];
    BHSceneBindingInput input[2];BHPickupRoom rooms[2];BHEntityScheduleRoom scheduled[2];
    BHSceneBindings bindings;BHSceneOwner owner;BHEntityScheduleContext scheduler;
    BHEntity player;BHPlayerCameraStart start;BHPlayerCameraContext reset;
    BHCameraState *camera_state;BHCameraStateContext camera_context;
    BHSoundQueue sound;BHSceneView view;BHTextureContext textures;BHModelContext models;
    BHRenderResources resources;BHDrawQueue queue;BHRenderMaterial material;
    BHRenderState state;BHNativePrimitive native;BHMainCamera camera;
    BHModelInstanceContext prototype;BHClipContext clip;BHScreenClipContext screen;
    BHAttributeClipContext attribute;BHDrawControlContext controls;
    BHDrawResourceContext draw;BHDrawSubmitContext submit;BHLightConfig light;
    BHVec3 eye,angles;unsigned long handle,rendered,rects,previous,frame;
    unsigned long mode;long model_index;int mounted=0,geometry_live=0,textures_live=0;
    const char *images[5]={"scene-callers-native-first.bmp",
        "scene-callers-native-cached.bmp","scene-callers-native-second.bmp",NULL,
        "scene-callers-native-after-gate.bmp"};
    checks=failures=0;
    memset(&sound,0,sizeof(sound));
    memset(&library,0,sizeof(library));memset(&level,0,sizeof(level));
    memset(&geometry,0,sizeof(geometry));memset(&model,0,sizeof(model));
    memset(world,0,sizeof(world));memset(input,0,sizeof(input));
    memset(rooms,0,sizeof(rooms));memset(scheduled,0,sizeof(scheduled));
    memset(&bindings,0,sizeof(bindings));memset(&owner,0,sizeof(owner));
    memset(&scheduler,0,sizeof(scheduler));memset(&view,0,sizeof(view));
    memset(&textures,0,sizeof(textures));memset(&resources,0,sizeof(resources));
    memset(&queue,0,sizeof(queue));memset(&material,0,sizeof(material));
    memset(&camera,0,sizeof(camera));memset(&native,0,sizeof(native));
    memset(&prototype,0,sizeof(prototype));memset(&light,0,sizeof(light));
    memset(&controls,0,sizeof(controls));
    if(!bh_math_environment_enter(&fp))return 2;
    bh_blocks_init(&library,NULL);bh_scene_level_init(&level,NULL);
    if(!archive)goto done;
    CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"real source listener owner");
    CHECK(bh_vfile_mount(archive),"real archive mounted");mounted=1;
    CHECK(bh_blocks_load(&library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY),"real block registry");
    if(library.count<=129)goto done;
    file=bh_vfile_open("TEST.LEV","rb");
    CHECK(file && bh_scene_level_read(&level,file),"real TEST.LEV placement records");
    if(file){bh_vfile_close(file);file=NULL;}
    if(level.placement_count<=5797)goto done;
    model_index=bh_blocks_find(&library,level.placements[0].name);
    CHECK(model_index==129 && !strcmp(level.placements[0].name,level.placements[5797].name),
          "captured adjacent placements resolve to the same real model");
    if(model_index!=129)goto done;
    CHECK(bh_geometry_init(&geometry,&library.blocks[model_index].bsp,NULL),"real mutable BSP geometry");
    geometry_live=1;
    CHECK(bh_geometry_model_init(&model,&geometry,10,6),"real portal faces and model bounds");
    if(!model.geometry)goto done;
    for(mode=0;mode<2;++mode) {
        const BHScenePlacement *p=&level.placements[mode?5797:0];
        world[mode].model=&model;world[mode].flags=p->flags;
        world[mode].query_flags=model.flags; /* scripts unapplied, as in the existing world fixture */
        world[mode].position.x=(float)p->x;world[mode].position.y=(float)p->y;
        world[mode].position.z=(float)p->z;
        world[mode].navigation.next=&world[1-mode].navigation;
        world[mode].previous=&world[1-mode];
    }
    CHECK(bh_world_geometry_build(world,2) && bh_world_geometry_build(world+1,2),
          "both real placements construct source bounds and apertures");
    CHECK(bh_portals_match(&world[0].navigation,2) &&
          bh_portals_match(&world[1].navigation,2),"source reciprocal portals match");
    CHECK(world[0].navigation.portals[0].neighbor==&world[1].navigation &&
          world[1].navigation.portals[1].neighbor==&world[0].navigation,
          "real two-room reciprocal portal identity");
    CHECK(bh_scene_view_init(&view,160,120) && bh_view_config_init(&view.projection,160,120),
          "native Direct3D2 offscreen target");
    if(!view.device)goto done;view.original_depth=1;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL),"native texture owner");
    if(!textures.device)goto done;textures_live=1;
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    CHECK(bh_texture_formats(&textures) && bh_texture_set_path(&textures,"TEXTURES",2),
          "real archive texture path and device formats");
    CHECK(bh_model_context_init(&models,&textures,NULL) &&
          bh_model_path_set(&models,"BSP\\",BH_BSP_PREFIX_ONLY),"real model owner/path");
    CHECK(bh_clip_init(&clip,1,100000,0,160,0,120) &&
          bh_screen_clip_init(&screen,1,100000,0,160,0,120) &&
          bh_attribute_clip_init(&attribute,1,100000,0,160,0,120),
          "real clipping consumers");
    native.textures=&textures;native.near_z=1;
    prototype.camera=&camera;prototype.config=&view.projection;prototype.clip=&clip;
    prototype.light_config=&light;prototype.texture_mask=0xffffffffUL;
    prototype.base_color=0xff000000UL;prototype.emit=bh_native_primitive_emit;
    prototype.emit_user=&native;
    CHECK(bh_render_resources_init(&resources,&models,&prototype,NULL),"source resource owner");
    handle=bh_render_resources_load_model(&resources,library.blocks[model_index].name);
    CHECK(handle && bh_render_resources_lookup(handle,&resources),"actual room BSP model resource loaded");
    if(!handle)goto done;
    textures.error=BH_TEXTURE_OK;
    bh_render_material_initialize(&material,&textures,view.d3d);
    CHECK(material.material && material.handle && !material.failures &&
          bh_render_state_init(&state,&textures),"native material and render state");
    CHECK(bh_draw_queue_init(&queue,1024,NULL),"real draw queue allocation");
    if(!queue.commands)goto done;
    scheduler.queue=&queue;scheduler.viewport=&view.projection;
    scheduler.camera=&owner.camera;scheduler.max_rooms=2;
    scheduler.overlay_position=&eye; /* explicit player-space binding for overlay branch */
    scheduler.max_entities=1;scheduler.max_depth=2;
    for(mode=0;mode<2;++mode) {
        input[mode].world=&world[mode];input[mode].resource=handle;
        input[mode].resource_flags=0; /* block scripts still unapplied */
        input[mode].angles.x=(float)((level.placements[mode?5797:0].flags&3UL)*1.5707963267948966);
    }
    CHECK(bh_scene_bindings_init(&bindings,input,2,rooms,scheduled,&resources,&scheduler),
          "explicit borrowed world/resource binding");
    CHECK(bh_scene_owner_init(&owner,&bindings,160,120),"recovered outer scene owner initialized");
    controls.view=&view.projection;controls.resources=&resources;
    controls.model_clip=&clip;controls.screen_clip=&screen;
    controls.attribute_clip=&attribute;controls.texture_mask=0xffffffffUL;
    draw.camera=&camera;draw.config=&view.projection;
    submit.draw=&draw;submit.lookup=bh_render_resources_lookup;submit.lookup_user=&resources;
    angles.x=angles.y=angles.z=0;
    for(mode=0;mode<5;++mode) {
        eye.x=mode==2?-870:-970;eye.y=-60;eye.z=1460;
        angles.x=mode==2?4.71238898038469f:1.5707963267948966f;
        owner.mode=mode==3?1:0;
        CHECK(bh_draw_queue_begin(&queue),"queue begins each pass");
        if(mode==3) {
            frame=owner.frame;
            CHECK(!bh_scene_owner_step(&owner,&eye,&angles,90,1,100000,160,120) &&
                  bindings.cached==NULL && owner.frame==frame,
                  "scene gate clears cached room without scheduling");
            CHECK(queue.count==9,"gated pass leaves queue headers only");
            continue;
        }
        memset(&player,0,sizeof(player));memset(&start,0,sizeof(start));
        memset(&reset,0,sizeof(reset));memset(&camera_context,0,sizeof(camera_context));
        start.position=eye;start.position.y+=2.5f;
        start.position.x+=(float)(7.5*sin((double)angles.x));
        start.position.z+=(float)(7.5*cos((double)angles.x));start.angles=angles;
        player.position=start.position;camera_state=bh_player_camera_state(&player);
        reset.player=&player;reset.starts=&start;reset.start_count=1;reset.player_count=1;
        reset.camera=&camera_context;
        CHECK(bh_camera_state_init(camera_state,1) && bh_player_camera_reset(&reset,0),
              "actual player reset and fifty source camera settling updates");
        CHECK(fabs((double)camera_state->position.x-eye.x)<.01 &&
              fabs((double)camera_state->position.y-eye.y)<.01 &&
              fabs((double)camera_state->position.z-eye.z)<.01,
              "source player reset reconstructs the static chase camera eye");
        CHECK(bh_sound_listener(&sound,0,1,&camera_state->position,&camera_state->angles,&camera_state->velocity),
              "shared caller listener consumes recovered state pose and velocity");
        CHECK(bh_camera_queue_pose(&queue,&camera_state->position,&camera_state->angles)==1,
              "shared caller queues camera before scene");
        CHECK(bh_scene_view_begin(&view) && bh_render_mode(&state,0),"native target starts clean");
        CHECK(bh_scene_owner_step(&owner,&eye,&angles,90,1,100000,160,120),
              "actual 00409130 owner locates and schedules scene");
        CHECK(bindings.cached==&world[mode==2?1:0],
              "cached room is reused then changes with eye");
        CHECK(queue.count>11,"owner queued room geometry and target resets");
        CHECK(scheduled[0].last_frame==owner.frame &&
              scheduled[1].last_frame==owner.frame,
              "real portal recursion reached both room resources");
        CHECK(bh_main_camera_settings(&view.projection,90,1,100000),
              "projection settings bound before queue consumption");
        previous=native.polygons;
        CHECK(drain(&queue,&submit,&controls,&state,&view.projection,&camera,&rendered,&rects),
              "full recovered queue consumed through native resource and control owners");
        CHECK(fabs((double)camera.position.x-eye.x)<.01 &&
              fabs((double)camera.position.y-eye.y)<.01 &&
              fabs((double)camera.position.z-eye.z)<.01 &&
              fabs((double)sound.listener.x-eye.x)<.01 &&
              fabs((double)sound.listener.y-eye.y)<.01 && fabs((double)sound.listener.z-eye.z)<.01,
              "actual queued render camera and immediate listener share caller eye");
        CHECK(rendered>=2 && rects>=4,"both portal geometry and full target resets consumed");
        CHECK(view.projection.left==0 && view.projection.top==0 &&
              view.projection.right==160 && view.projection.bottom==120 &&
              clip.left==0 && clip.right==160 && screen.top==0 &&
              attribute.bottom==120,"full target clipping restored after portal draw");
        CHECK(native.polygons>previous,"real model resource emitted native polygons");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,images[mode]),
              "native target flush and readback");
        CHECK(view.pixels>0,"actual room produces nonblack pixels");
    }
    world[0].query_flags=world[1].query_flags=0;
    CHECK(bh_draw_queue_begin(&queue),"blank control queue begins");
    eye.x=-970;eye.y=-60;eye.z=1460;angles.x=1.5707963267948966f;
    CHECK(bh_scene_view_begin(&view) && bh_scene_owner_step(&owner,&eye,&angles,90,1,100000,160,120),
          "source owner runs with world render eligibility disabled");
    CHECK(drain(&queue,&submit,&controls,&state,&view.projection,&camera,&rendered,&rects) && rendered==0,
          "disabled world emits no model resources");
    CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,"scene-callers-native-blank.bmp") &&
          view.pixels==0,"disabled world is an actually blank native target");
done:
    bh_sound_queue_dispose(&sound);
    bh_scene_bindings_dispose(&bindings);bh_draw_queue_dispose(&queue);
    bh_render_resources_dispose(&resources);bh_render_material_dispose(&material);
    if(textures_live)bh_texture_context_dispose(&textures);
    bh_scene_view_dispose(&view);
    if(geometry_live)bh_geometry_dispose(&geometry);
    if(file)bh_vfile_close(file);bh_scene_level_dispose(&level);
    bh_blocks_dispose(&library);if(mounted)bh_vfile_shutdown();
    CHECK(bh_math_environment_supported(),"PC53 environment retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"scene-callers-native\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_SCENE_CALLERS_NATIVE_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_scene_callers_native_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
