#include "../src/bh_primitive_dispatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL primitive dispatch %d\n",__LINE__);}}while(0)
static void rectangle(BHClipVertex *v){
    static const float x[4]={0,0,8,8},y[4]={0,8,8,0};unsigned long i;float scalar=.25f;
    memset(v,0,4*sizeof(*v));for(i=0;i<4;i++){
        v[i].sx=x[i];v[i].sy=y[i];v[i].point.z=2;v[i].next=v+(i+1)%4;v[i].previous=v+(i+3)%4;memcpy(&v[i].unknown28,&scalar,4);
    }
}
int bh_primitive_dispatch_checkpoint(unsigned long *count){
    LPDIRECTDRAW draw=NULL;LPDIRECTDRAWSURFACE surface=NULL;HWND window;
    DDSURFACEDESC desc,locked;BHTextureContext textures;BHTextureImage image;
    BHSoftwareTables tables;BHHorizonColor gradient;BHRenderState state;
    BHPrimitiveDispatch c;BHSoftwareRaster frame;BHClipVertex v[4],saved[4];
    unsigned char pixels[64],dither[1024];unsigned short depth[64],screen[1024];
    unsigned long i,j,k,expected,row,selected,old_locks;float scalar;
    static const unsigned long alpha[5]={0,36,37,219,220};
    static const unsigned long selection[5]={0xffffffffUL,0xffffffffUL,12,12,10};
    HRESULT hr;
    checks=failures=0;memset(&textures,0,sizeof(textures));memset(&tables,0,sizeof(tables));memset(&image,0,sizeof(image));memset(&frame,0,sizeof(frame));
    window=CreateWindowA("STATIC","BHunter source primitive fixture",WS_POPUP,0,0,1,1,NULL,NULL,GetModuleHandleA(NULL),NULL);CHECK(window!=NULL);
    hr=DirectDrawCreate(NULL,&draw,NULL);CHECK(hr==DD_OK&&draw);if(FAILED(hr)||!draw){if(window)DestroyWindow(window);return 1;}
    CHECK(IDirectDraw_SetCooperativeLevel(draw,window,DDSCL_NORMAL)==DD_OK);
    CHECK(bh_texture_context_init_software(&textures,draw,NULL));CHECK(bh_render_state_init(&state,&textures));
    CHECK(bh_software_tables_initialize(&tables,&textures,1,NULL));CHECK(bh_horizon_color_init(&gradient,&textures));CHECK(bh_horizon_color_prepare(&gradient,1,0x204060));
    memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    desc.dwWidth=desc.dwHeight=32;desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat.dwSize=sizeof(desc.ddpfPixelFormat);desc.ddpfPixelFormat.dwFlags=DDPF_RGB|DDPF_PALETTEINDEXED8;desc.ddpfPixelFormat.dwRGBBitCount=8;
    hr=IDirectDraw_CreateSurface(draw,&desc,&surface,NULL);CHECK(hr==DD_OK&&surface);if(FAILED(hr)||!surface)return 1;
    memset(&locked,0,sizeof(locked));locked.dwSize=sizeof(locked);CHECK(IDirectDrawSurface_Lock(surface,NULL,&locked,0,NULL)==DD_OK);
    CHECK(locked.lPitch==32&&locked.dwWidth==32);for(i=0;i<32;i++)memset((char*)locked.lpSurface+i*locked.lPitch,7,32);
    CHECK(IDirectDrawSurface_Unlock(surface,NULL)==DD_OK);
    for(i=0;i<1024;i++){dither[i]=(unsigned char)i;screen[i]=(unsigned short)i;}
    image.source=surface;image.format=textures.formats;image.dither=dither;image.screen=screen;
    frame.pixels=pixels;frame.pitch=8;frame.depth=depth;frame.depth_pitch=16;frame.width=frame.height=8;
    frame.tables=&tables;frame.gradient=&gradient;frame.near_z=1;
    CHECK(bh_primitive_dispatch_init(&c,&state,&frame,0,8,0,8));rectangle(v);memcpy(saved,v,sizeof(v));
    for(k=0;k<3;k++){
        state.depth=k;state.mode=0;memset(pixels,57,sizeof(pixels));for(i=0;i<64;i++)depth[i]=k==2?0xffff:0;
        CHECK(bh_primitive_emit(&c,&image,v,0xffffff));CHECK(c.selected_index==3+k);
        for(i=0;i<64;i++){CHECK(pixels[i]==7);CHECK(depth[i]==(k==1?0:0x8000));}
    }
    state.mode=0;state.depth=0;for(i=0;i<64;i++)depth[i]=0;
    CHECK(bh_primitive_emit(&c,&image,v,0x606060)&&c.selected_index==9&&c.raster.shade_offset==96*256);
    for(i=0;i<64;i++)CHECK(pixels[i]==tables.shade_indexed[96*256+7]);
    for(k=1;k<=3;k++){
        state.mode=k;memset(pixels,57,sizeof(pixels));for(i=0;i<64;i++)depth[i]=0;
        CHECK(bh_primitive_emit(&c,&image,v,0xffffff)&&c.selected_index==12&&c.raster.blend_plane==k-1);
        expected=tables.blend_indexed[(k-1)*65536+57*256+7];for(i=0;i<64;i++)CHECK(pixels[i]==expected&&depth[i]==0);
    }
    for(k=0;k<5;k++){
        state.mode=4;selected=c.selected_index;memset(pixels,57,sizeof(pixels));for(i=0;i<64;i++)depth[i]=0;
        old_locks=c.locks;CHECK(bh_primitive_emit(&c,&image,v,(alpha[k]<<24)|0xffffff));
        CHECK(c.locks==old_locks+1&&c.locks==c.unlocks&&!c.raster.texels);
        if(k<2){CHECK(c.selected_index==selected);for(i=0;i<64;i++)CHECK(pixels[i]==57&&depth[i]==0);}
        else{CHECK(c.selected_index==selection[k]);for(i=0;i<64;i++)CHECK(depth[i]==0);}
    }
    state.mode=0;state.depth=0;c.base_color=0x80000000UL;memset(pixels,57,sizeof(pixels));memset(depth,0,sizeof(depth));
    CHECK(bh_primitive_emit_scalar(&c,&image,v));CHECK(c.raster.shade_offset==63*256);
    CHECK(bh_primitive_emit_gradient(&c,&image,v,0xffffff)&&!c.raster.attribute&&c.raster.attribute_row==191);
    for(i=0;i<64;i++)CHECK(pixels[i]==gradient.indexed[191*256+7]);
    CHECK(!memcmp(v,saved,sizeof(v)));
    /* Native branch exercises real recovered arena growth/append/run ownership;
       it performs no native DrawPrimitive or presentation. */
    textures.software=0;image.vertex_capacity=96;image.runs=(unsigned long*)malloc(128);image.vertices=(D3DTLVERTEX*)malloc(3072);
    CHECK(image.runs&&image.vertices);v[0].unknown28=0x7fc12345UL;
    CHECK(bh_primitive_emit(&c,&image,v,0x12345678)&&image.run_count==1&&image.vertex_count==4&&image.runs[0]==4);
    for(i=0;i<4;i++)CHECK(image.vertices[i].color==0x12345678&&image.vertices[i].specular==0&&image.vertices[i].sz==.5f&&image.vertices[i].rhw==.5f);
    for(i=0;i<4;i++){scalar=(float)i*.25f;memcpy(&v[i].unknown28,&scalar,4);}
    CHECK(bh_primitive_emit_scalar(&c,&image,v)&&image.run_count==2&&image.runs[1]==4);
    CHECK(bh_primitive_emit_gradient(&c,&image,v,0xaabbccdd)&&image.run_count==3&&image.runs[2]==4);
    for(i=0;i<4;i++){
        j=(unsigned long)(long)(-(double)i*.25*255);
        CHECK(image.vertices[4+i].color==0x80000000UL-j*0x10101UL);
        CHECK(image.vertices[8+i].color==0xffffff&&image.vertices[8+i].specular==0xff000000UL+(j<<24));
    }
    free(image.runs);free(image.vertices);textures.software=1;
    CHECK(IDirectDrawSurface_Release(surface)==0);surface=NULL;bh_horizon_color_dispose(&gradient);bh_software_tables_dispose(&tables);
    bh_texture_context_dispose(&textures);CHECK(!textures.draw);CHECK(IDirectDraw_Release(draw)==0);CHECK(DestroyWindow(window));
    if(count)*count=checks;printf("{\"scenario\":\"primitive-dispatch\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_PRIMITIVE_STANDALONE
int main(void){unsigned long count;_controlfp(_PC_53|_RC_NEAR,_MCW_PC|_MCW_RC);return bh_primitive_dispatch_checkpoint(&count);}
#endif
