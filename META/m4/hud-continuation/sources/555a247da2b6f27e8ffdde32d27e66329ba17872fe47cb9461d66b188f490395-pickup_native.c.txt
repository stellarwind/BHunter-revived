#include "../src/bh_render_resources.h"
#include "../src/bh_pickup_draw.h"
#include "../src/bh_entity_schedule.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks;static int failures;
#define CHECK(x,s) check((x)!=0,s,__LINE__)
static void check(int ok,const char *name,int line){
    ++checks;if(!ok){++failures;if(failures<16)printf("FAIL pickup native %d: %s\n",line,name);}
}
typedef struct PickupImages {BHTextureImage *glow,*atlas;} PickupImages;
static BHTextureImage *__cdecl image_lookup(unsigned long handle,void *user){
    PickupImages *images=(PickupImages *)user;
    if(handle==77)return images->glow;
    if(handle==78)return images->atlas;
    return NULL;
}
int bh_pickup_native_checkpoint(const char *archive,unsigned long *count){
    BHMathEnvironment fp;BHSceneView view;BHTextureContext textures;
    BHModelContext models;BHRenderResources resources;BHMainCamera camera;
    BHViewCamera schedule_camera;BHClipContext clip;BHModelInstanceContext prototype;
    BHDrawResourceContext draw;BHDrawSubmitContext submit;BHRenderMaterial material;
    BHRenderState state;BHNativePrimitive native;BHLightConfig light;
    BHSpriteBillboardContext billboard;BHScreenClipContext screen;
    BHDrawBillboardSubmitContext billboard_submit;PickupImages images;
    BHPickupDrawContext pickup_draw;BHEntityResourceDrawContext special_draw;
    BHPickupRenderBinding binding;BHPickupContext pickup_owner;
    BHEntityPool pool;BHEntity *pickup;BHWorldGeometry world;BHPickupRoom room;
    BHEntityScheduleRoom scheduled;BHEntityScheduleContext scheduler;BHDrawQueue queue;
    BHVec3 point;const BHDrawResource *resource;unsigned long table[10];
    unsigned long handle,index,next,steps,models_drawn,billboards,rectangles,mode,prior;
    int mounted=0,textures_live=0;
    const char *names[]={"pickup-native-active.bmp","pickup-native-inactive.bmp"};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));
    memset(&resources,0,sizeof(resources));memset(&material,0,sizeof(material));
    memset(&pool,0,sizeof(pool));memset(&queue,0,sizeof(queue));
    memset(&camera,0,sizeof(camera));memset(&schedule_camera,0,sizeof(schedule_camera));
    memset(&light,0,sizeof(light));memset(&images,0,sizeof(images));
    memset(&pickup_owner,0,sizeof(pickup_owner));memset(&world,0,sizeof(world));
    memset(&room,0,sizeof(room));memset(&scheduled,0,sizeof(scheduled));
    memset(&scheduler,0,sizeof(scheduler));memset(table,0,sizeof(table));
    if(!archive)goto done;
    CHECK(bh_vfile_mount(archive),"real archive mounted");mounted=1;
    CHECK(bh_scene_view_init(&view,96,96),"native offscreen target");
    if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL),"native texture context");
    if(!textures.device)goto done;textures_live=1;
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    CHECK(bh_texture_formats(&textures) && bh_texture_set_path(&textures,"TEXTURES",2),
          "real texture formats and path");
    CHECK(bh_view_config_init(&view.projection,96,96) &&
          bh_main_camera_settings(&view.projection,60,1,100000) &&
          bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) &&
          bh_view_camera_build(&view.projection,&camera.view,&schedule_camera),
          "recovered main and schedule cameras");
    view.original_depth=1;
    CHECK(bh_clip_init(&clip,1,100000,0,96,0,96),"model clipper");
    memset(&prototype,0,sizeof(prototype));memset(&native,0,sizeof(native));
    native.textures=&textures;native.near_z=1;
    prototype.camera=&camera;prototype.config=&view.projection;
    prototype.clip=&clip;prototype.light_config=&light;
    prototype.texture_mask=0xffffffffUL;prototype.base_color=0xff000000UL;
    prototype.emit=bh_native_primitive_emit;prototype.emit_user=&native;
    CHECK(bh_model_context_init(&models,&textures,NULL) &&
          bh_render_resources_init(&resources,&models,&prototype,NULL),
          "source-owned model resource construction");
    handle=bh_render_resources_load_model(&resources,"BSP\\PU_9A.BSP");
    resource=bh_render_resources_lookup(handle,&resources);
    CHECK(handle && resource && resource->tag==BH_RESOURCE_TAG_MODEL,
          "real PU_9A model resource");if(!resource)goto done;
    images.glow=bh_texture_load(&textures,"flare_yl.bmp");
    images.atlas=bh_texture_load(&textures,"clipbrd.bmp");
    CHECK(images.glow && images.atlas,"original pickup glow and numeric atlas images");
    if(!images.glow || !images.atlas)goto done;
    CHECK(bh_screen_clip_init(&screen,1,100000,0,96,0,96),"billboard/rect clipper");
    memset(&billboard,0,sizeof(billboard));billboard.camera=&camera;
    billboard.config=&view.projection;billboard.clip=&screen;
    billboard.base_color=0xff000000UL;billboard.emit=bh_native_primitive_emit;
    billboard.emit_user=&native;
    billboard_submit.draw=&billboard;billboard_submit.lookup=image_lookup;
    billboard_submit.lookup_user=&images;
    bh_render_material_initialize(&material,&textures,view.d3d);
    CHECK(material.material && material.handle && !material.failures &&
          bh_render_state_init(&state,&textures),"native material/state owners");
    CHECK(bh_entities_init(&pool,4,NULL) && bh_draw_queue_init(&queue,128,NULL),
          "real entity/queue allocation owners");
    memset(&draw,0,sizeof(draw));draw.camera=&camera;draw.config=&view.projection;
    submit.draw=&draw;submit.lookup=bh_render_resources_lookup;submit.lookup_user=&resources;
    memset(&pickup_draw,0,sizeof(pickup_draw));
    pickup_draw.queue=&queue;pickup_draw.camera=&camera;
    pickup_draw.view=&view.projection;pickup_draw.clip=&screen;
    pickup_draw.emit=bh_native_primitive_emit;pickup_draw.emit_user=&native;
    pickup_draw.atlas_image=78;pickup_draw.glow_image=77;pickup_draw.display_depth=32;
    special_draw.queue=&queue;special_draw.glow_image=77;
    binding.default_draw=&pickup_draw;binding.special_draw=&special_draw;
    world.navigation.next=(BHPortalNode *)&world;
    world.minimum.x=world.minimum.y=world.minimum.z=-1000;
    world.maximum.x=world.maximum.y=world.maximum.z=1000;
    world.query_flags=4;room.world=&world;
    pickup_owner.pool=&pool;pickup_owner.first_world=&world;
    pickup_owner.world_count=1;pickup_owner.rooms=&room;pickup_owner.room_count=1;
    table[0]=handle;pickup_owner.resources=table;pickup_owner.resource_count=10;
    CHECK(bh_pickup_render_bind(&pickup_owner,&binding),"real draw bridges installed");
    point.x=point.y=0;point.z=20;
    pickup=bh_pickup_spawn(&pickup_owner,&point,0,0,3,3,7,0);
    CHECK(pickup && room.pickups==pickup && bh_pickup_state(pickup)->room==&world,
          "constructor locates room and installs pickup child");
    if(!pickup)goto done;
    scheduled.room=&room;scheduled.resource=handle;
    scheduler.queue=&queue;scheduler.viewport=&view.projection;
    scheduler.camera=&schedule_camera;scheduler.frame=1;
    scheduler.max_rooms=4;scheduler.max_entities=8;scheduler.max_depth=4;
    for(mode=0;mode<2;++mode){
        world.query_flags=mode?0:4;scheduler.frame++;
        CHECK(bh_draw_queue_begin(&queue),"begin pickup queue");
        CHECK(bh_entity_schedule(&scheduler,&scheduled),
              "source room schedules constructor-created pickup");
        CHECK(bh_scene_view_begin(&view) && bh_render_mode(&state,0),
              "begin native pickup scene");
        prior=native.polygons;index=0;steps=models_drawn=billboards=rectangles=0;
        while(index!=BH_DRAW_END && steps++<queue.count){BHDrawCommand *command;
            CHECK(index<queue.count,"bounded queue chain");
            if(index>=queue.count)break;command=queue.commands+index;next=command->next;
            if(command->kind==BH_DRAW_MODEL_RESOURCE){
                CHECK(bh_draw_submit_execute(&submit,command,&next),"real model consumer");
                ++models_drawn;
            }else if(command->kind==BH_DRAW_BILLBOARD){
                CHECK(bh_draw_submit_billboard_execute(&billboard_submit,command,&next),
                      "real glow consumer");++billboards;
            }else if(command->kind==BH_DRAW_RECT){
                CHECK(bh_pickup_rect_execute(&pickup_draw,image_lookup,&images,command,&next),
                      "real atlas rectangle consumer");++rectangles;
            }else if(command->kind==BH_DRAW_STATE)
                CHECK(bh_render_mode(&state,command->words[0]),"real render state");
            index=next;
        }
        CHECK(index==BH_DRAW_END,"entire scheduled queue consumed");
        CHECK(mode?(!models_drawn && !billboards && !rectangles):
              (models_drawn>=2 && billboards==1 && rectangles>0),
              "active pickup model/glow/numeric records and inactive control");
        CHECK(mode?native.polygons==prior:native.polygons>prior,
              "native primitive submission follows room active flag");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,names[mode]),
              "native flush and bitmap readback");
        CHECK(mode?!view.pixels:view.pixels>0,
              "active source pickup pixels and inactive blank control");
    }
done:
    bh_draw_queue_dispose(&queue);bh_entities_dispose(&pool);
    bh_render_resources_dispose(&resources);bh_render_material_dispose(&material);
    if(textures_live)bh_texture_context_dispose(&textures);
    bh_scene_view_dispose(&view);if(mounted)bh_vfile_shutdown();
    CHECK(bh_math_environment_supported(),"PC53 retained");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"pickup-native\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_PICKUP_NATIVE_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_pickup_native_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
