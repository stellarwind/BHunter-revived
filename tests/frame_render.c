#include "../src/bh_frame_render.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL frame-render: %s\n",s);}}while(0)
static unsigned long bmp_pixel(const char *path,unsigned long x,unsigned long y){
    FILE *f;BITMAPFILEHEADER file;BITMAPINFOHEADER info;unsigned char rgb[3];
    unsigned long stride,result=0xffffffffUL;
    f=fopen(path,"rb");if(!f)return result;
    if(fread(&file,sizeof(file),1,f)==1 && fread(&info,sizeof(info),1,f)==1 &&
       info.biBitCount==24 && x<(unsigned long)info.biWidth &&
       y<(unsigned long)info.biHeight){
        stride=((unsigned long)info.biWidth*3+3)&~3UL;
        if(!fseek(f,file.bfOffBits+((unsigned long)info.biHeight-1-y)*stride+x*3,SEEK_SET) &&
           fread(rgb,1,3,f)==3)
            result=((unsigned long)rgb[2]<<16)|((unsigned long)rgb[1]<<8)|rgb[0];
    }
    fclose(f);return result;
}
static void check_format(BHSceneView *view,BHTextureContext *textures,
    const BHFrameDisplay *base,unsigned long bits,unsigned long r,unsigned long g,
    unsigned long b,unsigned long expected){
    DDSURFACEDESC surface,locked;LPDIRECTDRAWSURFACE color=NULL;
    BHFrameDisplay display;unsigned long actual=0,epoch;
    memset(&surface,0,sizeof(surface));surface.dwSize=sizeof(surface);
    surface.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    surface.dwWidth=640;surface.dwHeight=480;
    surface.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    surface.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    surface.ddpfPixelFormat.dwFlags=DDPF_RGB;
    surface.ddpfPixelFormat.dwRGBBitCount=bits;
    surface.ddpfPixelFormat.dwRBitMask=r;
    surface.ddpfPixelFormat.dwGBitMask=g;
    surface.ddpfPixelFormat.dwBBitMask=b;
    CHECK(IDirectDraw_CreateSurface(view->draw,&surface,&color,NULL)==DD_OK,
        "real alternate RGB display surface created");
    if(!color)return;
    memset(&surface,0,sizeof(surface));surface.dwSize=sizeof(surface);
    CHECK(IDirectDrawSurface_GetSurfaceDesc(color,&surface)==DD_OK &&
        surface.ddpfPixelFormat.dwRGBBitCount==bits &&
        surface.ddpfPixelFormat.dwRBitMask==r &&
        surface.ddpfPixelFormat.dwGBitMask==g &&
        surface.ddpfPixelFormat.dwBBitMask==b,
        "actual offscreen format matches requested channel masks");
    if(bits==16){
        textures->display_right[0]=textures->display_right[1]=textures->display_right[2]=3;
        textures->display_left[0]=10;textures->display_left[1]=5;textures->display_left[2]=0;
    }else{
        textures->display_right[0]=textures->display_right[1]=textures->display_right[2]=0;
        textures->display_left[0]=16;textures->display_left[1]=8;textures->display_left[2]=0;
    }
    display=*base;display.color=color;display.locked=0;display.left=display.top=0;
    display.indexed_color=0;epoch=(unsigned long)textures->frame;
    CHECK(bh_frame_clear_color(&display,0x12,0x34,0x56) &&
        display.error==BH_FRAME_RENDER_OK &&
        (unsigned long)textures->frame==epoch,
        "software unlocked alternate-format color/depth fills omit native barrier");
    memset(&locked,0,sizeof(locked));locked.dwSize=sizeof(locked);
    CHECK(IDirectDrawSurface_Lock(color,NULL,&locked,DDLOCK_WAIT,NULL)==DD_OK,
        "alternate-format real color surface read back");
    if(locked.lpSurface){
        memcpy(&actual,locked.lpSurface,bits/8);
        CHECK(actual==expected,"actual packed color matches independently known mask value");
        IDirectDrawSurface_Unlock(color,NULL);
    }
    IDirectDrawSurface_Release(color);
}
int bh_frame_render_checkpoint(unsigned long *count){
    BHSceneView view;BHTextureContext textures;BHAtlasContext atlas;BHFrameDisplay display;
    BHMathEnvironment fp;DDSURFACEDESC desc,color,surface;unsigned short *row;
    unsigned long epoch;int depth_locked=0,color_locked=0;
    memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));
    memset(&atlas,0,sizeof(atlas));memset(&display,0,sizeof(display));
    CreateDirectoryA("build",NULL);
    CreateDirectoryA("build/frame-render",NULL);
    CHECK(bh_math_environment_enter(&fp),"caller enters pinned PC53 rounding profile");
    CHECK(bh_scene_view_init(&view,640,480),"real 640x480 RGB DirectDraw/D3D2 offscreen target");
    if(!view.device)goto done;
    CHECK(bh_texture_context_init(&textures,view.draw,view.device,NULL),
        "actual native texture/scene owner bound");
    memset(&surface,0,sizeof(surface));surface.dwSize=sizeof(surface);
    CHECK(IDirectDrawSurface_GetSurfaceDesc(view.color,&surface)==DD_OK &&
        bh_texture_bind_display(&textures,&surface.ddpfPixelFormat),
        "real RGB565 display shifts bound to color surface");
    CHECK(bh_atlas_context_init(&atlas,&textures,0),"shared atlas scan-step owner initialized");
    display.textures=&textures;display.atlas=&atlas;
    display.auxiliary=view.depth;display.color=view.color;
    CHECK(bh_scene_view_begin(&view),"real offscreen scene begun");
    epoch=(unsigned long)textures.frame;
    CHECK(bh_frame_clear_color(&display,255,255,255) &&
        (unsigned long)textures.frame==epoch+2 && display.native_calls==6 &&
        display.error==BH_FRAME_RENDER_OK,
        "complete white clear executes color native barrier then nested depth barrier");
    CHECK(bh_scene_view_end(&view,"build/frame-render/white.bmp") &&
        bmp_pixel("build/frame-render/white.bmp",0,0)==0xffffffUL &&
        bmp_pixel("build/frame-render/white.bmp",320,240)==0xffffffUL &&
        bmp_pixel("build/frame-render/white.bmp",639,479)==0xffffffUL,
        "real native fill covers the entire color target");
    CHECK(bh_scene_view_begin(&view),"second native frame begins");
    epoch=(unsigned long)textures.frame;
    CHECK(bh_frame_clear_depth(&display) && (unsigned long)textures.frame==epoch+1,
        "depth-only native clear performs one full image/device barrier");
    CHECK(bh_scene_view_end(&view,"build/frame-render/depth.bmp") &&
        bmp_pixel("build/frame-render/depth.bmp",320,240)==0,
        "depth-only begin does not invent a color fill");
    textures.software=1;
    epoch=(unsigned long)textures.frame;
    CHECK(bh_frame_clear_depth(&display) && (unsigned long)textures.frame==epoch &&
        display.native_calls==10,
        "software unlocked depth branch uses actual auxiliary Blt without frame barrier");
    memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
    CHECK(IDirectDrawSurface_Lock(view.depth,NULL,&desc,DDLOCK_WAIT,NULL)==DD_OK &&
        ((unsigned short*)((unsigned char*)desc.lpSurface+240*desc.lPitch))[320]==0,
        "software unlocked auxiliary Blt zeroes actual middle depth pixel");
    if(desc.lpSurface)IDirectDrawSurface_Unlock(view.depth,NULL);
    memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
    CHECK(IDirectDrawSurface_Lock(view.depth,NULL,&desc,DDLOCK_WAIT,NULL)==DD_OK,
        "real auxiliary surface locked for software branch");
    if(!desc.lpSurface)goto done;
    depth_locked=1;memset(desc.lpSurface,0x7f,(unsigned long)desc.lPitch*480);
    memset(&color,0,sizeof(color));color.dwSize=sizeof(color);
    CHECK(IDirectDrawSurface_Lock(view.color,NULL,&color,DDLOCK_WAIT,NULL)==DD_OK,
        "real color surface locked for software branch");
    if(!color.lpSurface)goto done;
    color_locked=1;
    display.auxiliary_pixels=(unsigned char*)desc.lpSurface;
    display.auxiliary_pitch=desc.lPitch;
    display.auxiliary_bytes=(unsigned long)desc.lPitch*480;
    display.color_pixels=(unsigned char*)color.lpSurface;
    display.color_pitch=color.lPitch;display.locked=1;
    display.left=20.0f;display.top=30.0f;
    epoch=(unsigned long)textures.frame;
    CHECK(bh_frame_clear_color(&display,255,0,0) &&
        (unsigned long)textures.frame==epoch && display.color_pixels &&
        display.color_pitch>0 && atlas.scan_step==(unsigned long)display.color_pitch,
        "software Unlock/Blt/Lock refreshes shared color pointer and scan pitch");
    color_locked=1;
    row=(unsigned short*)(display.color_pixels+30*display.color_pitch);
    CHECK(row[20]==0xf800 && row[19]!=0xf800,
        "real software color fill begins at converted viewport left/top");
    row=(unsigned short*)(display.auxiliary_pixels+30*display.auxiliary_pitch);
    CHECK(row[20]==0 && row[19]==0x7f7f,
        "real locked 16-bit auxiliary clear preserves pixels before rectangle");
    row=(unsigned short*)(display.auxiliary_pixels+29*display.auxiliary_pitch);
    CHECK(row[20]==0x7f7f,"real locked auxiliary clear preserves preceding row");
    display.indexed_color=1;display.left=40.0f;display.top=50.0f;
    CHECK(bh_frame_clear_color(&display,255,255,255) &&
        display.error==BH_FRAME_RENDER_OK,
        "indexed 6x7x6 color branch completes through real surface and nested depth owner");
    row=(unsigned short*)(display.color_pixels+50*display.color_pitch);
    CHECK(row[40]==251 && row[39]!=251,
        "indexed color formula maps white to 251 at viewport boundary");
    display.left=641.0f;
    CHECK(!bh_frame_clear_depth(&display) && display.error==BH_FRAME_RENDER_BOUNDS,
        "invalid viewport is rejected before unsafe CPU span");
    display.left=20.0f;
    CHECK(!bh_frame_clear_color(&display,-1,255,255) &&
        display.error==BH_FRAME_RENDER_ARGUMENT,
        "out-of-domain signed color rejected before C overflow/shift");
    IDirectDrawSurface_Unlock(view.color,NULL);color_locked=0;
    IDirectDrawSurface_Unlock(view.depth,NULL);depth_locked=0;
    check_format(&view,&textures,&display,16,0x7c00,0x03e0,0x001f,0x08ca);
    check_format(&view,&textures,&display,24,0xff0000,0x00ff00,0x0000ff,0x123456);
    check_format(&view,&textures,&display,32,0xff0000,0x00ff00,0x0000ff,0x123456);
done:
    if(color_locked)IDirectDrawSurface_Unlock(view.color,NULL);
    if(depth_locked)IDirectDrawSurface_Unlock(view.depth,NULL);
    bh_atlas_context_dispose(&atlas);
    bh_texture_context_dispose(&textures);bh_scene_view_dispose(&view);
    CHECK(bh_math_environment_supported(),"caller FP profile survives display owners");
    bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"frame-render\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"play_calls\":0}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_FRAME_RENDER_STANDALONE
int main(void){unsigned long count;return bh_frame_render_checkpoint(&count);}
#endif
