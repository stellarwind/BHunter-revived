#include "../src/bh_present.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <string.h>
#include <stdio.h>
#include <float.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL present-native: %s\n",s);}}while(0)
static int lock(LPDIRECTDRAWSURFACE surface,DDSURFACEDESC *d) {
 memset(d,0,sizeof(*d));d->dwSize=sizeof(*d);return IDirectDrawSurface_Lock(surface,NULL,d,DDLOCK_WAIT,NULL)==DD_OK;
}
int __cdecl bh_present_checkpoint(unsigned long *count) {
 BHSceneView view;BHTextureContext textures;BHRenderState render;BHFadeContext fade;BHPresentContext present;
 DDSURFACEDESC color,depth,desc;LPDIRECTDRAWSURFACE target=NULL;BHMathEnvironment fp;
 unsigned long i,x,y,before,raw;unsigned short *row;float amounts[4]={0,-1,1,0};
 checks=failures=0;memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&fade,0,sizeof(fade));memset(&present,0,sizeof(present));
 if(!bh_math_environment_enter(&fp))return 2;
 CHECK(bh_scene_view_init(&view,64,64),"actual offscreen Direct3D2/DirectDraw owners");if(!view.device)goto done;
 textures.device=view.device;CHECK(bh_render_state_init(&render,&textures),"real render caches bound to borrowed actual device");
 fade.render=&render;fade.width=fade.height=64;fade.near_plane=view.projection.near_z;present.fade=&fade;
 raw=0x7fc12345;memcpy(amounts+3,&raw,4);
 for(i=0;i<4;++i) {
  CHECK(bh_scene_view_begin(&view),"actual native scene begin");
  before=present.fade_calls;bh_fade_set(&fade,amounts[i],0xff0000);
  if(i==3)_clear87();
  CHECK(bh_present_end(&present),"complete native end-frame owner");view.begun=0;
  if(i==3){CHECK((_status87()&_SW_INVALID)!=0,"unordered raw FCOMP gate retains actual masked x87 invalid sticky");_clear87();}
  CHECK(present.fade_calls==before+(i==2),"ordered positive fade gate zero negative and unordered controls");
  CHECK(lock(view.color,&color),"actual native color readback after EndScene");
  CHECK(((unsigned short*)color.lpSurface)[32+32*color.lPitch/2]==(i==2?0xf800:0),"actual fade-before-EndScene red vs clear-black control");
  CHECK(IDirectDrawSurface_Unlock(view.color,NULL)==DD_OK,"native readback unlock");
 }
 memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);desc.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;
 desc.dwWidth=desc.dwHeight=64;desc.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
 desc.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);desc.ddpfPixelFormat.dwFlags=DDPF_RGB;desc.ddpfPixelFormat.dwRGBBitCount=16;
 desc.ddpfPixelFormat.dwRBitMask=0xf800;desc.ddpfPixelFormat.dwGBitMask=0x7e0;desc.ddpfPixelFormat.dwBBitMask=31;
 CHECK(IDirectDraw_CreateSurface(view.draw,&desc,&target,NULL)==DD_OK,"actual optional copy destination");if(!target)goto done;
 textures.software=1;present.auxiliary=view.depth;present.color=view.color;
 fade.pixel_bytes=2;fade.masks[0]=0xf800;fade.masks[1]=0x7e0;fade.masks[2]=31;fade.extract[0]=11;fade.extract[1]=5;fade.expand[0]=fade.expand[2]=3;fade.expand[1]=2;
 for(i=0;i<3;++i) {
  CHECK(lock(view.color,&color)&&lock(view.depth,&depth),"actual color and auxiliary surfaces locked before software frame end");
  for(y=0;y<64;++y){row=(unsigned short*)((unsigned char*)color.lpSurface+y*color.lPitch);for(x=0;x<64;++x)row[x]=31;}
  fade.pixels=(unsigned char*)color.lpSurface;fade.pitch=color.lPitch;fade.cached_amount=0;fade.cached_color=0;
  bh_fade_set(&fade,i==2?1.0f:0.0f,0xff0000);present.target=i?target:NULL;present.locked=1;before=present.copies;
  CHECK(bh_present_end(&present)&&!present.locked,"software owner fades then unlocks both actual surfaces and clears live lock state");
  CHECK(present.copies==before+(i!=0)&&present.restores==0,"optional copy gate and healthy-surface no-restore path");
  CHECK(lock(i?target:view.color,&color),"actual presented surface readback");
  CHECK(((unsigned short*)color.lpSurface)[32+32*color.lPitch/2]==(i==2?0xf800:31),"software fade occurs before actual whole-surface BltFast");
  CHECK(IDirectDrawSurface_Unlock(i?target:view.color,NULL)==DD_OK,"software destination readback unlock");
 }
 before=present.failures;present.target=NULL;fade.amount=0;present.locked=1;
 CHECK(!bh_present_end(&present)&&!present.locked&&present.failures>=before+2,"actual Unlock failures continue second Unlock and clear lock state");
 CHECK(present.native_calls>=20&&!present.restores,"all real COM paths instrumented without fabricated lost surfaces");
done:
 if(target)IDirectDrawSurface_Release(target);bh_fade_dispose(&fade);bh_scene_view_dispose(&view);
 CHECK(bh_math_environment_supported(),"presentation preserves PC53 caller profile");bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"present-native\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_PRESENT_STANDALONE
int main(void){unsigned long n;return bh_present_checkpoint(&n);}
#endif

