#include "../src/bh_loading_display.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<20)printf("FAIL loading-display: %s\n",s);}}while(0)
static void *__cdecl allocate(unsigned long n,void *u){return malloc(n);}
static void __cdecl release(void *p,void *u){free(p);}
static LPDIRECTDRAWSURFACE surface(LPDIRECTDRAW draw,unsigned long depth){
    DDSURFACEDESC d;LPDIRECTDRAWSURFACE s=NULL;memset(&d,0,sizeof(d));d.dwSize=sizeof(d);
    d.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;d.dwWidth=320;d.dwHeight=240;
    d.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    d.ddpfPixelFormat.dwSize=sizeof(d.ddpfPixelFormat);d.ddpfPixelFormat.dwFlags=DDPF_RGB;
    if(depth==1)d.ddpfPixelFormat.dwFlags|=DDPF_PALETTEINDEXED8;
    d.ddpfPixelFormat.dwRGBBitCount=depth*8;
    if(depth==2){d.ddpfPixelFormat.dwRBitMask=0xf800;d.ddpfPixelFormat.dwGBitMask=0x7e0;d.ddpfPixelFormat.dwBBitMask=31;}
    else if(depth>2){d.ddpfPixelFormat.dwRBitMask=0xff0000;d.ddpfPixelFormat.dwGBitMask=0xff00;d.ddpfPixelFormat.dwBBitMask=0xff;}
    CHECK(SUCCEEDED(IDirectDraw_CreateSurface(draw,&d,&s,NULL)),"actual private system-memory offscreen format created");return s;
}
static void bind_display(BHLoadingDisplay *c,BHLoadingImages *im,LPDIRECTDRAWSURFACE color,LPDIRECTDRAWSURFACE target){
    memset(c,0,sizeof(*c));c->images=im;c->width=320;c->height=240;c->color=color;c->primary=color;
    c->software_target=target;c->back=target;c->software=1;c->previous[0]=c->previous[1]=-1;c->scale=1;
}
static void full_pixels(BHLoadingDisplay *c,long stage){
    DDSURFACEDESC d;unsigned long x,y,k,n,depth=c->images->pixel_bytes;unsigned char *p,*src,*row;
    const unsigned char *indices=stage==0?c->images->midas_indexed:stage==1?c->images->iel_indexed:c->images->indexed;
    const unsigned long *native=stage==0?c->images->midas_native:stage==1?c->images->iel_native:c->images->native;
    int equal=1;memset(&d,0,sizeof(d));d.dwSize=sizeof(d);
    CHECK(SUCCEEDED(IDirectDrawSurface_Lock(c->color,NULL,&d,0,NULL)),"real native full-copy readback lock");if(!d.lpSurface)return;
    src=c->images->arena+(stage==0?0x96000UL:stage==1?0xe1000UL:0);
    row=(unsigned char*)d.lpSurface;
    for(y=0;y<240;++y){p=row;for(x=0;x<320;++x){
        k=*src++;n=native[k];if(depth==1){if(*p++!=indices[k])equal=0;}
        else if(depth==2){if(p[0]!=(unsigned char)n||p[1]!=(unsigned char)(n>>8))equal=0;p+=2;}
        else if(depth==3){if(p[0]!=(unsigned char)(n>>16)||p[1]!=(unsigned char)(n>>8)||p[2]!=(unsigned char)n)equal=0;p+=3;}
        else{if(memcmp(p,&n,4))equal=0;p+=4;}
    }if(y+1<240)row+=d.lPitch;}CHECK(equal,"every full image pixel uses selected stage table, original byte order and native pitch");
    CHECK(SUCCEEDED(IDirectDrawSurface_Unlock(c->color,NULL)),"native readback unlock");
}
static void presented(BHLoadingDisplay *c){CHECK(c->lock_result==DD_OK&&c->unlock_result==DD_OK&&c->copy_result==DD_OK,"real lock/unlock and software BltFast successful");
    CHECK(c->flip_result==DDERR_NOTFLIPPABLE,"actual offscreen Flip returns native NOTFLIPPABLE, no full-screen success claimed");}
static unsigned long digest(LPDIRECTDRAWSURFACE s,unsigned long depth){
    DDSURFACEDESC d;unsigned long x,y,n=2166136261UL;unsigned char *p;memset(&d,0,sizeof(d));d.dwSize=sizeof(d);
    CHECK(SUCCEEDED(IDirectDrawSurface_Lock(s,NULL,&d,0,NULL)),"actual offscreen frame digest lock");if(!d.lpSurface)return 0;
    p=(unsigned char*)d.lpSurface;
    for(y=0;y<240;++y){for(x=0;x<320*depth;++x)n=(n^p[x])*16777619UL;if(y+1<240)p+=d.lPitch;}
    CHECK(SUCCEEDED(IDirectDrawSurface_Unlock(s,NULL)),"actual offscreen digest unlock");return n;
}
static void run_format(LPDIRECTDRAW draw,unsigned long depth,const char *archive){
    BHLoadingImages im;BHLoadingDisplay c;BHSceneAllocator a={allocate,release,NULL};
    unsigned long right[3]={3,2,3},left[3]={11,5,0},i,before,now,bits,old_digest,new_digest;float nan;
    static const float boundaries[5]={0.125f,0.375f,0.625f,0.875f,1.25f};
    LPDIRECTDRAWSURFACE color=surface(draw,depth),target=surface(draw,depth);
    if(!color||!target)goto done;
    if(depth>2){right[0]=right[1]=right[2]=0;left[0]=16;left[1]=8;left[2]=0;}
    CHECK(bh_loading_images_bind(&im,320,depth,right,left,&a),"real image context format binding");
    if(archive){CHECK(bh_loading_images_init(&im),"admitted whole three-image initializer loads immutable archive before actual display consumer");}
    else{im.arena=(unsigned char*)malloc(0x12c000UL);im.arena_bytes=0x12c000UL;im.frame_stride=76800;
        CHECK(im.arena!=NULL,"owned synthetic complete arena");if(im.arena){for(i=0;i<im.arena_bytes;++i)im.arena[i]=(unsigned char)(i*13+i/320);
            for(i=0;i<256;++i){im.indexed[i]=(unsigned char)(i+1);im.midas_indexed[i]=(unsigned char)(i+3);im.iel_indexed[i]=(unsigned char)(i+7);
                im.native[i]=i*0x10203UL;im.midas_native[i]=i*0x20301UL;im.iel_native[i]=i*0x30102UL;}}
        im.stage=0;im.redraw_count=2;}
    if(!im.arena)goto image_done;bind_display(&c,&im,color,target);now=GetTickCount();
    CHECK(!bh_loading_display(&c,0.25f),"whole native logo render completes until legitimate offscreen Flip error");
    CHECK(im.stage==0&&im.redraw_count==1&&(long)(im.deadline-now)>=5000&&c.full_copies==1&&!c.wedge_calls,"stage0 first redraw renews deadline, decrements old count and bypasses wedges");presented(&c);full_pixels(&c,0);
    im.stage=1;im.redraw_count=1;im.deadline=GetTickCount()+5000;
    bh_loading_display(&c,0.5f);CHECK(im.redraw_count==0&&c.full_copies==2,"stage1 retained palette and pending full redraw");presented(&c);full_pixels(&c,1);
    before=c.locks;im.deadline=GetTickCount()-1;bh_loading_display(&c,0.25f);
    CHECK(im.stage==2&&c.locks==before&&c.baseline==0.25f&&c.previous[0]==-1&&c.previous[1]==-1,"old zero redraw skips rendering after transition yet updates baseline, both histories and scale");
    CHECK(c.scale>1.3333332f&&c.scale<1.3333335f,"normalization scale stored as binary32");
    bh_loading_display(&c,0.25f);CHECK(c.full_copies==3&&c.normalized==0,"first BAD history forces whole copy before zero progress");full_pixels(&c,2);
    old_digest=digest(color,depth);CHECK(old_digest==digest(target,depth),"actual software target contains complete copied BAD source frame");
    c.previous[0]=c.previous[1]=0;c.alternate=0;before=c.wedge_calls;
    bh_loading_display(&c,1.0f);CHECK(c.wedge_calls==before+5&&c.normalized==1&&c.previous[1]==1&&c.alternate==1,"complete ordered five-wedge chain and alternating late history updates");presented(&c);
    new_digest=digest(color,depth);CHECK(depth==3?new_digest==old_digest:new_digest!=old_digest,"real geometry changes native1/2/4 pixels while original3byte wedge path leaves full image unchanged");
    CHECK(new_digest==digest(target,depth),"software BltFast publishes actual incremental wedge result");
    for(i=0;i<5;++i){c.previous[0]=c.previous[1]=0;c.baseline=0;c.scale=1;before=c.wedge_calls;
        bh_loading_display(&c,boundaries[i]);CHECK(c.wedge_calls==before+i+1,"exact ordered threshold selects original inclusive segment endpoint and clamps last segment above one");}
    c.previous[0]=c.previous[1]=0.5f;c.baseline=0;c.scale=1;c.alternate=1;before=c.wedge_calls;
    bh_loading_display(&c,0.5f);CHECK(c.wedge_calls==before&&c.alternate==0&&c.previous[0]==0.5f,"equal progress skips wedges but still unlocks/copies/flips");
    bits=0x7fc00000UL;memcpy(&nan,&bits,4);c.previous[1]=nan;c.alternate=0;before=c.full_copies;
    bh_loading_display(&c,0);CHECK(c.full_copies==before+1,"unordered prior progress forces full copy");
    c.previous[0]=0;c.alternate=1;before=c.wedge_calls;bh_loading_display(&c,nan);
    CHECK(c.wedge_calls==before,"unordered current progress skips geometry while preserving native tail");
    im.stage=0;im.redraw_count=0;im.deadline=GetTickCount()-1;before=c.locks;
    bh_loading_display(&c,0.75f);CHECK(im.stage==1&&im.redraw_count==2&&c.locks==before,"stage0 timed transition resets count but oldzero still returns before lock");
    im.stage=0;im.redraw_count=0;im.deadline=GetTickCount()+0x80001000UL;
    bh_loading_display(&c,0);CHECK(im.stage==1&&c.locks==before,"signed wrapped deadline subtraction retains original half-range ordering");
    im.stage=1;im.redraw_count=1;im.deadline=GetTickCount()-1;before=c.wedge_calls;
    bh_loading_display(&c,1);CHECK(im.stage==2&&!_finite(c.scale)&&_isnan(c.normalized)&&c.wedge_calls==before,
        "baseline one retains masked divide-by-zero and unordered normalization instead of inventing a progress clamp");
    im.stage=2;c.primary=NULL;before=c.locks;CHECK(!bh_loading_display(&c,0)&&c.locks==before,"source binding guard rejects missing primary without native dereference");
    c.primary=color;c.color=NULL;CHECK(bh_loading_display(&c,0)&&c.locks==before,"missing native color retains original immediate no-op");c.color=color;
    im.stage=3;CHECK(!bh_loading_display(&c,0)&&c.locks==before,"invalid source stage rejects original uninitialized-table hazard");
image_done:bh_loading_images_dispose(&im);
done:if(target)CHECK(IDirectDrawSurface_Release(target)==0,"private copy surface releases final owned reference");if(color)CHECK(IDirectDrawSurface_Release(color)==0,"private color surface releases final owned reference");
}
int bh_loading_display_checkpoint(const char *archive,unsigned long *count){
    LPDIRECTDRAW draw=NULL;HWND window;unsigned long depth;BHMathEnvironment fp;int entered;
    checks=failures=0;entered=bh_math_environment_enter(&fp);CHECK(entered,"admitted PC53 source math environment");
    window=CreateWindowExA(0,"STATIC","loading-owner",0,0,0,1,1,NULL,NULL,GetModuleHandle(NULL),NULL);
    CHECK(window!=NULL&&SUCCEEDED(DirectDrawCreate(NULL,&draw,NULL)),"private hidden window and actual DirectDraw owner");
    if(draw){CHECK(SUCCEEDED(IDirectDraw_SetCooperativeLevel(draw,window,DDSCL_NORMAL)),"normal cooperative mode without changing display/focus");
        for(depth=1;depth<=4;++depth)run_format(draw,depth,NULL);
        if(archive){CHECK(bh_vfile_mount(archive),"immutable archive mounted as data only");run_format(draw,2,archive);
            CHECK(bh_vfile_detach_archive(),"fixture detaches its own mounted archive");bh_vfile_shutdown();}
        IDirectDraw_Release(draw);}
    if(window)DestroyWindow(window);if(entered)bh_math_environment_leave(&fp);if(count)*count=checks;
    printf("{\"scenario\":\"loading-display\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"ui_capture\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_LOADING_DISPLAY_STANDALONE
int main(int argc,char **argv){return bh_loading_display_checkpoint(argc>1?argv[1]:NULL,NULL);}
#endif


