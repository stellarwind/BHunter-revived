#include "../src/bh_render_startup.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL renderer startup %s\n",n);}}while(0)
static LPDIRECTDRAWSURFACE surface(LPDIRECTDRAW draw,unsigned long bits) {
    DDSURFACEDESC d;LPDIRECTDRAWSURFACE p=NULL;memset(&d,0,sizeof(d));
    d.dwSize=sizeof(d);d.dwFlags=0x1007;d.dwWidth=64;d.dwHeight=48;
    d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    d.ddpfPixelFormat.dwSize=sizeof(d.ddpfPixelFormat);
    d.ddpfPixelFormat.dwRGBBitCount=bits;d.ddpfPixelFormat.dwFlags=DDPF_RGB;
    if(bits==8)d.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
    else {d.ddpfPixelFormat.dwRBitMask=0xf800;d.ddpfPixelFormat.dwGBitMask=0x7e0;d.ddpfPixelFormat.dwBBitMask=31;}
    CHECK(IDirectDraw_CreateSurface(draw,&d,&p,NULL)==DD_OK,"actual offscreen color creation");return p;
}
int bh_render_startup_checkpoint(unsigned long *count) {
    LPDIRECTDRAW draw=NULL;LPDIRECTDRAW2 draw2=NULL;LPDIRECTDRAWSURFACE color=NULL;
    BHRenderStartup s;BHTextureContext textures;BHRenderMode mode;HRESULT hr;unsigned long i,bits;
    checks=0;failures=0;memset(&s,0,sizeof(s));memset(&mode,0,sizeof(mode));
    CHECK(bh_render_device_policy(NULL,NULL,NULL,NULL,NULL,NULL)==D3DENUMRET_OK,"exact enumeration policy continues");
    CHECK(DirectDrawCreate(NULL,&draw,NULL)==DD_OK,"real DirectDraw");
    if(!draw)goto end;
    CHECK(IDirectDraw_SetCooperativeLevel(draw,GetDesktopWindow(),DDSCL_NORMAL)==DD_OK,"normal nonexclusive mode");
    memset(&textures,0,sizeof(textures));
    CHECK(!bh_texture_context_init(&textures,draw,NULL,NULL),"native binding rejects null device");
    CHECK(bh_texture_context_init_software(&textures,draw,NULL),"explicit software texture owner");
    CHECK(textures.draw==draw&&!textures.device&&textures.software,"real draw no fabricated device");
    CHECK(bh_texture_formats(&textures),"actual software format initialization without native device");
    for(i=0;i<9;++i)CHECK(textures.formats[i].pixel.dwRGBBitCount==8&&textures.formats[i].pixel.dwFlags==0x60,"all nine original software formats");
    bh_texture_context_dispose(&textures);
    for(bits=8;bits<=16;bits+=8) {
        color=surface(draw,bits);if(!color)continue;
        CHECK(!bh_render_startup_software(&s,draw,color,color,NULL,64,48,32),"software32 branch rejected");
        bh_render_startup_dispose(&s);
        hr=bh_render_startup_software(&s,draw,color,color,bits==8?color:NULL,64,48,bits);
        CHECK(hr,"complete software startup with provided real render surface");
        if(hr) {
            CHECK(s.depth&&s.render==color&&s.software&&!s.owns_render,"depth and borrowed render lifetime");
            CHECK(s.textures.draw==draw&&!s.textures.device&&s.textures.formats[8].pixel.dwRGBBitCount==8,"whole software format dependency");
            CHECK(s.extent_x==64&&s.extent_y==48&&s.clip_x==64&&s.clip_y==48&&!s.scale_x&&!s.scale_y,"original dimensions and zero transform fields");
            CHECK(s.tables.shade_indexed!=NULL,"actual software shade allocation");
            CHECK(bits==8?(s.tables.blend_indexed!=NULL&&s.palette!=NULL):(s.tables.blend_native!=NULL&&s.tables.shade_native!=NULL),"actual indexed palette or native16 tables");
            CHECK(!bh_render_startup_software(&s,draw,color,color,NULL,64,48,bits),"live owner guard");
        } else printf("software native failure bits=%lu hr=%08lx\n",bits,s.last_hresult);
        bh_render_startup_dispose(&s);CHECK(!s.depth&&!s.render&&!s.textures.draw&&!s.tables.shade_indexed,"release entire source owner");
        if(bits==16) {
            hr=bh_render_startup_software(&s,draw,color,NULL,NULL,64,48,16);
            CHECK(hr,"actual original fallback render allocation");
            if(hr)CHECK(s.render&&s.render!=color&&s.owns_render,"owned fallback identity");
            bh_render_startup_dispose(&s);
            hr=bh_render_startup_hardware(&s,draw,color,NULL,64,48,16);
            /* HAL availability is not assumed. All acquired resources must
               be released on a real native failure. */
            CHECK(hr || (!s.depth&&!s.render&&!s.device&&!s.viewport&&!s.textures.draw&&s.last_hresult!=DD_OK),"HAL real result and cleanup");
            printf("hardware startup result=%ld hr=%08lx\n",(long)hr,s.last_hresult);
            bh_render_startup_dispose(&s);
        } else CHECK(!bh_render_startup_hardware(&s,draw,color,NULL,64,48,8),"hardware indexed rejection");
        bh_render_startup_dispose(&s);IDirectDrawSurface_Release(color);color=NULL;
    }
    CHECK(IDirectDraw_QueryInterface(draw,&IID_IDirectDraw2,(void **)&draw2)==DD_OK,"actual display mode interface");
    if(draw2) {
        /* Deliberately impossible display mode: exercise actual native
           rejection without requesting any usable desktop display mode. */
        CHECK(!bh_render_mode_startup(&mode,draw2,draw,0,0,1,1),"invalid display mode rejected");
        CHECK(!mode.screen&&!mode.render&&!mode.engine.render&&mode.last_hresult!=DD_OK,"mode failure owns no buffers");
        CHECK(!strcmp(mode.errors,"Couldn't setup mode:\n0 x 0 1 bit\n"),"exact original mode error category and text");
        bh_render_mode_dispose(&mode);IDirectDraw2_Release(draw2);
    }
    IDirectDraw_Release(draw);
end:
    if(count)*count+=checks;
    printf("{\"scenario\":\"render-startup\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
#ifdef BH_RENDER_STARTUP_STANDALONE
int main(void){unsigned long n=0;return bh_render_startup_checkpoint(&n);}
#endif
