#include "../src/bh_player_scene.h"
#include "../src/bh_camera_queue.h"
#include "../src/bh_horizon_queue.h"
#include "../src/bh_material_queue.h"
#include "../src/bh_native_attribute.h"
#include "../src/bh_segment_queue.h"
#include "../src/bh_path_queue.h"
#include "../src/bh_atlas_glyph.h"
#include "../src/bh_atlas_text.h"
#include "../src/bh_model_path.h"
#include "../src/bh_draw_controls.h"
#include <stdlib.h>
#include "../src/bh_hud.h"
#include "../src/bh_hud_markers.h"
#include "../src/bh_hud_world.h"
#include "../src/bh_hud_flat.h"
#include "../src/bh_hud_scores.h"
#include "../src/bh_plane_quad.h"
#include "../src/bh_queue_reserve.h"
#include "../src/bh_blocks.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL player-scene-native: %s\n",s);}}while(0)
typedef struct HudImages {BHTextureImage *items[20];} HudImages;
static BHTextureImage *__cdecl image(unsigned long handle,void *raw) {
    HudImages *images=(HudImages *)raw;
    return handle<20?images->items[handle]:NULL;
}
static BHTextureImage *solid(BHTextureContext *textures,const char *name,int red,int green,int blue) {
    BHPcx pcx;unsigned char pixels[1024];
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));
    pcx.width=pcx.height=32;pcx.pixels=pixels;pcx.pixel_bytes=1024;
    pcx.palette[3]=(unsigned char)red;pcx.palette[4]=(unsigned char)green;pcx.palette[5]=(unsigned char)blue;
    return bh_texture_stage(textures,name,&pcx,0,0);
}
static int equal_files(const char *a,const char *b) {
    FILE *fa=fopen(a,"rb"),*fb=fopen(b,"rb");int ca,cb,ok=1;
    if(!fa || !fb){if(fa)fclose(fa);if(fb)fclose(fb);return 0;}
    do{ca=fgetc(fa);cb=fgetc(fb);if(ca!=cb){ok=0;break;}}while(ca!=EOF);
    fclose(fa);fclose(fb);return ok;
}
/* Count independent red/blue channels in actual native readback, rather than
   treating successful queue production as rendered output. */
static int colors(const char *path,unsigned long *red,unsigned long *blue) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER i;
    unsigned long x,y,stride;unsigned char rgb[3];
    *red=*blue=0;if(!f)return 0;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&i,sizeof(i),1,f)!=1 || i.biBitCount!=24 || i.biWidth!=320 || i.biHeight!=240){fclose(f);return 0;}
    stride=(i.biWidth*3+3)&~3UL;
    for(y=0;y<(unsigned long)i.biHeight;++y){
        if(fseek(f,h.bfOffBits+y*stride,SEEK_SET)){fclose(f);return 0;}
        for(x=0;x<(unsigned long)i.biWidth;++x){
            if(fread(rgb,1,3,f)!=3){fclose(f);return 0;}
            if(rgb[2]>16 && !rgb[1])++*red;
            if(rgb[0]>16 && !rgb[1] && !rgb[2])++*blue;
        }
    }
    fclose(f);return 1;
}
typedef struct HudWorld {
    BHBlockLibrary library;BHSceneLevel level;BHGeometry geometry;
    BHGeometryModel model;BHWorldGeometry nodes[2];int geometry_live,mounted;
} HudWorld;
static int world(HudWorld *w,const char *archive) {
    unsigned long j;BHVFile *file=NULL;long index;
    memset(w,0,sizeof(*w));bh_blocks_init(&w->library,NULL);bh_scene_level_init(&w->level,NULL);
    if(!archive){
        /* Isolated native fixture when original assets are absent. Both
           source-world nodes use the same six-slot topology as construction. */
        for(j=0;j<2;++j){w->nodes[j].minimum.x=-10;w->nodes[j].maximum.x=10;w->nodes[j].minimum.y=-10;w->nodes[j].maximum.y=10;w->nodes[j].minimum.z=-10;w->nodes[j].maximum.z=10;}
        w->nodes[0].navigation.count=w->nodes[1].navigation.count=1;
        w->nodes[0].navigation.portals[0].direction=0;w->nodes[0].navigation.portals[0].plane=100;
        w->nodes[0].navigation.portals[0].neighbor=&w->nodes[1].navigation;
        w->nodes[1].navigation.portals[0].direction=0;w->nodes[1].navigation.portals[0].plane=100;
        w->nodes[1].navigation.portals[0].neighbor=&w->nodes[0].navigation;
        return 1;
    }
    CHECK(bh_vfile_mount(archive),"actual archive mounted as immutable data");w->mounted=1;
    CHECK(bh_blocks_load(&w->library,"BSP\\BLOCKS.TXT","BSP\\",BH_BSP_PREFIX_ONLY),"real block geometry registry");
    file=bh_vfile_open("TEST.LEV","rb");
    CHECK(file && bh_scene_level_read(&w->level,file),"real level placement reader");
    if(file)bh_vfile_close(file);
    if(w->level.placement_count<=5797)return 0;
    index=bh_blocks_find(&w->library,w->level.placements[0].name);
    CHECK(index==129 && !strcmp(w->level.placements[0].name,w->level.placements[5797].name),"established real adjacent-room fixture identity");
    if(index!=129)return 0;
    CHECK(bh_geometry_init(&w->geometry,&w->library.blocks[index].bsp,NULL),"real owned mutable geometry");w->geometry_live=1;
    CHECK(bh_geometry_model_init(&w->model,&w->geometry,10,6),"real portal and model bound owners");
    if(!w->model.geometry)return 0;
    for(j=0;j<2;++j){
        const BHScenePlacement *p=w->level.placements+(j?5797:0);
        w->nodes[j].model=&w->model;w->nodes[j].flags=p->flags;
        w->nodes[j].query_flags=w->model.flags;
        w->nodes[j].position.x=(float)p->x;w->nodes[j].position.y=(float)p->y;w->nodes[j].position.z=(float)p->z;
        w->nodes[j].navigation.next=&w->nodes[1-j].navigation;w->nodes[j].previous=&w->nodes[1-j];
    }
    CHECK(bh_world_geometry_build(w->nodes,2) && bh_world_geometry_build(w->nodes+1,2),"both actual room worlds source constructed");
    CHECK(bh_portals_match(&w->nodes[0].navigation,2) && bh_portals_match(&w->nodes[1].navigation,2),"actual source portal connections");
    return w->nodes[0].navigation.portals[0].neighbor==&w->nodes[1].navigation;
}
static void dispose_world(HudWorld *w) {
    if(w->geometry_live)bh_geometry_dispose(&w->geometry);
    bh_scene_level_dispose(&w->level);bh_blocks_dispose(&w->library);
    if(w->mounted)bh_vfile_shutdown();
}

typedef struct NativeScene {
    BHDrawQueue *queue; BHViewConfig *view; BHMainCamera *camera;
    BHRenderState *state; HudImages *images;
    BHScreenQuadSubmit screen; BHPickupDrawContext rect;
    BHQuadQueueContext plane; BHHorizonQueueContext horizon;
    BHDrawSubmitContext model; BHDrawControlContext controls;
    BHPathQueueContext path; BHSegmentQueueContext segment;
    BHFadeContext fade;
    unsigned long kinds[17];
} NativeScene;
static int consume(NativeScene *n) {
    unsigned long at=0,next,steps=0;BHDrawQueue *q=n->queue;
    memset(n->kinds,0,sizeof(n->kinds));
    while(at!=BH_DRAW_END && steps++<q->count){
        BHDrawCommand *c;if(at>=q->count)return 0;c=q->commands+at;next=c->next;
        if(c->kind>=17)return 0;++n->kinds[c->kind];
        switch(c->kind){
        case BH_DRAW_NOP:break;
        case BH_DRAW_STATE:if(!bh_queue_mode_execute(n->state,c,&next))return 0;break;
        case BH_DRAW_CAMERA:if(!bh_camera_queue_execute(n->view,n->camera,q,c,&next))return 0;break;
        case BH_DRAW_HORIZON:if(!bh_horizon_queue_execute(&n->horizon,q,c,&next))return 0;break;
        case BH_DRAW_MATERIAL_WORDS:if(!bh_material_queue_execute(image,n->images,c,&next))return 0;break;
        case BH_DRAW_MODEL_RESOURCE:if(!bh_draw_submit_execute(&n->model,c,&next))return 0;break;
        case BH_DRAW_CLIP_RECT:case BH_DRAW_TEXTURE_MASK:
            if(!bh_draw_controls_execute(&n->controls,q,c,&next))return 0;break;
        case BH_DRAW_SCREEN_QUAD:if(!bh_screen_quad_execute(&n->screen,c,&next))return 0;break;
        case BH_DRAW_RECT:if(!bh_pickup_rect_execute(&n->rect,image,n->images,c,&next))return 0;break;
        case BH_DRAW_ORIENTED:if(!bh_plane_quad_execute(&n->plane,c,&next))return 0;break;
        case BH_DRAW_QUAD:if(!bh_quad_queue_execute(&n->plane,c,&next))return 0;break;
        case BH_DRAW_PATH_HEADER:if(!bh_path_queue_execute(&n->path,q,c,&next))return 0;break;
        case BH_DRAW_SEGMENT:if(!bh_segment_queue_execute(&n->segment,c,&next))return 0;break;
        case BH_DRAW_FADE:if(!bh_fade_execute(&n->fade,c,&next))return 0;break;
        default:return 0;
        }
        at=next;
    }
    return at==BH_DRAW_END;
}
static void vector_put(BHEntity *p,unsigned long at,float x,float y,float z){
    BHVec3 v;v.x=x;v.y=y;v.z=z;memcpy((unsigned char*)p+at,&v,12);
}
static void word_put(BHEntity *p,unsigned long at,unsigned long word){memcpy((unsigned char*)p+at,&word,4);}
static void float_put(BHEntity *p,unsigned long at,float value){memcpy((unsigned char*)p+at,&value,4);}

/* This invokes one source callback per fixed fixture and drains only its draw
   commands into an offscreen surface. It has no game/window/input/step loop. */
int bh_player_scene_native_checkpoint(const char *archive,unsigned long *count){
    BHSceneView view;BHTextureContext textures;BHRenderMaterial material;BHRenderState state;
    BHNativePrimitive native;BHDrawQueue queue;BHMainCamera camera;BHMathEnvironment fp;
    BHScreenClipContext screen_clip;BHClipContext clip;BHAttributeClipContext attribute;
    BHScreenQuadContext screen;BHSpriteQuadContext plane;BHLightConfig light;
    BHHorizonContext horizon;BHHorizonNative horizon_native;NativeScene consumer;
    BHSpriteOrientedContext ribbon;BHSpriteOrientedVertex path_nodes[512];
    BHModelContext models;BHRenderResources resources;BHModelInstanceContext prototype;
    BHDrawResourceContext resource_draw;BHSceneBindings bindings;BHSceneOwner owner;
    BHSceneBindingInput inputs[2];BHPickupRoom rooms[2];BHEntityScheduleRoom scheduled[2];
    BHEntityScheduleContext scheduler;HudWorld topology;HudImages images;
    BHEntityPool pool;BHWeaponFactory factory;BHSoundQueue sound;BHEntity player,*actor=NULL,*pickup=NULL;
    BHPlayerScene scene;BHSceneBackground background;BHAttachmentContext graph;BHRadarFrame radar_frame;
    BHHudContext hud;BHHudMarkers markers;BHRadarContext radar;BHHudWorldContext world_status;
    BHHudFlatContext flat_status;BHArcContext arc;BHWeaponHudTargetContext target;
    BHAtlasContext atlas;BHAtlasPage *confirmation=NULL;BHCameraState *embedded;
    long modes=1,frames=100,width=320,height=240,confirm=0;
    unsigned long i,j,handle=0,cockpit=0,prior,phase,pixels[8]={0,0,0,0,0,0,0,0};
    int pool_live=0,factory_live=0,world_live=0;
    const char *outputs[8]={"player-scene-native-gated.bmp","player-scene-native-flat.bmp",
        "player-scene-native-world.bmp","player-scene-native-confirm.bmp",
        "player-scene-native-tail.bmp","player-scene-native-particles.bmp","player-scene-native-repeat.bmp",
        "player-scene-native-cockpit.bmp"};
    const char *assets[14]={"bluesky.bmp","cross.bmp","cross2.bmp","radar.bmp","blue.bmp",
        "dots.bmp","tags.bmp","clipbrd.bmp","weapons.bmp","shield.bmp","water.gif","thrust.bmp","malfunc.bmp","blue2.bmp"};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));
    memset(&native,0,sizeof(native));memset(&queue,0,sizeof(queue));memset(&camera,0,sizeof(camera));
    memset(&plane,0,sizeof(plane));memset(&light,0,sizeof(light));memset(&consumer,0,sizeof(consumer));
    memset(&horizon_native,0,sizeof(horizon_native));memset(&ribbon,0,sizeof(ribbon));
    memset(&resources,0,sizeof(resources));memset(&prototype,0,sizeof(prototype));memset(&resource_draw,0,sizeof(resource_draw));
    memset(&bindings,0,sizeof(bindings));memset(&owner,0,sizeof(owner));memset(&scheduler,0,sizeof(scheduler));
    memset(&images,0,sizeof(images));memset(&pool,0,sizeof(pool));memset(&factory,0,sizeof(factory));
    memset(&sound,0,sizeof(sound));memset(&player,0,sizeof(player));memset(&scene,0,sizeof(scene));
    memset(&graph,0,sizeof(graph));memset(&radar_frame,0,sizeof(radar_frame));memset(&hud,0,sizeof(hud));
    memset(&markers,0,sizeof(markers));memset(&world_status,0,sizeof(world_status));memset(&flat_status,0,sizeof(flat_status));
    memset(&arc,0,sizeof(arc));memset(&target,0,sizeof(target));memset(&atlas,0,sizeof(atlas));
    CHECK(world(&topology,archive),"source topology and actual level data");world_live=1;
    CHECK(bh_scene_view_init(&view,320,240),"real offscreen Direct3D2 surface");if(!view.device)goto done;
    view.original_depth=1;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"actual texture owner and formats");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    if(archive){
        textures.palette_mode=2;
        CHECK(bh_texture_set_path(&textures,"textures",2),"observed original texture palette policy and path");
        for(i=0;i<14;++i){
            if(i==10)continue; /* water belongs to the later magenta-key group */
            images.items[i+1]=bh_texture_load(&textures,assets[i]);CHECK(images.items[i+1]!=NULL,"actual startup-named source image");if(!images.items[i+1])goto done;
        }
    }else{
        for(i=1;i<=14;++i){char name[32];sprintf(name,"SCENE%lu.PCX",i);images.items[i]=solid(&textures,name,i==1?0:255,i==1?0:255,255);CHECK(images.items[i]!=NULL,"isolated owned source texture");if(!images.items[i])goto done;}
    }
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"real native material and mode owners");
    CHECK(bh_view_config_init(&view.projection,320,240) && bh_main_camera_settings(&view.projection,60,.01f,25000) &&
        bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0),"actual initial camera projection");
    CHECK(bh_clip_init(&clip,.01f,25000,0,320,0,240) && bh_clip_set_camera(&clip,&camera.projected) &&
        bh_screen_clip_init(&screen_clip,.01f,25000,0,320,0,240) && bh_attribute_clip_init(&attribute,.01f,25000,0,320,0,240),"actual three clipping owners");
    native.textures=&textures;native.near_z=.01f;horizon_native.textures=&textures;horizon_native.near_z=.01f;
    memset(&screen,0,sizeof(screen));screen.view=&view.projection;screen.clip=&screen_clip;
    screen.emit=bh_native_primitive_emit;screen.emit_user=&native;screen.base_color=0xff000000UL;
    CHECK(bh_light_configure(&light,0,0,0,0,0,0,0,0),"actual disabled-light state");
    plane.camera=&camera;plane.clip=&clip;plane.directional=&light;plane.base_color=0xff000000UL;
    plane.emit=bh_native_primitive_emit;plane.emit_user=&native;
    CHECK(bh_horizon_init(&horizon,&camera,&attribute,&state,NULL,bh_horizon_native_emit,&horizon_native),"real horizon uses actual native gradient owner");
    CHECK(bh_draw_queue_init(&queue,4096,NULL),"one live source draw queue");
    consumer.queue=&queue;consumer.view=&view.projection;consumer.camera=&camera;consumer.state=&state;consumer.images=&images;
    consumer.screen.draw=&screen;consumer.screen.lookup=image;consumer.screen.lookup_user=&images;
    consumer.rect.queue=&queue;consumer.rect.view=&view.projection;consumer.rect.camera=&camera;consumer.rect.clip=&screen_clip;
    consumer.rect.emit=bh_native_primitive_emit;consumer.rect.emit_user=&native;
    consumer.plane.draw=&plane;consumer.plane.lookup=image;consumer.plane.lookup_user=&images;
    consumer.horizon.draw=&horizon;consumer.horizon.lookup=image;consumer.horizon.lookup_user=&images;
    ribbon.path=path_nodes;ribbon.capacity=512;ribbon.near_z=.01f;ribbon.projection_scale=1;
    ribbon.clip=&attribute;ribbon.emit=bh_native_attribute_emit;ribbon.emit_user=&native;
    consumer.path.view=&view.projection;consumer.path.camera=&camera;consumer.path.draw=&ribbon;consumer.path.lookup=image;consumer.path.lookup_user=&images;
    consumer.segment.camera=&camera;consumer.segment.draw=&ribbon;consumer.segment.lookup=image;consumer.segment.lookup_user=&images;
    consumer.fade.render=&state;consumer.fade.width=320;consumer.fade.height=240;consumer.fade.near_plane=.01f;
    CHECK(bh_model_context_init(&models,&textures,NULL) && bh_model_path_set(&models,"BSP\\",BH_BSP_PREFIX_ONLY),"actual model owner and literal path");
    prototype.camera=&camera;prototype.config=&view.projection;prototype.clip=&clip;prototype.light_config=&light;
    prototype.texture_mask=0xffffffffUL;prototype.base_color=0xff000000UL;prototype.emit=bh_native_primitive_emit;prototype.emit_user=&native;
    CHECK(bh_render_resources_init(&resources,&models,&prototype,NULL),"real resource owner lifetime");
    if(archive){
        /* Actual00479200 changes policy at004797ff before cockpitL/R and all
           later model resources. Keeping policy2 here exposes fuchsia key
           pixels in the genuine cockpit textures instead of making windows. */
        CHECK(bh_texture_policy_set(&textures,1,0xff00ffUL),"original later magenta-key palette group");
        handle=bh_render_resources_load_model(&resources,topology.library.blocks[129].name);
        cockpit=bh_render_resources_load_model(&resources,"cockpit.bsp");
        CHECK(handle && cockpit,"actual room and original cockpit BSP resources");if(!handle || !cockpit)goto done;
        images.items[11]=bh_texture_load(&textures,"water.gif");
        CHECK(images.items[11]!=NULL,"actual later water material policy");if(!images.items[11])goto done;
        memset(inputs,0,sizeof(inputs));memset(rooms,0,sizeof(rooms));memset(scheduled,0,sizeof(scheduled));
        scheduler.queue=&queue;scheduler.camera=&owner.camera;scheduler.viewport=&view.projection;scheduler.max_rooms=2;scheduler.max_entities=1;scheduler.max_depth=2;
        scheduler.overlay_position=&player.position;
        for(i=0;i<2;++i){inputs[i].world=topology.nodes+i;inputs[i].resource=handle;inputs[i].angles.x=(float)((topology.level.placements[i?5797:0].flags&3UL)*1.5707963267948966);}
        CHECK(bh_scene_bindings_init(&bindings,inputs,2,rooms,scheduled,&resources,&scheduler),"actual constructed world and owned resources bind to scene");
    }
    CHECK(bh_scene_owner_init(&owner,&bindings,320,240),"real complete outer scene owner");
    resource_draw.camera=&camera;resource_draw.config=&view.projection;
    consumer.model.draw=&resource_draw;consumer.model.lookup=bh_render_resources_lookup;consumer.model.lookup_user=&resources;
    consumer.controls.view=&view.projection;consumer.controls.resources=&resources;consumer.controls.model_clip=&clip;
    consumer.controls.screen_clip=&screen_clip;consumer.controls.attribute_clip=&attribute;consumer.controls.texture_mask=0xffffffffUL;
    CHECK(bh_entities_init(&pool,12,NULL),"real pool for source weapon records");pool_live=1;
    CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL),"real sound listener owner without audio playback");
    CHECK(bh_weapon_factory_open(&factory,&pool,&sound),"explicit construction adapter binds real query owner");factory_live=1;
    for(i=0;i<10;++i){BHEntity *weapon=bh_weapon_factory_create(&factory,i);CHECK(weapon!=NULL,"real ten-record source weapon allocation");word_put(&player,0x258+4*i,(unsigned long)weapon);}
    vector_put(&player,0x18,1,0,0);vector_put(&player,0x24,0,1,0);vector_put(&player,0x30,0,0,1);
    vector_put(&player,0x280,0,0,1);vector_put(&player,0x28c,0,0,10);
    float_put(&player,0x24c,.25f);float_put(&player,0x23c,.5f);word_put(&player,0x230,12345);word_put(&player,0x234,60000);
    *((unsigned char*)&player+0x2b6)=0;*((unsigned char*)&player+0x2b7)=9;
    bh_weapon_object_state(factory.registry.slots[0].entity)->common.ammo=300;
    bh_weapon_object_state(factory.registry.slots[9].entity)->common.ammo=1;
    if(archive){player.position.x=-970;player.position.y=-60;player.position.z=1460;word_put(&player,0xc4,(unsigned long)topology.nodes);
        vector_put(&player,0x18,0,0,-1);vector_put(&player,0x30,1,0,0);float_put(&player,0x3c,1.5707963267948966f);}
    embedded=bh_player_camera_state(&player);embedded->position=player.position;embedded->angles.x=archive?1.5707963267948966f:0;
    graph.queue=&queue;graph.frame=&radar_frame;graph.viewport_width=320;graph.default_image=graph.flag200_image=graph.flag400_image=graph.flag800_image=5;
    radar_frame.near_z=.01f;hud.frame=&radar_frame;hud.attachment=&graph;hud.background_image=4;
    CHECK(bh_radar_bind(&radar,&queue,&radar_frame,6,7),"real radar and HUD share live state");
    markers.projection=&consumer.rect;markers.radar=&radar;markers.brackets_image=7;markers.status_image=7;markers.display_width=320;markers.limit=12;
    world_status.hud=&hud;world_status.atlas_image=8;world_status.weapon_icons_image=9;
    flat_status.hud=&hud;flat_status.atlas_image=8;flat_status.weapon_icons_image=9;flat_status.viewport_height=240;
    arc.queue=&queue;arc.image=5;target.projection=&consumer.rect;target.viewport_width=320;target.atlas_image=7;
    CHECK(bh_scene_background_init(&background,&queue,handle,handle,5,5),"real background owner with explicit borrowed resources");
    scene.player=&player;scene.queue=&queue;scene.sound=&sound;scene.hud=&hud;scene.markers=&markers;scene.radar=&radar;
    scene.world_status=&world_status;scene.flat_status=&flat_status;scene.background=&background;scene.scene=&owner;
    scene.weapons=&factory.registry;scene.arc=&arc;scene.target=&target;scene.actor_head=&actor;scene.pickup_head=&pickup;scene.pickup_limit=12;
    scene.mode_count=&modes;scene.frame_count=&frames;scene.display_width=&width;scene.display_height=&height;scene.confirmation_gate=&confirm;
    scene.material_image=11;scene.horizon_image=1;scene.aim_image=2;scene.aim_overlay_image=3;scene.particle_image=10;scene.trail_image=12;scene.overlay_image=13;
    scene.cockpit_resource=cockpit;
    if(archive){
        CHECK(bh_atlas_context_init(&atlas,&textures,320),"actual TVFONT atlas owner");confirmation=bh_atlas_page_create(&atlas,"textures\\tvfont.pcx",64);
        CHECK(confirmation && bh_atlas_page_clear(&atlas,confirmation,0),"real confirmation font page created and cleared");
        if(confirmation){bh_atlas_page_cursor(confirmation,32-bh_atlas_page_text_width(confirmation,(const unsigned char*)"Abandon")/2,13);CHECK(bh_atlas_page_text(confirmation,(const unsigned char*)"Abandon")>0,"actual Abandon glyphs");
            bh_atlas_page_cursor(confirmation,32-bh_atlas_page_text_width(confirmation,(const unsigned char*)"Game ?")/2,26);CHECK(bh_atlas_page_text(confirmation,(const unsigned char*)"Game ?")>0,"actual Game question glyphs");
            bh_atlas_page_cursor(confirmation,32-bh_atlas_page_text_width(confirmation,(const unsigned char*)"[Y/N]")/2,40);CHECK(bh_atlas_page_text(confirmation,(const unsigned char*)"[Y/N]")>0,"actual confirmation-key glyphs");
            images.items[15]=bh_atlas_page_image(confirmation);scene.confirmation_image=15;}
    }else scene.confirmation_image=4;
    CHECK(bh_texture_preallocate(&textures),"all real image formats have residency slots before consumption");
    for(phase=0;phase<8;++phase){
        int active=phase!=0 && phase!=4;
        if((phase==4 || phase==7) && !archive)continue; /* cockpit is a named BSP asset */
        *((unsigned char*)&player+0x2bb)=(unsigned char)active;word_put(&player,0xe8,phase==2?0:1);confirm=phase==3;
        frames=(phase==4 || phase==7)?74:100;float_put(&player,0x240,phase==5?1:0);
        *((unsigned char*)&player+0x2b4)=phase==4?0x10:0;
        if(phase==4){
            for(j=0;j<4;++j){
                float side=(float)j-1.5f;BHVec3 from=player.position,to=player.position;
                if(archive){from.x+=20;from.z+=side;to.x+=25;to.z+=side;}
                else {from.z+=20;from.x+=side;to.z+=25;to.x+=side;}
                memcpy((unsigned char*)&player+0x2fc+j*24,&from,12);
                memcpy((unsigned char*)&player+0x308+j*24,&to,12);
                vector_put(&player,0x2c0+j*12,0,0,2);
            }
        }
        /* Eight legitimate delayed slots exercise the real wait branch and
           avoid introducing unrelated random background models into this fixture. */
        for(j=0;j<8;++j){background.slots[j].elapsed=0;background.slots[j].delay=100;}
        CHECK(bh_draw_queue_begin(&queue),"source frame reset");prior=native.polygons;
        CHECK(bh_player_scene_draw(&scene),"complete source callback with actual live dependency owners");
        CHECK(phase?queue.count>15:queue.count==9,"independent main-gate blank command control");
        CHECK(phase?sound.listener_active==1:!sound.listener_active,"actual listener state follows main gate");
        CHECK(bh_scene_view_begin(&view) && consume(&consumer),"all produced kinds consumed by real source owners");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,outputs[phase]),"actual native flush and bitmap readback");pixels[phase]=view.pixels;
        if(!phase)CHECK(!view.pixels && native.polygons==prior,"gated callback is independently blank");
        else CHECK(view.pixels>100 && native.polygons>prior,"full source callback reaches actual pixels");
        if(active){CHECK(consumer.kinds[BH_DRAW_CAMERA]==1 && consumer.kinds[BH_DRAW_HORIZON]==1 && consumer.kinds[BH_DRAW_MATERIAL_WORDS]==1,"camera horizon and material commands execute");
            CHECK(archive?consumer.kinds[BH_DRAW_MODEL_RESOURCE]>=2:!consumer.kinds[BH_DRAW_MODEL_RESOURCE],"actual real-world resources execute only in real asset fixture");}
        if(phase==3)CHECK(consumer.kinds[BH_DRAW_SCREEN_QUAD]>=1,"confirmation overlay uses actual screen consumer");
        if(phase==4)CHECK(consumer.kinds[BH_DRAW_MODEL_RESOURCE]==1 && consumer.kinds[BH_DRAW_SEGMENT]==4 &&
            consumer.kinds[BH_DRAW_SCREEN_QUAD]==1 && !consumer.kinds[BH_DRAW_CAMERA] && scene.random_calls==4,
            "main-gated tail consumes real cockpit model overlay and four source trail segments");
        if(phase==5)CHECK(consumer.kinds[BH_DRAW_ORIENTED]==32 && scene.random_calls==128,
            "complete transient producer reaches real plane consumers with128 random calls");
        if(phase==7)CHECK(consumer.kinds[BH_DRAW_MODEL_RESOURCE]>=3 && consumer.kinds[BH_DRAW_SEGMENT]==4 &&
            scene.random_calls==4 && !equal_files(outputs[1],outputs[7]),
            "whole active scene composes real world HUD cockpit and four trails in one callback");
    }
    CHECK(!equal_files(outputs[0],outputs[1]) && !equal_files(outputs[1],outputs[2]) && !equal_files(outputs[1],outputs[3]),"blank flat world and confirmation outputs independently differ");
    CHECK(equal_files(outputs[1],outputs[6]) && !equal_files(outputs[1],outputs[5]),"flat image repeats after transient particles and particle pixels differ");
    CHECK(bh_draw_queue_dispose(&queue) && bh_draw_queue_init(&queue,9,NULL),"cap real scene queue at its nine headers");
    *((unsigned char*)&player+0x2bb)=1;frames=74;confirm=1;
    float_put(&player,0x240,1);srand(17);prior=owner.frame;
    CHECK(bh_player_scene_draw(&scene) && queue.count==9 && scene.random_calls==132,
        "complete callback retains132 direct random calls and succeeds when every real-world draw drops");
    CHECK(archive?owner.frame==prior+1:owner.frame==prior,
        "real source scene mutation survives full queue while absent-world branch remains inactive");
done:
    if(factory_live){for(i=0;i<10;++i)if(factory.registry.slots[i].entity)bh_entity_release(&pool,factory.registry.slots[i].entity);CHECK(bh_weapon_factory_close(&factory),"all weapon identities released before query binding closes");}
    bh_sound_queue_dispose(&sound);if(pool_live)bh_entities_dispose(&pool);
    bh_draw_queue_dispose(&queue);bh_fade_dispose(&consumer.fade);bh_scene_bindings_dispose(&bindings);bh_render_resources_dispose(&resources);
    bh_render_material_dispose(&material);if(confirmation)bh_atlas_page_dispose(&atlas,confirmation);bh_atlas_context_dispose(&atlas);
    bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);if(world_live)dispose_world(&topology);
    CHECK(bh_math_environment_supported(),"native composition preserves declared PC53 profile");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"player-scene-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
