#include "../src/bh_radar.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL radar-native: %s\n",s);}}while(0)
static BHTextureImage *__cdecl lookup(unsigned long n,void *u){return n==17 || n==18?(BHTextureImage *)u:NULL;}
static unsigned long pixel(const char *path,unsigned long x,unsigned long y) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;
    if(!f)return 0xffffffffUL;if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || info.biBitCount!=24){fclose(f);return 0xffffffffUL;}
    stride=(info.biWidth*3+3)&~3UL;if(fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}
    fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
int bh_radar_native_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;
    BHDrawQueue queue;BHMainCamera camera;BHClipContext clip;BHScreenClipContext screen;BHLightConfig light;BHSpriteQuadContext quad;BHQuadQueueContext submit;
    BHPickupDrawContext rect;BHRadarFrame frame;BHRadarContext radar;BHPcx pcx;unsigned char pixels[1024];BHMathEnvironment fp;BHVec3 player={0,0,0},target={0,0,0};
    unsigned long x,y,i,mode,next,prior;char name[80];const unsigned long kinds[4]={0,5,2,3},expected[4]={0xff0000UL,0x00ff00UL,0x0000ffUL,0xffff00UL};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&native,0,sizeof(native));memset(&queue,0,sizeof(queue));memset(&camera,0,sizeof(camera));memset(&light,0,sizeof(light));
    CHECK(bh_scene_view_init(&view,64,64),"real native source target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"actual texture lifetime and formats");textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;
    for(y=0;y<32;++y)for(x=0;x<32;++x)pixels[y*32+x]=(unsigned char)(1+(x>=16)+2*(y>=16));
    pcx.palette[3]=255;pcx.palette[7]=255;pcx.palette[11]=255;pcx.palette[12]=pcx.palette[13]=255;
    image=bh_texture_stage(&textures,"RADARFOURCOLORS.PCX",&pcx,0,0);CHECK(image && image->source && image->palette,"actual four-quadrant radar atlas");if(!image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"real native material and state");view.original_depth=1;
    CHECK(bh_view_config_init(&view.projection,64,64) && bh_main_camera_settings(&view.projection,60,.01,100) && bh_main_camera_pose(&view.projection,&camera,0,0,0,0,0,0) &&
        bh_clip_init(&clip,view.projection.near_z,100,0,64,0,64) && bh_screen_clip_init(&screen,view.projection.near_z,100,0,64,0,64),"real world and screen projection/clip owners");
    memset(&quad,0,sizeof(quad));quad.camera=&camera;quad.clip=&clip;quad.directional=&light;quad.base_color=0xff000000UL;quad.emit=bh_native_primitive_emit;quad.emit_user=&native;
    CHECK(bh_light_configure(&light,0,0,0,0,0,0,0,0) && bh_clip_set_camera(&clip,&camera.projected),"real disabled directional light and clip projection bindings");
    submit.draw=&quad;submit.lookup=lookup;submit.lookup_user=image;
    memset(&rect,0,sizeof(rect));rect.view=&view.projection;rect.camera=&camera;rect.clip=&screen;rect.emit=bh_native_primitive_emit;rect.emit_user=&native;
    native.textures=&textures;native.near_z=view.projection.near_z;
    memset(&frame,0,sizeof(frame));frame.origin.z=.1f;frame.axis_x.x=1;frame.axis_y.y=1;frame.normal.z=-1;frame.yaw_x.x=1;frame.yaw_y.z=1;frame.near_z=view.projection.near_z;frame.unit=12;frame.screen_radius=64;frame.screen_x=frame.screen_y=32;
    CHECK(bh_draw_queue_init(&queue,20,NULL) && bh_radar_bind(&radar,&queue,&frame,17,18),"real queue with explicit radar owner bindings");
    for(mode=0;mode<2;++mode){frame.flat=mode;
        for(i=0;i<4;++i){sprintf(name,"radar-native-%lu-%lu.bmp",mode,i);bh_draw_queue_begin(&queue);
            CHECK(bh_radar_marker(&radar,&player,&target,kinds[i],0,0) && queue.count==10 && radar.queued==1,"complete marker reaches selected real queue producer");
            CHECK(bh_scene_view_begin(&view),"native radar scene begins");prior=native.polygons;
            CHECK(mode?bh_pickup_rect_execute(&rect,lookup,image,queue.commands+9,&next):bh_quad_queue_execute(&submit,queue.commands+9,&next),"real screen/world consumer and clipped native quad");
            CHECK(next==(mode?BH_DRAW_END:5) && native.polygons==prior+1 && image->run_count==1,"actual native TL fan and original bucket successor");
            CHECK(bh_render_mode(&state,0) && bh_scene_view_end(&view,name),"real queue flush and offscreen native readback");
            CHECK(pixel(name,32,32)==expected[i] && pixel(name,4,4)==0,"independent red green blue yellow marker quadrant and blank exterior pixels");
        }
    }
    frame.flat=1;frame.visibility_word=8;target.x=1000;bh_draw_queue_begin(&queue);
    CHECK(bh_radar_marker(&radar,&player,&target,1,1,0) && radar.culled==1 && queue.count==9 && bh_scene_view_begin(&view) && bh_scene_view_end(&view,"radar-native-suppressed.bmp") &&
        pixel("radar-native-suppressed.bmp",32,32)==0,"actual suppressed-marker blank-image positive control");
done:
    bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"PC53 retained through native radar source closure");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"radar-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_RADAR_NATIVE_STANDALONE
int main(void){unsigned long count;return bh_radar_native_checkpoint(&count);}
#endif
