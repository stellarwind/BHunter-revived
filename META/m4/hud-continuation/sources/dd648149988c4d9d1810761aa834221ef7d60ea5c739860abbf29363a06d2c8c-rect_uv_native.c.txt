#include "../src/bh_pickup_draw.h"
#include "../src/bh_render_material.h"
#include "../src/bh_render_state.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL rect-uv-native: %s\n",s);}}while(0)
static BHTextureImage *__cdecl lookup(unsigned long handle,void *user){return handle==17?(BHTextureImage *)user:NULL;}
static unsigned long pixel(const char *path,unsigned long x,unsigned long y) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER info;unsigned char b[3];unsigned long stride;
    if(!f)return 0xffffffffUL;if(fread(&h,sizeof(h),1,f)!=1 || fread(&info,sizeof(info),1,f)!=1 || info.biBitCount!=24){fclose(f);return 0xffffffffUL;}
    stride=(info.biWidth*3+3)&~3UL;if(fseek(f,h.bfOffBits+(info.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(b,1,3,f)!=3){fclose(f);return 0xffffffffUL;}
    fclose(f);return ((unsigned long)b[2]<<16)|((unsigned long)b[1]<<8)|b[0];
}
int bh_rect_uv_native_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;
    BHDrawQueue queue;BHPickupDrawContext draw;BHScreenClipContext clip;BHPcx pcx;unsigned char pixels[1024];BHMathEnvironment fp;
    unsigned long i,x,y,next;const unsigned long expected[4]={0xff0000UL,0x00ff00UL,0x0000ffUL,0xffff00UL};
    checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));memset(&native,0,sizeof(native));memset(&queue,0,sizeof(queue));
    CHECK(bh_scene_view_init(&view,64,64),"actual native source target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real texture context and format enumeration");textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;
    for(y=0;y<32;++y)for(x=0;x<32;++x)pixels[y*32+x]=(unsigned char)(1+(x>=16)+2*(y>=16));
    pcx.palette[3]=255;pcx.palette[7]=255;pcx.palette[11]=255;pcx.palette[12]=pcx.palette[13]=255;
    image=bh_texture_stage(&textures,"RECTFOURCOLORS.PCX",&pcx,0,0);CHECK(image && image->source && image->palette,"real four-color quadrant atlas");if(!image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"real original material and depth state");view.original_depth=1;
    CHECK(bh_view_config_init(&view.projection,64,64) && bh_screen_clip_init(&clip,1,100,0,64,0,64) && bh_draw_queue_init(&queue,16,NULL),"real view clip and command owners");
    memset(&draw,0,sizeof(draw));draw.queue=&queue;draw.view=&view.projection;draw.clip=&clip;draw.emit=bh_native_primitive_emit;draw.emit_user=&native;native.textures=&textures;native.near_z=1;
    CHECK(bh_scene_view_begin(&view),"native scene begins");
    for(i=0;i<4;++i){x=(i&1)?48:16;y=(i&2)?48:16;
        CHECK(bh_pickup_rect_queue(&queue,8,17,(float)x,(float)y,20,20,0,0,1,(i&1)?.625f:.125f,.25f,(i&2)?.625f:.125f,.25f,2)==1,"original raw UV pair-order producer");
        CHECK(bh_pickup_rect_execute(&draw,lookup,image,queue.commands+9+i,&next),"corrected actual consumer native draw");
    }
    CHECK(native.polygons==4 && native.vertices==16 && image->run_count==4,"four real native TL fans");
    CHECK(bh_render_mode(&state,0) && !image->run_count && bh_scene_view_end(&view,"rect-uv-four-colors.bmp"),"actual queue flush and native readback");
    for(i=0;i<4;++i)CHECK(pixel("rect-uv-four-colors.bmp",(i&1)?48:16,(i&2)?48:16)==expected[i],"independent red green blue yellow quadrant pixels distinguish U extent from V start");
    CHECK(pixel("rect-uv-four-colors.bmp",32,32)==0,"between-rectangles pixel remains blank");
done:
    bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"native UV correction retains PC53 profile");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"rect-uv-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_RECT_UV_NATIVE_STANDALONE
int main(void){unsigned long count;return bh_rect_uv_native_checkpoint(&count);}
#endif
