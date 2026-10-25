#include "D:/Decomp/Bhunter/src/bh_fade.h"
#include "D:/Decomp/Bhunter/src/bh_screen_quad.h"
#include "D:/Decomp/Bhunter/src/bh_render_material.h"
#include "D:/Decomp/Bhunter/src/bh_scene_view.h"
#include "D:/Decomp/Bhunter/src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
static void states(LPDIRECT3DDEVICE2 device,const char *phase){DWORD s=99,d=99,a=99;HRESULT hs,hd,ha;hs=IDirect3DDevice2_GetRenderState(device,D3DRENDERSTATE_STIPPLEDALPHA,&s);hd=IDirect3DDevice2_GetRenderState(device,D3DRENDERSTATE_DITHERENABLE,&d);ha=IDirect3DDevice2_GetRenderState(device,D3DRENDERSTATE_ALPHATESTENABLE,&a);printf("DIAG %s stipple=%lu dither=%lu alpha_test=%lu hr=%08lx/%08lx/%08lx\n",phase,s,d,a,hs,hd,ha);}

#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL fade-native: %s\n",s);}}while(0)
static unsigned long pixel(const char *path,unsigned long x,unsigned long y) {
    FILE *f=fopen(path,"rb");BITMAPFILEHEADER h;BITMAPINFOHEADER i;unsigned char rgb[3];unsigned long stride;
    if(!f)return 0xffffffffUL;
    if(fread(&h,sizeof(h),1,f)!=1 || fread(&i,sizeof(i),1,f)!=1 || i.biBitCount!=24){fclose(f);return 0xffffffffUL;}
    stride=(i.biWidth*3+3)&~3UL;
    if(fseek(f,h.bfOffBits+(i.biHeight-1-y)*stride+x*3,SEEK_SET) || fread(rgb,1,3,f)!=3){fclose(f);return 0xffffffffUL;}
    fclose(f);return ((unsigned long)rgb[2]<<16)|((unsigned long)rgb[1]<<8)|rgb[0];
}
int bh_fade_native_checkpoint(unsigned long *count) {
    BHSceneView view;BHTextureContext textures;BHTextureImage *image=NULL;BHPcx pcx;unsigned char pixels[1024];
    BHRenderMaterial material;BHRenderState state;BHNativePrimitive native;BHScreenQuadContext draw;BHScreenClipContext clip;
    BHFadeContext fade;BHDrawQueue queue;BHMathEnvironment fp;unsigned long mode,next,color,zwrite,zfunc,blend,handle,before;D3DDEVICEDESC hw,hel;HRESULT cap_hr;
    static const float amounts[3]={0,1,.5f};static const char *names[3]={"fade-native-identity.bmp","fade-native-red.bmp","fade-native-half.bmp"};
    checks=failures=0;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&material,0,sizeof(material));
    memset(&native,0,sizeof(native));memset(&draw,0,sizeof(draw));memset(&fade,0,sizeof(fade));memset(&queue,0,sizeof(queue));
    if(!bh_math_environment_enter(&fp))return 2;
    CHECK(bh_scene_view_init(&view,64,64),"real native postprocess target");if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL) && bh_texture_formats(&textures),"real native texture formats");
    textures.slot_caps_override=DDSCAPS_TEXTURE|DDSCAPS_SYSTEMMEMORY;
    memset(&pcx,0,sizeof(pcx));memset(pixels,1,sizeof(pixels));pcx.width=pcx.height=32;pcx.pixel_bytes=1024;pcx.pixels=pixels;pcx.palette[5]=255;
    image=bh_texture_stage(&textures,"FADEBLUE.PCX",&pcx,2,0);CHECK(image!=NULL,"independent known blue texture");if(!image)goto done;
    CHECK(!bh_render_material_initialize(&material,&textures,view.d3d) && !material.failures && bh_render_state_init(&state,&textures),"actual material preallocation and render-state owners");
    memset(&hw,0,sizeof(hw));memset(&hel,0,sizeof(hel));hw.dwSize=sizeof(hw);hel.dwSize=sizeof(hel);cap_hr=IDirect3DDevice2_GetCaps(view.device,&hw,&hel);printf("CAPS hr=%08lx hwflags=%08lx helflags=%08lx hw_raster=%08lx hel_raster=%08lx hw_shade=%08lx hel_shade=%08lx hw_src=%08lx hel_src=%08lx hw_dst=%08lx hel_dst=%08lx\n",cap_hr,hw.dwFlags,hel.dwFlags,hw.dpcTriCaps.dwRasterCaps,hel.dpcTriCaps.dwRasterCaps,hw.dpcTriCaps.dwShadeCaps,hel.dpcTriCaps.dwShadeCaps,hw.dpcTriCaps.dwSrcBlendCaps,hel.dpcTriCaps.dwSrcBlendCaps,hw.dpcTriCaps.dwDestBlendCaps,hel.dpcTriCaps.dwDestBlendCaps);
    states(view.device,"initial");
#ifdef DIAG_DISABLE_STIPPLE
    printf("DIAG disable_stipple_hr=%08lx\n",IDirect3DDevice2_SetRenderState(view.device,D3DRENDERSTATE_STIPPLEDALPHA,FALSE));
    states(view.device,"explicit_false");
#endif
    #ifdef DIAG_DISABLE_DITHER
    printf("DIAG disable_dither_hr=%08lx\n",IDirect3DDevice2_SetRenderState(view.device,D3DRENDERSTATE_DITHERENABLE,FALSE));
    states(view.device,"explicit_dither_false");
#endif
    view.original_depth=1;
    CHECK(bh_screen_clip_init(&clip,view.projection.near_z,400,0,64,0,64),"real full-target screen clip");
    native.textures=&textures;native.near_z=view.projection.near_z;
    draw.view=&view.projection;draw.clip=&clip;draw.emit=bh_native_primitive_emit;draw.emit_user=&native;draw.base_color=0xff000000UL;
    fade.render=&state;fade.width=fade.height=64;fade.near_plane=view.projection.near_z;
    CHECK(bh_draw_queue_init(&queue,10,NULL),"real fade state command queue");
    for(mode=0;mode<3;++mode){
        CHECK(bh_draw_queue_begin(&queue) && bh_fade_queue(&queue,amounts[mode],0xff0000UL)==1 && queue.count==10 &&
            queue.commands[8].next==9 && queue.commands[9].next==BH_DRAW_END,"complete fade producer publishes raw state in final bucket8");
        CHECK(bh_fade_execute(&fade,queue.commands+9,&next) && next==BH_DRAW_END && fade.amount==amounts[mode] && fade.color==0xff0000UL,
            "complete consumer updates active fade state before reading continuation");
        CHECK(bh_scene_view_begin(&view),"native fade frame begins");
        CHECK(bh_screen_quad_draw(&draw,image,32,32,64,64,0,0,1) && image->run_count==1,"actual background stays pending until fade barrier");
        states(view.device,"before_fade");before=fade.native_draws;
        CHECK(bh_fade_apply(&fade) && fade.native_draws==before+1 && !image->run_count,"complete native owner flushes pending texture before its actual untextured alpha fan");
        CHECK(IDirect3DDevice2_GetRenderState(view.device,D3DRENDERSTATE_ZWRITEENABLE,&zwrite)==DD_OK &&
            IDirect3DDevice2_GetRenderState(view.device,D3DRENDERSTATE_ZFUNC,&zfunc)==DD_OK &&
            IDirect3DDevice2_GetRenderState(view.device,D3DRENDERSTATE_ALPHABLENDENABLE,&blend)==DD_OK &&
            IDirect3DDevice2_GetRenderState(view.device,D3DRENDERSTATE_TEXTUREHANDLE,&handle)==DD_OK &&
            zwrite==1 && zfunc==7 && blend==0 && handle==0 && state.mode==0 && state.depth==0 && state.blend==0,
            "actual device and recovered state caches restore depth/blend mode0; overlay leaves texture handle0");
        states(view.device,"after_fade");
        CHECK(bh_scene_view_end(&view,names[mode]) && view.pixels==4096,"actual full-target native pixel readback");
        color=pixel(names[mode],32,32);
        if(mode==0)CHECK(color==0x0000ffUL && pixel(names[mode],2,2)==color,"zero amount preserves independently known blue everywhere");
        else if(mode==1)CHECK(color==0xff0000UL && pixel(names[mode],2,2)==color,"full amount produces independently known red everywhere");
        else CHECK(((color>>16)&255)>=120 && ((color>>16)&255)<=136 && (color&255)>=120 && (color&255)<=136 &&
            !((color>>8)&255),"half amount blends red with prior blue within native RGB565 rounding");
    }
done:
    bh_fade_dispose(&fade);bh_draw_queue_dispose(&queue);bh_render_material_dispose(&material);
    bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"postprocess retains PC53 caller profile");bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"fade-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);
    return failures!=0;
}

int main(void){unsigned long n;return bh_fade_native_checkpoint(&n);}
