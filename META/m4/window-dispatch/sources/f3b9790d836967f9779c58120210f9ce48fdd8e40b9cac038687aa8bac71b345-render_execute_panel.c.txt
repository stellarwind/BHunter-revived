#include "../src/bh_render_execute.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_texture.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_queue_reserve.h"
#include "../src/bh_native_primitive.h"
#include "../src/bh_screen_quad.h"
#include "../src/bh_pickup_draw.h"
#include "../src/bh_atlas_text.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL render-execute-panel: %s\n",s);}}while(0)
typedef struct PanelImages {BHTextureImage *items[16];} PanelImages;
static BHTextureImage *__cdecl lookup(unsigned long handle,void *raw){
    PanelImages *images=(PanelImages*)raw;return handle<16?images->items[handle]:NULL;
}
static unsigned long image_pixels(const char *name){
    FILE *file=fopen(name,"rb");BITMAPFILEHEADER header;BITMAPINFOHEADER info;
    unsigned long x,y,pixels=0,stride;unsigned char rgb[3];
    if(!file)return 0;
    if(fread(&header,sizeof(header),1,file)!=1 ||
       fread(&info,sizeof(info),1,file)!=1 || info.biBitCount!=24 ||
       info.biWidth!=640 || info.biHeight!=480){fclose(file);return 0;}
    stride=(640*3+3)&~3UL;
    for(y=0;y<480;++y){
        if(fseek(file,header.bfOffBits+y*stride,SEEK_SET)){fclose(file);return 0;}
        for(x=0;x<640;++x){
            if(fread(rgb,1,3,file)!=3){fclose(file);return 0;}
            if(rgb[0] || rgb[1] || rgb[2])++pixels;
        }
    }
    fclose(file);return pixels;
}
static unsigned long changed_pixels(const char *a,const char *b,
                                    unsigned long *left,unsigned long *top,
                                    unsigned long *right,unsigned long *bottom){
    FILE *fa=fopen(a,"rb"),*fb=fopen(b,"rb");BITMAPFILEHEADER ah,bh;
    BITMAPINFOHEADER ai,bi;unsigned long x,y,changed=0,stride;
    unsigned char pa[3],pb[3];
    *left=640;*top=480;*right=*bottom=0;
    if(!fa || !fb){if(fa)fclose(fa);if(fb)fclose(fb);return 0;}
    if(fread(&ah,sizeof(ah),1,fa)!=1 || fread(&bh,sizeof(bh),1,fb)!=1 ||
       fread(&ai,sizeof(ai),1,fa)!=1 || fread(&bi,sizeof(bi),1,fb)!=1 ||
       ai.biWidth!=640 || ai.biHeight!=480 || bi.biWidth!=640 || bi.biHeight!=480 ||
       ai.biBitCount!=24 || bi.biBitCount!=24){fclose(fa);fclose(fb);return 0;}
    stride=(640*3+3)&~3UL;
    for(y=0;y<480;y++){
        if(fseek(fa,ah.bfOffBits+y*stride,SEEK_SET) ||
           fseek(fb,bh.bfOffBits+y*stride,SEEK_SET))break;
        for(x=0;x<640;x++){
            if(fread(pa,1,3,fa)!=3 || fread(pb,1,3,fb)!=3)break;
            if(memcmp(pa,pb,3)){
                unsigned long visible_y=479-y;
                ++changed;if(x<*left)*left=x;if(x>*right)*right=x;
                if(visible_y<*top)*top=visible_y;
                if(visible_y>*bottom)*bottom=visible_y;
            }
        }
    }
    fclose(fa);fclose(fb);return changed;
}
static unsigned long page_hash(BHAtlasPage *page){
    DDSURFACEDESC lock;unsigned long hash=2166136261UL,x,y,row_bytes;
    const unsigned char *row;
    memset(&lock,0,sizeof(lock));lock.dwSize=sizeof(lock);
    if(!page || !page->surface ||
       IDirectDrawSurface_Lock(page->surface,NULL,&lock,DDLOCK_WAIT|DDLOCK_READONLY,NULL)!=DD_OK)
        return 0;
    row_bytes=page->size*(lock.ddpfPixelFormat.dwRGBBitCount/8);
    for(y=0;y<page->size;y++){
        row=(const unsigned char*)lock.lpSurface+y*lock.lPitch;
        for(x=0;x<row_bytes;x++)hash=(hash^row[x])*16777619UL;
    }
    IDirectDrawSurface_Unlock(page->surface,NULL);return hash;
}
static unsigned long load_sound(BHSoundQueue *queue,BHWave *wave,const char *name){
    BHVFile *file=bh_vfile_open(name,"rb");unsigned long resource=BH_SOUND_INVALID;
    if(file){if(bh_wave_init(wave,NULL) && bh_wave_read(wave,file))
        resource=bh_sound_register_wave(queue,wave,0,3,0);bh_vfile_close(file);}
    return resource;
}
int bh_render_execute_panel_checkpoint(const char *archive,unsigned long *count){
    BHSceneView view;BHTextureContext textures;BHRenderMaterial material;BHRenderState state;
    BHNativePrimitive native;BHDrawQueue draw;BHScreenClipContext clip;
    BHScreenQuadContext quad;BHScreenQuadSubmit screen;BHPickupDrawContext rect;
    BHMainCamera camera;BHAtlasContext atlas;BHAtlasPage *page=NULL;
    BHEntityPool pool;BHMessageState messages;BHMessagePanel panel;BHTextQueue text;
    BHSoundQueue sound;BHWave waves[3];LPDIRECTSOUND device=NULL;
    BHFrameDisplay display;BHRenderExecute executor;
    PanelImages images;BHMathEnvironment fp;BHEntity *entity;BHMessagePanelFields *fields;
    unsigned long i,processed,blank_pixels,typed_pixels,prior;
    unsigned long changed,left,top,right,bottom;
    unsigned long page_blank_hash,page_typed_hash;int mounted=0;
    long first_x,second_x,first_y;
    const char *tv[12]={"tv1.pcx","tv2.pcx","tv3.pcx","tv4.pcx","tv5.pcx",
        "tv6.pcx","tv7.pcx","tv8.pcx","tv9.pcx","tva.pcx","tvb.pcx","tvc.pcx"};
    checks=failures=0;memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));
    memset(&material,0,sizeof(material));memset(&native,0,sizeof(native));
    memset(&draw,0,sizeof(draw));memset(&clip,0,sizeof(clip));
    memset(&quad,0,sizeof(quad));memset(&rect,0,sizeof(rect));
    memset(&camera,0,sizeof(camera));memset(&atlas,0,sizeof(atlas));
    memset(&pool,0,sizeof(pool));memset(&panel,0,sizeof(panel));
    memset(&sound,0,sizeof(sound));memset(waves,0,sizeof(waves));
    memset(&images,0,sizeof(images));bh_message_state_init(&messages);
    memset(&display,0,sizeof(display));memset(&executor,0,sizeof(executor));
    bh_text_queue_init(&text);
    if(!archive){
        if(count)*count=0;
        printf("{\"scenario\":\"render-execute-panel\",\"checks\":0,\"failures\":0,\"archive_skipped\":true,\"original_code_required\":false}\n");
        return 0;
    }
    CHECK(bh_math_environment_enter(&fp),"PC53 source numerical profile");
    mounted=archive?bh_vfile_mount(archive):0;
    CHECK(mounted,"immutable original archive mounted as asset data");
    if(!mounted)goto done;
    {BHVFile *control=bh_vfile_open("TEXTURES\\TV1.PCX","rb");
     CHECK(control!=NULL,"archive contains original startup TV1 image");
     if(!control)goto done;bh_vfile_close(control);}
    CHECK(bh_scene_view_init(&view,640,480),"real 640x480 original-bounds display");
    if(!view.device)goto done;
    view.original_depth=1;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) &&
          bh_texture_formats(&textures),"real texture owner and native formats");
    textures.palette_mode=2;textures.palette_color=0;
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    CHECK(bh_texture_set_path(&textures,"textures",2),"verified original TV image policy/path");
    for(i=0;i<12;i++){
        images.items[i+1]=bh_texture_load(&textures,tv[i]);
        CHECK(images.items[i+1]!=NULL,"actual startup TV1..TVC archive image decoded");
        if(!images.items[i+1])goto done;
    }
    images.items[13]=bh_texture_load(&textures,"tags.bmp");
    CHECK(images.items[13]!=NULL,"actual tags atlas decoded for ring alert");
    if(!images.items[13])goto done;
    CHECK(bh_atlas_context_init(&atlas,&textures,320),"real source atlas owner");
    page=bh_atlas_page_create(&atlas,"textures\\tvfont.pcx",128);
    CHECK(page && page->atlas && page->atlas->rectangle_count>64,
          "actual TVFONT archive atlas and native page");
    if(!page)goto done;
    images.items[14]=bh_atlas_page_image(page);
    CHECK(images.items[14]!=NULL,"panel flash binds actual mutable native text page");
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) &&
          !material.failures && bh_render_state_init(&state,&textures),
          "native material and render state owners");
    CHECK(bh_view_config_init(&view.projection,320,240) &&
          bh_main_camera_settings(&view.projection,60,.25,25000) &&
          bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) &&
          bh_screen_clip_init(&clip,.25f,25000,0,320,0,240),
          "real projection, camera and clipping owners");
    native.textures=&textures;native.near_z=.25f;
    quad.view=&view.projection;quad.clip=&clip;quad.emit=bh_native_primitive_emit;
    quad.emit_user=&native;quad.base_color=0xff000000UL;
    screen.draw=&quad;screen.lookup=lookup;screen.lookup_user=&images;
    rect.view=&view.projection;rect.camera=&camera;rect.clip=&clip;
    rect.emit=bh_native_primitive_emit;rect.emit_user=&native;
    CHECK(bh_draw_queue_init(&draw,64,NULL) && bh_entities_init(&pool,2,NULL),
          "real draw and entity allocations");rect.queue=&draw;
    display.textures=&textures;display.atlas=&atlas;
    display.auxiliary=view.depth;display.color=view.color;
    executor.display=&display;executor.text=&text;executor.atlas=&atlas;
    executor.draw=&draw;executor.state=&state;executor.screen=&screen;
    executor.rect=&rect;executor.image_lookup=lookup;executor.image_user=&images;
    CHECK(DirectSoundCreate(NULL,&device,NULL)==DS_OK && device &&
          IDirectSound_SetCooperativeLevel(device,GetDesktopWindow(),DSSCL_NORMAL)==DS_OK &&
          bh_sound_queue_init(&sound,device,NULL,NULL),
          "real sound interface and source queue without playback");
    if(!device)goto done;
    panel.tv_on_sound=load_sound(&sound,waves+0,"SOUND\\TV_ON.WAV");
    panel.tv_off_sound=load_sound(&sound,waves+1,"SOUND\\TV_OFF.WAV");
    panel.tv_char_sound=load_sound(&sound,waves+2,"SOUND\\TV_CHAR.WAV");
    CHECK(panel.tv_on_sound!=BH_SOUND_INVALID &&
          panel.tv_off_sound!=BH_SOUND_INVALID &&
          panel.tv_char_sound!=BH_SOUND_INVALID,
          "actual startup TV sound assets decoded and registered without playback");
    if(panel.tv_on_sound==BH_SOUND_INVALID || panel.tv_off_sound==BH_SOUND_INVALID ||
       panel.tv_char_sound==BH_SOUND_INVALID)goto done;
    panel.pool=&pool;panel.messages=&messages;panel.text=&text;panel.sound=&sound;
    panel.draw=&draw;panel.page=page;panel.screen_width=320;
    panel.near_depth=view.projection.near_z;panel.flash_image=14;panel.alert_image=13;
    for(i=0;i<12;i++)panel.icon_images[i]=i+1;
    entity=bh_message_panel_create(&panel);fields=bh_message_panel_fields(entity);
    CHECK(entity && fields && entity->refresh==bh_message_panel_refresh,
          "real source panel callback slots and live page/images installed");
    if(!entity)goto done;
    CHECK(bh_message_enqueue(&messages,"5HI\n") && messages.arena_used==5,
          "actual shared message owner feeds live panel arena");
    fields->timer=.01f;
    CHECK(bh_entities_step(&pool) && panel.mode_identity==0x00442b60UL &&
          bh_text_queue_count(&text)==1,"TV startup queues actual page clear");
    CHECK(bh_message_text_drain(&text,&atlas,&processed)==1 && processed==1,
          "real native atlas page clear consumes source queue");
    page_blank_hash=page_hash(page);
    CHECK(page_blank_hash!=0,"native page can be read after source clear");
    fields->timer=.01f;
    CHECK(bh_entities_step(&pool) && panel.mode_identity==0x00442950UL,
          "panel enters source typing method");
    CHECK(bh_texture_preallocate(&textures),"actual TV and page image residency prepared");
    messages.display_ticks=64;
    CHECK(bh_draw_queue_begin(&draw) && bh_entities_refresh(&pool) && draw.count>=12,
          "blank text page and panel ring queue real native draw records");
    CHECK(bh_scene_view_begin(&view),"blank native panel frame begins");prior=native.polygons;
    CHECK(bh_render_execute(&executor) && native.polygons>prior,
          "blank TV and ring consume through whole renderer owner");
    CHECK(bh_render_mode(&state,0) &&
          bh_scene_view_end(&view,"render-execute-panel-blank.bmp"),
          "blank native panel readback saved");
    blank_pixels=image_pixels("render-execute-panel-blank.bmp");
    CHECK(blank_pixels>100,"original TV/ring assets visibly render before typing");
    messages.display_ticks=64;
    CHECK(bh_draw_queue_begin(&draw) && bh_entities_refresh(&pool) &&
          bh_scene_view_begin(&view) && bh_render_execute(&executor) &&
          bh_render_mode(&state,0) &&
          bh_scene_view_end(&view,"render-execute-panel-blank-primed.bmp"),
          "second blank frame verifies stable native texture residency");
    CHECK(image_pixels("render-execute-panel-blank-primed.bmp")>1000,
          "actual TV image is visible in stable blank native frame");
    CHECK(bh_entities_step(&pool) && bh_text_queue_count(&text)==1 &&
          text.commands[text.read&255UL].kind==BH_TEXT_COMMAND_PLAIN &&
          bh_message_text_drain(&text,&atlas,&processed)==1 && processed==1,
          "first plain consumer types H through original current-cursor path");
    first_x=page->cursor_x;first_y=page->cursor_y;
    CHECK(first_x>0,"first actual font glyph advances native page cursor");
    CHECK(bh_entities_step(&pool) && bh_text_queue_count(&text)==1 &&
          bh_message_text_drain(&text,&atlas,&processed)==1 && processed==1,
          "second plain consumer types I without resetting cursor");
    second_x=page->cursor_x;
    CHECK(second_x>first_x && page->cursor_y==first_y,
          "two consecutive glyphs accumulate advances on the same native line");
    CHECK(bh_entities_step(&pool) && bh_text_queue_count(&text)==1 &&
          bh_message_text_drain(&text,&atlas,&processed)==1 && processed==1,
          "plain newline consumes through same source panel FIFO");
    CHECK(page->cursor_x==page->left && page->cursor_y>first_y &&
          panel.sound_requests==3,
          "newline moves to next native line and requests no character sound");
    page_typed_hash=page_hash(page);
    CHECK(page_typed_hash!=0 && page_typed_hash!=page_blank_hash,
          "source typing changes actual native atlas page pixels");
    messages.display_ticks=64;
    CHECK(bh_draw_queue_begin(&draw) && bh_entities_refresh(&pool) && draw.count>=12,
          "typed panel queues identical geometry with changed native page pixels");
    CHECK(bh_message_text_plain(&text,page,"H")==1,
          "live plain panel record queued for renderer FIFO");
    CHECK(bh_scene_view_begin(&view),"typed native panel frame begins");prior=native.polygons;
    CHECK(bh_render_execute(&executor) && native.polygons>prior &&
          executor.text_calls==1 && page->cursor_x>page->left,
          "typed TV, ring and live plain text consume through renderer");
    CHECK(bh_render_mode(&state,0) &&
          bh_scene_view_end(&view,"render-execute-panel-typed.bmp"),
          "typed native panel readback saved");
    typed_pixels=image_pixels("render-execute-panel-typed.bmp");
    changed=changed_pixels("render-execute-panel-blank-primed.bmp",
                           "render-execute-panel-typed.bmp",
                           &left,&top,&right,&bottom);
    CHECK(typed_pixels>100 && changed>0 && left>=64 && right<=165 &&
          top>=70 && bottom<=150,
          "native readback changes only inside flash/page rectangle after two glyphs");
done:
    if(panel.entity)bh_message_panel_close(&panel);
    bh_sound_queue_dispose(&sound);for(i=0;i<3;i++)bh_wave_dispose(waves+i);
    if(device)IDirectSound_Release(device);
    if(page)bh_atlas_page_dispose(&atlas,page);
    bh_atlas_context_dispose(&atlas);bh_texture_context_dispose(&textures);
    bh_draw_queue_dispose(&draw);bh_entities_dispose(&pool);
    bh_scene_view_dispose(&view);if(mounted)bh_vfile_shutdown();
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"render-execute-panel\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_RENDER_EXECUTE_PANEL_STANDALONE
int main(int argc,char **argv){unsigned long count;return bh_render_execute_panel_checkpoint(argc>1?argv[1]:NULL,&count);}
#endif
