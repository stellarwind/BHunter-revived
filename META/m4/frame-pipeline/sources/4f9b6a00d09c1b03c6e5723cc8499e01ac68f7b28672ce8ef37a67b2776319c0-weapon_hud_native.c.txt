#include "../src/bh_weapon_hud_arc.h"
#include "../src/bh_weapon_hud_target.h"
#include "../src/bh_path_queue.h"
#include "../src/bh_segment_queue.h"
#include "../src/bh_native_attribute.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_vfile.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks,failures,query_calls,query_errors;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL weapon-hud-native: %s\n",s);}}while(0)
typedef struct Images {BHTextureImage *arc,*target;} Images;
typedef struct FrameResult {
    unsigned long pixels,red,green,blue,next_random,polygons,accepted,commands;
    unsigned long arc_words[17][8];
} FrameResult;
static BHTextureImage *__cdecl lookup(unsigned long handle,void *raw) {
    Images *images=(Images *)raw;return handle==1?images->arc:(handle==2?images->target:NULL);
}
/* Isolated observable target receiver, not a production actor/query recovery
   or a callback installed by the restored weapon HUD implementations. */
static void __cdecl position_query(BHEntity *entity,void *raw) {
    BHEntityPositionPacket *packet=(BHEntityPositionPacket *)raw;
    ++query_calls;if(packet->event!=3)++query_errors;packet->position=entity->position;
}
static BHTextureImage *solid(BHTextureContext *textures,const char *name,int red,int green,int blue) {
    BHPcx pcx;unsigned char pixels[1024];memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));
    pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;
    pcx.palette[3]=(unsigned char)red;pcx.palette[4]=(unsigned char)green;pcx.palette[5]=(unsigned char)blue;
    return bh_texture_stage(textures,name,&pcx,2,0);
}
static int equal_files(const char *a,const char *b) {
    FILE *fa=fopen(a,"rb"),*fb=fopen(b,"rb");int ca,cb,ok=1;
    if(!fa || !fb){if(fa)fclose(fa);if(fb)fclose(fb);return 0;}
    do{ca=fgetc(fa);cb=fgetc(fb);if(ca!=cb){ok=0;break;}}while(ca!=EOF);
    fclose(fa);fclose(fb);return ok;
}
static int colors(const char *path,FrameResult *out) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;
    unsigned long x,y,stride;unsigned char rgb[3];out->red=out->green=out->blue=0;
    if(!f)return 0;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || h.bfType!=0x4d42 ||
        info.biBitCount!=24 || info.biWidth!=320 || info.biHeight!=240){fclose(f);return 0;}
    stride=(info.biWidth*3+3)&~3UL;
    for(y=0;y<240;++y){
        if(fseek(f,h.bfOffBits+y*stride,SEEK_SET)){fclose(f);return 0;}
        for(x=0;x<320;++x){
            if(fread(rgb,1,3,f)!=3){fclose(f);return 0;}
            if(rgb[2]>16 && rgb[1]<8 && rgb[0]<8)++out->red;
            if(rgb[1]>16 && rgb[2]<8 && rgb[0]<8)++out->green;
            if(rgb[0]>16 && rgb[1]<8 && rgb[2]<8)++out->blue;
        }
    }
    fclose(f);return 1;
}
static int drain(BHDrawQueue *queue,BHPathQueueContext *path,
    BHPickupDrawContext *rect,Images *images,BHRenderState *state) {
    unsigned long at=0,next,steps=0;
    BHSegmentQueueContext segment;memset(&segment,0,sizeof(segment));
    segment.camera=path->camera;segment.draw=path->draw;segment.lookup=lookup;segment.lookup_user=images;
    while(at!=BH_DRAW_END && steps++<queue->count){
        BHDrawCommand *command;if(at>=queue->count)return 0;command=queue->commands+at;next=command->next;
        switch(command->kind){
        case BH_DRAW_NOP:break;
        case BH_DRAW_STATE:if(!bh_queue_mode_execute(state,command,&next))return 0;break;
        case BH_DRAW_PATH_HEADER:if(!bh_path_queue_execute(path,queue,command,&next))return 0;break;
        case BH_DRAW_SEGMENT:if(!bh_segment_queue_execute(&segment,command,&next))return 0;break;
        case BH_DRAW_RECT:if(!bh_pickup_rect_execute(rect,lookup,images,command,&next))return 0;break;
        /* Path point callbacks are data consumed by their whole header owner. */
        default:return 0;
        }
        at=next;
    }
    return at==BH_DRAW_END;
}
static unsigned long next_random(unsigned long seed,unsigned long consumed) {
    unsigned long i;srand(seed);for(i=0;i<consumed;++i)rand();return (unsigned long)rand();
}
int bh_weapon_hud_native_checkpoint(const char *archive,unsigned long *count) {
    BHMathEnvironment fp;BHSceneView view;BHTextureContext textures;BHRenderMaterial material;BHRenderState state;
    BHNativePrimitive native;BHMainCamera camera;BHScreenClipContext screen;BHAttributeClipContext attribute;
    BHSpriteOrientedContext ribbon;BHSpriteOrientedVertex pool[64];BHPathQueueContext path;BHPickupDrawContext rect;
    BHDrawQueue queue;BHArcContext arc;BHWeaponHudTargetContext reticle;BHWeaponHudState arc_state,target_state;
    BHWeaponHudArcPlayer player,player_before;BHEntity target;Images images;FrameResult result[6];
    unsigned long phase,mode,i,prior,seed,expected_random;int mounted=0,has_arc,has_target;
    char names[6][96];
    checks=failures=0;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));
    memset(&native,0,sizeof(native));memset(&camera,0,sizeof(camera));memset(&queue,0,sizeof(queue));
    memset(&ribbon,0,sizeof(ribbon));memset(pool,0,sizeof(pool));memset(&path,0,sizeof(path));memset(&rect,0,sizeof(rect));
    memset(&arc,0,sizeof(arc));memset(&reticle,0,sizeof(reticle));memset(&player,0,sizeof(player));memset(&target,0,sizeof(target));
    memset(&images,0,sizeof(images));if(!bh_math_environment_enter(&fp))return 2;
    CHECK(bh_scene_view_init(&view,320,240),"actual offscreen native source target without a game/input loop");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"actual texture/cache/format owners");
    if(!textures.device)goto done;textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    images.arc=solid(&textures,"WEAPONARCRED.PCX",255,0,0);images.target=solid(&textures,"WEAPONTARGETGREEN.PCX",0,255,0);
    CHECK(images.arc && images.target,"independent procedural red arc and green reticle positive controls");if(!images.arc || !images.target)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"real native material/state and filtered texture setup");
    view.original_depth=1;
    CHECK(bh_view_config_init(&view.projection,320,240) && bh_main_camera_settings(&view.projection,60,.25,400) &&
        bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) && bh_screen_clip_init(&screen,.25f,400,0,320,0,240) &&
        bh_attribute_clip_init(&attribute,.25f,400,0,320,0,240),"real camera and screen/attribute clipping owners");
    CHECK(bh_draw_queue_init(&queue,128,NULL),"real queue owns complete17-point arc and reticle records");if(!queue.commands)goto done;
    native.textures=&textures;native.near_z=.25f;
    ribbon.path=pool;ribbon.capacity=64;ribbon.near_z=.25f;ribbon.projection_scale=camera.focal;
    ribbon.clip=&attribute;ribbon.color=0xff000000UL;ribbon.emit=bh_native_attribute_emit;ribbon.emit_user=&native;
    path.draw=&ribbon;path.view=&view.projection;path.camera=&camera;path.lookup=lookup;path.lookup_user=&images;
    rect.queue=&queue;rect.camera=&camera;rect.view=&view.projection;rect.clip=&screen;
    rect.emit=bh_native_primitive_emit;rect.emit_user=&native;
    arc.queue=&queue;arc.image=1;reticle.projection=&rect;reticle.viewport_width=320;reticle.atlas_image=2;
    player.position.x=-1.5f;player.position.y=-1;player.position.z=8;player.forward.z=1;
    player.aim.x=.04f;player.aim.y=.02f;player.aim.z=1;player_before=player;
    target.query=position_query;target.position.x=3;target.position.y=2;target.position.z=24;
    for(phase=0;phase<(archive?2UL:1UL);++phase){
        if(phase){
            CHECK(bh_vfile_mount(archive),"actual immutable archive mounted strictly as asset data");mounted=1;
            textures.palette_mode=2;textures.palette_color=0;
            CHECK(bh_texture_set_path(&textures,"textures",2),"startup texture search policy and transparent image binding");
            images.arc=bh_texture_load(&textures,"sray1.bmp");images.target=bh_texture_load(&textures,"tags.bmp");
            CHECK(images.arc && images.target && images.arc->source && images.target->source,
                "actual004474c0 sray1 and00479200 tags assets decode through recovered PCX/cache owners");
            if(!images.arc || !images.target)goto done;
            CHECK(bh_texture_preallocate(&textures) && images.arc->format->count && images.target->format->count,
                "late actual asset images acquire native residency slot ownership");
        }
        memset(result,0,sizeof(result));
        for(mode=0;mode<6;++mode){
            has_arc=mode==1 || mode>=3;has_target=mode==2 || mode>=3;seed=mode==5?918:917;
            sprintf(names[mode],"weapon-hud-native-%s-%lu.bmp",phase?"assets":"control",mode);
            expected_random=next_random(seed,has_arc?65:0);srand(seed);
            query_calls=query_errors=0;path.accepted=0;
            arc_state.target=has_arc?&target:NULL;arc_state.charge=48;
            target_state.target=has_target?&target:NULL;target_state.charge=52;
            CHECK(bh_draw_queue_begin(&queue) && !ribbon.used,"fresh source draw queue and successfully reset shared ribbon pool");
            CHECK(bh_weapon_hud_arc(&arc,&arc_state,&player) && bh_weapon_hud_target_draw(&reticle,&target_state),
                "both complete selected weapon callbacks use actual source query/producer owners");
            CHECK(query_calls==(unsigned long)(has_arc+has_target) && !query_errors && !memcmp(&player,&player_before,sizeof(player)) &&
                arc_state.charge==48 && target_state.charge==52 && arc_state.target==(has_arc?&target:NULL) &&
                target_state.target==(has_target?&target:NULL),"isolated event3 receiver and actual callbacks preserve player/weapon ownership");
            CHECK(arc.path_points==(has_arc?17UL:0UL) && arc.random_calls==(has_arc?65UL:0UL) &&
                arc.attempted==(has_arc?18UL:0UL) && reticle.queued==(has_target?5UL:0UL),"complete17-point path and ready reticle command counts");
            result[mode].commands=queue.count;result[mode].next_random=(unsigned long)rand();
            CHECK(result[mode].next_random==expected_random,"same real CRT seed and exact callback/producer random consumption");
            if(has_arc)for(i=0;i<17;++i)memcpy(result[mode].arc_words[i],&queue.commands[10+i].resource,8*4);
            CHECK(bh_scene_view_begin(&view),"actual native frame begins");prior=native.polygons;
            CHECK(drain(&queue,&path,&rect,&images,&state),"whole queued path consumer plus reticle geometry reach real native emitters");
            result[mode].polygons=native.polygons-prior;result[mode].accepted=path.accepted;
            CHECK(!ribbon.used && (has_arc?path.accepted>=3:!path.accepted),"actual adaptive path consumption and ribbon lifetime reset");
            CHECK(mode?native.polygons>prior:native.polygons==prior,"independent blank-null-target versus visible source polygon controls");
            CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,names[mode]),"actual native flush and offscreen bitmap readback");
            result[mode].pixels=view.pixels;
            CHECK(colors(names[mode],result+mode),"independent native RGB bitmap inspection");
            if(!mode)CHECK(!view.pixels && !result[mode].red && !result[mode].green && !result[mode].blue,"both null-target callbacks produce a genuinely blank image");
            else CHECK(view.pixels>10,"actual selected weapon output covers visible pixels");
            if(!phase && mode==1)CHECK(result[mode].red>10 && !result[mode].green && !result[mode].blue,"red-only arc positive control reaches real path geometry/rasterization");
            if(!phase && mode==2)CHECK(result[mode].green>10 && !result[mode].red && !result[mode].blue,"green-only target positive control reaches real rotated rectangles/rasterization");
            if(!phase && mode>=3)CHECK(result[mode].red>10 && result[mode].green>10 && !result[mode].blue,"both independent native color paths coexist in the composed source image");
        }
        CHECK(equal_files(names[3],names[4]) && result[3].next_random==result[4].next_random &&
            result[3].commands==result[4].commands && result[3].accepted==result[4].accepted &&
            result[3].polygons==result[4].polygons && !memcmp(result[3].arc_words,result[4].arc_words,sizeof(result[3].arc_words)),
            "same CRT seed reproduces actual image, semantic point records and consumer state");
        CHECK(!equal_files(names[0],names[1]) && !equal_files(names[0],names[2]) && !equal_files(names[1],names[3]) &&
            !equal_files(names[2],names[3]),"arc-only, reticle-only, composed and blank outputs are independently distinct");
        CHECK(!equal_files(names[3],names[5]) && memcmp(result[3].arc_words,result[5].arc_words,sizeof(result[3].arc_words)),
            "different real CRT seed changes arc point state and actual raster output");
        CHECK(images.arc->resident && images.arc->resident->handle && images.target->resident && images.target->resident->handle,
            "both selected HUD images use actual owning native texture handles");
        if(phase)CHECK(!equal_files("weapon-hud-native-control-3.bmp",names[3]),"actual startup-named mask images produce different filtered shapes from solid color controls");
        {
            BHVec3 from={-2,1,12},to={3,-1,20};char name[96];FrameResult segment_pixels;
            memset(&segment_pixels,0,sizeof(segment_pixels));
            sprintf(name,"weapon-hud-native-%s-segment.bmp",phase?"assets":"control");
            CHECK(bh_draw_queue_begin(&queue) && !ribbon.used &&
                bh_draw_segment(&queue,3,1,&from,&to,.6f,.2f,1,.25f,0)==1,
                "actual segment producer queues independent taper/modulation payload");
            CHECK(bh_scene_view_begin(&view),"independent segment native frame begins");prior=native.polygons;
            CHECK(drain(&queue,&path,&rect,&images,&state) && !ribbon.used && native.polygons>prior,
                "complete segment append/consumer and corrected real ribbon reach actual native emitter");
            CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,name) && colors(name,&segment_pixels) && view.pixels>10,
                "independent segment geometry produces visible native readback");
            CHECK(!equal_files(names[0],name) && !equal_files(names[1],name),
                "segment output differs from blank and curved weapon path controls");
            if(!phase)CHECK(segment_pixels.red>10 && !segment_pixels.green && !segment_pixels.blue,
                "procedural red image independently verifies native segment texture ownership");
            else CHECK(!equal_files("weapon-hud-native-control-segment.bmp",name),
                "actual sray1 archive texture changes native segment filtered shape");
        }
    }
done:
    bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    if(mounted)bh_vfile_shutdown();CHECK(bh_math_environment_supported(),"native selected weapon composition retains PC53 profile");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"weapon-hud-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_WEAPON_HUD_NATIVE_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_weapon_hud_native_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
