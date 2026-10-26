#include "../src/bh_render_execute.h"
#include "../src/bh_scene_view.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL render-execute: %s\n",s);}}while(0)

int bh_render_execute_checkpoint(unsigned long *count){
    BHDrawQueue queue;BHTextQueue text;BHTextureContext textures;
    BHRenderState state;BHFrameDisplay display;BHRenderExecute execute;
    BHSceneView view;BHViewConfig projection;BHMainCamera camera;
    BHVec3 position,angles;DDPIXELFORMAT pixel;float valid_fov;BHLightConfig light;BHGdiText gdi;
    unsigned char *depth;FILE *bmp;unsigned char rgb[3];
    memset(&queue,0,sizeof(queue));memset(&text,0,sizeof(text));
    memset(&textures,0,sizeof(textures));memset(&state,0,sizeof(state));
    memset(&display,0,sizeof(display));memset(&execute,0,sizeof(execute));
    memset(&view,0,sizeof(view));memset(&projection,0,sizeof(projection));
    memset(&camera,0,sizeof(camera));checks=failures=0;
    depth=(unsigned char *)malloc(640UL*480UL*2UL);
    CHECK(depth!=NULL,"software depth storage");if(!depth)goto done;
    memset(depth,0xa5,640UL*480UL*2UL);
    CHECK(bh_draw_queue_init(&queue,24,NULL)&&bh_draw_queue_begin(&queue),"real source bucket headers");
    if(!queue.commands)goto done;
    position.x=1;position.y=2;position.z=3;
    angles.x=angles.y=angles.z=0;
    CHECK(bh_view_config_init(&projection,640,480)&&
          bh_main_camera_settings(&projection,60,.01,25000)&&
          bh_camera_queue_pose(&queue,&position,&angles)==1,
          "actual camera command installed in bucket zero");
    bh_text_queue_init(&text);text.commands[0].kind=BH_TEXT_COMMAND_CLEAR;
    text.commands[0].page=NULL;text.write=1;
    textures.software=1;state.textures=&textures;
    display.textures=&textures;display.auxiliary=(LPDIRECTDRAWSURFACE)1;
    display.auxiliary_pixels=depth;display.auxiliary_bytes=640UL*480UL*2UL;
    display.auxiliary_pitch=1280;display.locked=1;
    execute.display=&display;execute.text=&text;execute.draw=&queue;
    execute.state=&state;execute.view=&projection;execute.camera=&camera;
    CHECK(bh_render_execute(&execute),"complete software queue executor");
    CHECK(execute.draw_calls==10&&execute.text_calls==1&&text.read==1,"camera, headers and ring drained");
    CHECK(execute.kinds[BH_DRAW_NOP]==3&&execute.kinds[BH_DRAW_STATE]==6,"all original bucket mode identities");
    CHECK(execute.kinds[BH_DRAW_CAMERA]==1&&camera.position.x==1&&camera.position.z==3,
          "real camera owner applied queued pose");
    CHECK(textures.frame==0&&execute.native_calls==0,"software tail skips native epoch");
    CHECK(depth[0]==0&&depth[640UL*480UL*2UL-1]==0,"full fixed depth rectangle cleared");
    valid_fov=projection.fov;projection.fov=0;
    CHECK(!bh_render_execute(&execute)&&execute.error==BH_RENDER_EXECUTE_CALLBACK,
          "invalid camera callback failure recorded");
    CHECK(execute.draw_calls==10&&execute.kinds[BH_DRAW_STATE]==6,
          "valid callback failure does not suppress later draw records");
    projection.fov=valid_fov;
    memset(&light,0,sizeof(light));execute.light=&light;
    CHECK(bh_menu_light_queue(&queue,0,1,2,3,4,5,6,7,8)==1,"real menu light producer");
    CHECK(bh_render_execute(&execute)&&execute.kinds[BH_DRAW_LIGHT]==1,"menu light through actual render executor");
    execute.light=NULL;CHECK(!bh_render_execute(&execute)&&execute.error==BH_RENDER_EXECUTE_BINDING,"light requires actual source binding");
    CHECK(bh_draw_queue_begin(&queue)&&bh_camera_queue_pose(&queue,&position,&angles)==1,"restore camera queue after light");
    /* The original 32-bit epoch wraps. Source-bound native path is exercised
       below; software mode deliberately preserves it. */
    queue.commands[8].next=10;queue.commands[10].kind=BH_DRAW_PATH_POINT;
    queue.commands[10].next=BH_DRAW_END;queue.count=11;
    CHECK(!bh_render_execute(&execute)&&execute.error==BH_RENDER_EXECUTE_SELECTOR,
          "contiguous path payload cannot masquerade as callback");
    queue.commands[8].next=BH_DRAW_END;queue.count=10;
    CHECK(bh_scene_view_init(&view,640,480),"real offscreen Direct3D2 display");
    if(!view.device)goto done;
    memset(&pixel,0,sizeof(pixel));pixel.dwSize=sizeof(pixel);
    pixel.dwFlags=DDPF_RGB;pixel.dwRGBBitCount=16;
    pixel.dwRBitMask=0xf800;pixel.dwGBitMask=0x7e0;pixel.dwBBitMask=0x1f;
    CHECK(bh_texture_bind_display(&textures,&pixel),"actual RGB565 display color shifts");
    CHECK(bh_scene_view_begin(&view),"actual scene begun");
    textures.software=0;textures.device=view.device;
    display.auxiliary=view.depth;display.color=view.color;display.locked=0;
    execute.display_mode=1;textures.frame=-1;
    CHECK(bh_render_execute(&execute),"white clear, draw chain and native tail");
    CHECK(textures.frame==8&&execute.native_calls==2&&display.native_calls>=6,
          "six mode callbacks, nested clears and tail each cycled the device");
    CHECK(execute.draw_calls==10&&execute.failures==0,"native callbacks all completed");
    CHECK(bh_scene_view_end(&view,"render-execute-white.bmp"),"native readback after executor");
    bmp=fopen("render-execute-white.bmp","rb");
    CHECK(bmp!=NULL,"native output exists");
    if(bmp){fseek(bmp,54,SEEK_SET);CHECK(fread(rgb,1,3,bmp)==3&&
        rgb[0]==255&&rgb[1]==255&&rgb[2]==255,"white selector visible in native pixels");fclose(bmp);}
    memset(&gdi,0,sizeof(gdi));gdi.display=&display;execute.gdi=&gdi;
    textures.software=1;display.auxiliary_pixels=depth;display.auxiliary_bytes=640UL*480UL*2UL;display.auxiliary_pitch=1280;
    CHECK(bh_draw_queue_begin(&queue)&&bh_gdi_text_queue(&queue,10,10,"Source queue",0xffffff)==1,"GDI text queued for complete executor");
    CHECK(bh_render_execute(&execute)&&execute.kinds[BH_DRAW_GDI_TEXT]==1&&gdi.native_calls==2&&!gdi.failures,"real executor dispatches native GDI consumer");
    execute.gdi=NULL;CHECK(!bh_render_execute(&execute)&&execute.error==BH_RENDER_EXECUTE_BINDING,"GDI consumer requires source owner");
    bh_gdi_text_dispose(&gdi);
done:
    if(view.device)bh_scene_view_dispose(&view);
    if(queue.commands)bh_draw_queue_dispose(&queue);
    free(depth);
    if(count)*count=checks;
    printf("{\"scenario\":\"render-execute\",\"checks\":%lu,\"failures\":%lu}\n",checks,failures);
    return failures?1:0;
}
#ifdef BH_RENDER_EXECUTE_STANDALONE
int main(void){return bh_render_execute_checkpoint(NULL);}
#endif
