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
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL hud-native: %s\n",s);}}while(0)
typedef struct HudImages {BHTextureImage *items[12];} HudImages;
static BHTextureImage *__cdecl image(unsigned long handle,void *raw) {
    HudImages *images=(HudImages *)raw;
    return handle<12?images->items[handle]:NULL;
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
static int drain(BHDrawQueue *q,BHScreenQuadSubmit *submit,BHPickupDrawContext *rect,HudImages *images,BHQuadQueueContext *plane,BHRenderState *state) {
    unsigned long at=0,next,steps=0;
    while(at!=BH_DRAW_END && steps++<q->count){
        BHDrawCommand *c;if(at>=q->count)return 0;c=q->commands+at;next=c->next;
        switch(c->kind){
        case BH_DRAW_NOP:break;
        case BH_DRAW_STATE:if(!bh_queue_mode_execute(state,c,&next))return 0;break;
        case BH_DRAW_SCREEN_QUAD:if(!bh_screen_quad_execute(submit,c,&next))return 0;break;
        case BH_DRAW_RECT:if(!bh_pickup_rect_execute(rect,image,images,c,&next))return 0;break;
        case BH_DRAW_ORIENTED:if(!bh_plane_quad_execute(plane,c,&next))return 0;break;
        case BH_DRAW_QUAD:if(!bh_quad_queue_execute(plane,c,&next))return 0;break;
        default:return 0;
        }
        at=next;
    }
    return at==BH_DRAW_END;
}
/* Position-query method used only by this isolated source rendering fixture.
   It is not a production actor receiver or an original-backed callback. */
static void __cdecl marker_position(BHEntity *entity,void *raw) {
    BHEntityPositionPacket *packet=(BHEntityPositionPacket *)raw;
    if(packet->event==3)packet->position=entity->position;
}
int bh_hud_native_checkpoint(const char *archive,unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHRenderMaterial material;BHRenderState state;
    BHNativePrimitive native;BHDrawQueue q;BHScreenClipContext clip;BHScreenQuadContext draw;
    BHScreenQuadSubmit submit;BHPickupDrawContext rect;BHMainCamera camera;
    BHClipContext world_clip;BHSpriteQuadContext plane;BHQuadQueueContext plane_submit;BHLightConfig light;
    BHHudContext hud;BHHudPlayer player;BHAttachmentContext graph;BHRadarFrame frame;
    BHRadarContext radar;HudImages images;HudWorld rooms;BHMathEnvironment fp;
    BHHudWorldContext world_status;BHHudFlatContext flat_status;BHHudStatusState status;
    BHHudWeaponAmmo ammo[2];const BHHudWeaponAmmo *weapons[6];BHHudMarkers markers;BHEntity actor;
    BHAtlasContext atlas;BHAtlasPage *page=NULL;BHTextQueue text_queue;BHHudScores scores;
    BHHudScoreText score_text;BHHudScoreDraw score_draw;BHEntity peers[2],*slots[8];
    unsigned long mode,red,blue,baseline_blue=0,prior;int world_live=0;
    const char *names[5]={"hud-native-background.bmp","hud-native-graph.bmp","hud-native-repeat.bmp","hud-native-hidden.bmp","hud-native-world.bmp"};
    checks=failures=0;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));
    memset(&native,0,sizeof(native));memset(&q,0,sizeof(q));memset(&draw,0,sizeof(draw));
    memset(&rect,0,sizeof(rect));memset(&camera,0,sizeof(camera));memset(&hud,0,sizeof(hud));
    memset(&player,0,sizeof(player));memset(&graph,0,sizeof(graph));memset(&frame,0,sizeof(frame));memset(&images,0,sizeof(images));
    memset(&plane,0,sizeof(plane));memset(&light,0,sizeof(light));
    memset(&atlas,0,sizeof(atlas));
    if(!bh_math_environment_enter(&fp))return 2;
    world_live=1;CHECK(world(&rooms,archive),"live room/portal graph fixture");
    CHECK(bh_scene_view_init(&view,320,240),"real native HUD offscreen target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"actual texture context and native formats");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    images.items[1]=solid(&textures,"HUDBLUE.PCX",0,0,255);images.items[2]=solid(&textures,"HUDRED.PCX",255,0,0);
    CHECK(images.items[1] && images.items[2],"independent blue background and red portal-line textures");if(!images.items[1] || !images.items[2])goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"actual native material and state owners");
    view.original_depth=1;
    CHECK(bh_view_config_init(&view.projection,320,240) && bh_main_camera_settings(&view.projection,60,.25,25000) &&
        bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) && bh_screen_clip_init(&clip,.25f,25000,0,320,0,240),"real live projection and screen clipping");
    native.textures=&textures;native.near_z=view.projection.near_z;
    draw.view=&view.projection;draw.clip=&clip;draw.emit=bh_native_primitive_emit;draw.emit_user=&native;draw.base_color=0xff000000UL;
    submit.draw=&draw;submit.lookup=image;submit.lookup_user=&images;
    rect.queue=&q;rect.view=&view.projection;rect.camera=&camera;rect.clip=&clip;rect.emit=bh_native_primitive_emit;rect.emit_user=&native;
    CHECK(bh_draw_queue_init(&q,256,NULL),"real queue owns full radar command lifetime");
    graph.queue=&q;graph.frame=&frame;graph.viewport_width=320;graph.default_image=graph.flag200_image=graph.flag400_image=graph.flag800_image=2;
    frame.near_z=view.projection.near_z;hud.frame=&frame;hud.attachment=&graph;hud.background_image=1;
    player.right.x=player.up.y=player.forward.z=1;player.flat_raw=1;
    player.position.x=(rooms.nodes[0].minimum.x+rooms.nodes[0].maximum.x)*.5f;
    player.position.y=(rooms.nodes[0].minimum.y+rooms.nodes[0].maximum.y)*.5f;
    player.position.z=(rooms.nodes[0].minimum.z+rooms.nodes[0].maximum.z)*.5f;
    CHECK(bh_main_camera_pose(&view.projection,&camera,player.position.x,player.position.y,player.position.z,0,0,0) &&
        bh_clip_init(&world_clip,view.projection.near_z,25000,0,320,0,240) && bh_clip_set_camera(&world_clip,&camera.projected) &&
        bh_light_configure(&light,0,0,0,0,0,0,0,0),"real player-relative world camera and disabled directional light");
    plane.camera=&camera;plane.clip=&world_clip;plane.directional=&light;plane.base_color=0xff000000UL;plane.emit=bh_native_primitive_emit;plane.emit_user=&native;
    plane_submit.draw=&plane;plane_submit.lookup=image;plane_submit.lookup_user=&images;
    CHECK(bh_radar_bind(&radar,&q,&frame,2,2),"actual marker shares HUD-owned frame");
    for(mode=0;mode<5;++mode){
        CHECK(bh_draw_queue_begin(&q),"native HUD frame queue reset");
        player.root=(mode==1 || mode==2 || mode==4)?rooms.nodes:NULL;player.flat_raw=mode>=3?0:1;
        CHECK(bh_hud_setup(&hud,&player),"complete HUD setup and guarded graph reset");
        CHECK(!rooms.nodes[0].radar_visited && !rooms.nodes[1].radar_visited,"both shared real-world traversal marks reset after rendering");
        if(mode==1 || mode==2 || mode==4)CHECK(graph.visited==2 && graph.queued>=1,"actual connected source rooms reach radar line queues");
        CHECK(bh_scene_view_begin(&view),"native HUD scene begin");prior=native.polygons;
        CHECK(drain(&q,&submit,&rect,&images,&plane_submit,&state),"all HUD commands consumed by real source geometry and native emitter");
        CHECK(mode==3?native.polygons==prior:native.polygons>prior,"blank and visible native emission controls");
        CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,names[mode]),"actual native flush and readback");
        CHECK(colors(names[mode],&red,&blue),"native bitmap color readback");
        if(mode==0){baseline_blue=blue;CHECK(!red && blue>1000,"independent blue-only background control");}
        else if(mode<3)CHECK(red>0 && blue>0 && blue<baseline_blue,"source graph replaces blue background pixels with red portal connectors");
        else if(mode==3)CHECK(!red && !blue && !view.pixels,"source world mode without graph is blank");
        else CHECK(red>0 && !blue,"world HUD plane source geometry produces red portal connectors");
    }
    CHECK(equal_files(names[1],names[2]) && !equal_files(names[0],names[1]),"repeat HUD graph deterministic after reset and differs from no-root control");
    if(archive){
        /* Actual00479200 startup selects policy(2,0) for this image group.
           Opaque policy0 remaps the black index and destroys these HUD masks. */
        textures.palette_mode=2;textures.palette_color=0;
        CHECK(bh_texture_set_path(&textures,"textures",2),"actual original HUD image path");
        images.items[1]=bh_texture_load(&textures,"radar.bmp");images.items[2]=bh_texture_load(&textures,"blue.bmp");
        images.items[3]=bh_texture_load(&textures,"blue2.bmp");images.items[4]=bh_texture_load(&textures,"blue3.bmp");
        images.items[5]=bh_texture_load(&textures,"blue4.bmp");images.items[6]=bh_texture_load(&textures,"dots.bmp");
        images.items[7]=bh_texture_load(&textures,"tags.bmp");images.items[8]=bh_texture_load(&textures,"clipbrd.bmp");
        images.items[9]=bh_texture_load(&textures,"weapons.bmp");
        CHECK(images.items[1] && images.items[2] && images.items[3] && images.items[4] && images.items[5] &&
            images.items[6] && images.items[7] && images.items[8] && images.items[9],"all nine startup-named HUD assets decode through recovered PCX texture owner");
        if(!images.items[1] || !images.items[2] || !images.items[6] || !images.items[7] || !images.items[8] || !images.items[9])goto done;
        graph.default_image=2;graph.flag200_image=3;graph.flag400_image=4;graph.flag800_image=5;
        CHECK(bh_radar_bind(&radar,&q,&frame,6,7),"actual dots and tags share the HUD setup frame");
        memset(&status,0,sizeof(status));memset(ammo,0,sizeof(ammo));memset(weapons,0,sizeof(weapons));
        ammo[0].amount=120;ammo[1].amount=9;weapons[0]=ammo;weapons[5]=ammo+1;
        status.weapons=weapons;status.weapon_count=6;status.primary=0;status.secondary=5;
        status.gauge_a=.25f;status.gauge_b=.5f;status.counter_a=12345;status.counter_b=60000;
        memset(&world_status,0,sizeof(world_status));world_status.hud=&hud;world_status.atlas_image=8;world_status.weapon_icons_image=9;
        memset(&flat_status,0,sizeof(flat_status));flat_status.hud=&hud;flat_status.viewport_height=240;flat_status.atlas_image=8;flat_status.weapon_icons_image=9;
        memset(&markers,0,sizeof(markers));markers.projection=&rect;markers.radar=&radar;markers.brackets_image=7;markers.status_image=7;
        markers.display_width=320;markers.limit=1;
        memset(&actor,0,sizeof(actor));actor.query=marker_position;actor.position=player.position;actor.position.z+=20;
        {long kind=2;memcpy(actor.payload+24,&kind,4);}
        CHECK(bh_atlas_context_init(&atlas,&textures,320),"real atlas owner with observed TVFONT scan stride");
        page=bh_atlas_page_create(&atlas,"textures\\tvfont.pcx",128);
        CHECK(page && page->atlas && page->atlas->rectangle_count>64,"actual original128px scoreboard/target-label page");
        memset(&score_text,0,sizeof(score_text));memset(&score_draw,0,sizeof(score_draw));
        memset(peers,0,sizeof(peers));memset(slots,0,sizeof(slots));
        {short first=75,second=-2;memcpy(peers[0].payload+56,&first,2);memcpy(peers[1].payload+56,&second,2);}
        strcpy((char *)peers[0].payload+58,"PEER");strcpy((char *)peers[1].payload+58,"RIVAL");
        slots[1]=peers;slots[2]=peers+1;
        scores.remote=slots;scores.local_name="LOCAL";scores.local_score=100;scores.local_index=0;
        if(page){
            unsigned long processed;
            images.items[10]=bh_atlas_page_image(page);score_text.queue=&text_queue;score_text.page=page;bh_text_queue_init(&text_queue);
            CHECK(bh_hud_scores_text(&score_text,&scores) && score_text.queued==7 &&
                bh_text_queue_drain(&text_queue,&atlas,&processed)==1 && processed==7,"complete source roster builder and real queued clear/native glyph blits");
            score_draw.queue=&q;score_draw.page_image=10;score_draw.display_width=320;score_draw.near_z=frame.near_z;
            markers.status_image=10;
        }
        CHECK(bh_texture_preallocate(&textures) && images.items[1]->format->count && images.items[8]->format->count &&
            images.items[9]->format->count && (!page || page->image->format->count),"actual HUD assets loaded after material initialization acquire real residency slots");
        for(mode=0;mode<3;++mode){
            const char *name=mode==0?"hud-native-original-flat.bmp":mode==1?"hud-native-original-world.bmp":"hud-native-original-flat-repeat.bmp";
            CHECK(bh_draw_queue_begin(&q),"original image HUD queue reset");
            player.flat_raw=mode!=1;player.root=rooms.nodes;
            CHECK(bh_hud_setup(&hud,&player) && bh_hud_markers_draw(&markers,&actor,&player.position,&status.flags),"complete setup graph marker and real queried target brackets");
            CHECK(mode==1?bh_hud_world_draw(&world_status,&status):bh_hud_flat_draw(&flat_status,&status),"complete selected HUD status owner with signed ammo and counters");
            if(page)CHECK(bh_hud_scores_draw(&score_draw,&scores) && score_draw.queued==3,"complete ranked scoreboard borrows real populated font page");
            CHECK(bh_scene_view_begin(&view),"actual-asset native HUD scene");prior=native.polygons;
            CHECK(drain(&q,&submit,&rect,&images,&plane_submit,&state) && native.polygons>prior+10,"real HUD atlas quads and markers reach source clipping and native queues");
            CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,name) && view.pixels>100,"actual HUD asset pixels read back");
            CHECK(images.items[8]->resident && images.items[8]->resident->handle && images.items[9]->resident &&
                images.items[9]->resident->handle,"HUD atlases rendered with actual native texture handles");
            CHECK(!rooms.nodes[0].radar_visited && !rooms.nodes[1].radar_visited,"actual status composition retains graph reset");
        }
        CHECK(equal_files("hud-native-original-flat.bmp","hud-native-original-flat-repeat.bmp") &&
            !equal_files("hud-native-original-flat.bmp","hud-native-original-world.bmp"),"original texture flat repeat stable across intervening world HUD mode");
    }
done:
    bh_draw_queue_dispose(&q);bh_render_material_dispose(&material);
    if(page)bh_atlas_page_dispose(&atlas,page);bh_atlas_context_dispose(&atlas);
    bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    if(world_live)dispose_world(&rooms);
    CHECK(bh_math_environment_supported(),"native HUD preserves PC53 profile");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"hud-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
