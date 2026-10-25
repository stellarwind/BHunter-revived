#include "../src/bh_loading_progress.h"
#include "../src/bh_vfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks;
static int failures;
#define CHECK(x,label) do { ++checks; if (!(x)) { ++failures; printf("FAIL loading-progress: %s\n",label); } } while (0)
static void u32(unsigned char *p,unsigned long n) {
    p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);
    p[2]=(unsigned char)(n>>16);p[3]=(unsigned char)(n>>24);
}
static int archive_fixture(const char *path) {
    unsigned char bytes[1032];unsigned long i;FILE *file;int ok;
    memset(bytes,0,sizeof(bytes));u32(bytes,4);
    for (i=0;i<4;++i) {
        sprintf((char *)bytes+4+i*256,"bh_loading_progress_%lu.dat",i);
        u32(bytes+4+i*256+248,1028+i);u32(bytes+4+i*256+252,1029+i);
        bytes[1028+i]=(unsigned char)(17+i);
    }
    file=fopen(path,"wb");if (!file) return 0;
    ok=fwrite(bytes,1,sizeof(bytes),file)==sizeof(bytes);
    return fclose(file)==0 && ok;
}
static void access_file(unsigned long index) {
    char name[64];BHVFile *file;unsigned char byte=0;
    sprintf(name,"bh_loading_progress_%lu.dat",index);
    file=bh_vfile_open(name,"rb");CHECK(file!=NULL,"real VFILE embedded open");
    if (file) {
        CHECK(bh_vfile_seek(file,0,SEEK_SET)==0,"position original embedded handle");
        CHECK(bh_vfile_read(&byte,1,1,file)==1 && byte==(unsigned char)(17+index),"real archived payload survives callback");
        CHECK(bh_vfile_close(file)==0,"close real owning archive handle");
    }
}
static int surface(LPDIRECTDRAW dd,LPDIRECTDRAWSURFACE *out) {
    DDSURFACEDESC desc;memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
    desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
    desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    desc.dwWidth=8;desc.dwHeight=8;desc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags=DDPF_RGB;desc.ddpfPixelFormat.dwRGBBitCount=32;
    desc.ddpfPixelFormat.dwRBitMask=0xff0000;desc.ddpfPixelFormat.dwGBitMask=0xff00;
    desc.ddpfPixelFormat.dwBBitMask=0xff;
    return SUCCEEDED(IDirectDraw_CreateSurface(dd,&desc,out,NULL));
}
static void readback(BHLoadingDisplay *display,const unsigned long *palette,
                     unsigned long offset) {
    DDSURFACEDESC desc;HRESULT hr;unsigned long x,y,value;
    memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
    hr=IDirectDrawSurface_Lock(display->color,NULL,&desc,DDLOCK_WAIT,NULL);
    CHECK(SUCCEEDED(hr),"lock actual offscreen callback output");
    if (FAILED(hr)) return;
    for (y=0;y<8;++y) for (x=0;x<8;++x) {
        memcpy(&value,(unsigned char *)desc.lpSurface+y*desc.lPitch+x*4,4);
        CHECK(value==palette[display->images->arena[offset+y*8+x]],"archive callback copies each native MIDAS pixel");
    }
    CHECK(SUCCEEDED(IDirectDrawSurface_Unlock(display->color,NULL)),"unlock actual offscreen output");
}
int bh_loading_progress_checkpoint(unsigned long *count) {
    BHLoadingImages images;BHLoadingDisplay display;BHLoadingProgress owner,other;
    LPDIRECTDRAW dd=NULL;LPDIRECTDRAWSURFACE color=NULL,primary=NULL;
    unsigned long i,calls;HRESULT hr;const char *path="bh_loading_progress_fixture.bin";
    checks=0;failures=0;memset(&images,0,sizeof(images));memset(&display,0,sizeof(display));
    memset(&owner,0,sizeof(owner));memset(&other,0,sizeof(other));
    bh_vfile_shutdown();bh_loading_progress(0.25f);
    CHECK(!bh_loading_progress_install(NULL),"null installation rejected");
    CHECK(!bh_loading_progress_install(&owner),"missing display rejected");
    CHECK(archive_fixture(path),"write isolated source archive");
    CHECK(bh_vfile_mount(path),"mount isolated source archive");
    hr=DirectDrawCreate(NULL,&dd,NULL);CHECK(SUCCEEDED(hr),"create actual DirectDraw owner");
    if (FAILED(hr)) goto done;
    hr=IDirectDraw_SetCooperativeLevel(dd,NULL,DDSCL_NORMAL);
    CHECK(SUCCEEDED(hr),"normal private offscreen cooperative level");if (FAILED(hr)) goto done;
    CHECK(surface(dd,&color),"create actual color surface");
    CHECK(surface(dd,&primary),"create actual offscreen presentation surface");
    if (!color || !primary) goto done;
    images.arena=(unsigned char *)malloc(0x12c000UL);
    CHECK(images.arena!=NULL,"allocate real loading arena");if (!images.arena) goto done;
    images.arena_bytes=0x12c000UL;memset(images.arena,0,images.arena_bytes);
    images.pixel_bytes=4;images.display_width=640;images.frame_stride=64;
    images.redraw_count=2;images.stage=0;
    for (i=0;i<256;++i) images.midas_native[i]=0x120000UL+i*0x101UL;
    for (i=0;i<64;++i) images.arena[0x96000UL+i]=(unsigned char)(i*3+7);
    display.images=&images;display.color=color;display.primary=primary;
    display.width=8;display.height=8;display.scale=1.0f;
    display.previous[0]=display.previous[1]=-1.0f;owner.display=&display;other.display=&display;
    CHECK(bh_loading_progress_install(&owner),"register real cdecl float callback");
    CHECK(!bh_loading_progress_install(&other),"competing callback lifetime rejected");
    access_file(0);
    CHECK(owner.calls==1 && owner.fraction==0.25f,"actual first access forwards exact quarter fraction");
    CHECK(display.calls==1 && display.normalized==0.25f,"real whole display receives forwarded float");
    CHECK(display.full_copies==1 && display.locks==1 && display.unlocks==1 && display.flips==1,"real copy lock unlock and Flip invoked");
    CHECK(FAILED(display.flip_result) && !owner.display_result,"offscreen Flip failure is reported without fabricated success");
    CHECK(images.redraw_count==1 && images.stage==0,"native timed MIDAS stage retains redraw ordering");
    readback(&display,images.midas_native,0x96000UL);
    access_file(0);CHECK(owner.calls==1,"repeat archive access does not repaint");
    bh_loading_progress_remove(&other);access_file(1);
    CHECK(owner.calls==2 && owner.fraction==0.5f && display.calls==2,"nonowner removal preserves registration and exact half fraction");
    CHECK(images.redraw_count==0 && display.full_copies==2,"second owned redraw completes");
    bh_loading_progress_remove(&owner);calls=display.calls;access_file(2);
    CHECK(owner.calls==2 && display.calls==calls,"removed callback receives no further archive access");
    CHECK(bh_loading_progress_install(&owner),"owner can explicitly reinstall");
    access_file(3);
    CHECK(owner.calls==3 && owner.fraction==1.0f && display.calls==calls+1,"complete archive fraction forwards through real display early-return path");
    CHECK(display.full_copies==2 && display.locks==2,"elapsed stage returns after exhausted redraw budget");
done:
    bh_loading_progress_remove(&owner);calls=display.calls;bh_loading_progress(0.75f);
    CHECK(display.calls==calls,"removed direct callback cannot access borrowed state");
    bh_vfile_shutdown();CHECK(remove(path)==0,"remove isolated archive after owning close");free(images.arena);
    if (primary) CHECK(IDirectDrawSurface_Release(primary)==0,"release final native presentation reference");
    if (color) CHECK(IDirectDrawSurface_Release(color)==0,"release final native color reference");
    if (dd) CHECK(IDirectDraw_Release(dd)==0,"release final native DirectDraw reference");
    if (count) *count=checks;
    printf("{\"scenario\":\"loading-progress\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);
    return failures?1:0;
}
#ifdef BH_LOADING_PROGRESS_STANDALONE
int main(void) { return bh_loading_progress_checkpoint(NULL); }
#endif
