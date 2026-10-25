#include "../src/bh_capture.h"
#include "../src/bh_present_begin.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL capture: %s\n",s);}}while(0)
static unsigned long load(unsigned char *out,unsigned long cap) {
 FILE *f=fopen("capture-synthetic-fixture.bmp","rb");unsigned long n;if(!f)return 0;n=fread(out,1,cap,f);fclose(f);remove("capture-synthetic-fixture.bmp");return n;
}
int __cdecl bh_capture_checkpoint(unsigned long *count) {
 BHSceneView view;BHTextureContext textures;BHRenderState render;BHFadeContext fade;BHPresentContext present;BHCapture capture;BHPresentBegin begin;
 BHMathEnvironment fp;unsigned char pixels[4096],actual[4096];unsigned long i,n,x,y,before;unsigned short *row;DDSURFACEDESC desc;
 checks=failures=0;memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&render,0,sizeof(render));memset(&fade,0,sizeof(fade));memset(&present,0,sizeof(present));memset(&capture,0,sizeof(capture));memset(&begin,0,sizeof(begin));
 if(!bh_math_environment_enter(&fp))return 2;
 textures.software=1;render.textures=&textures;fade.render=&render;present.fade=&fade;capture.present=&present;fade.pixels=pixels;capture.pixel_bytes=1;
 capture.left=1.75f;capture.top=1.75f;capture.right=257.75f;capture.bottom=2.75f;fade.pitch=260;
 for(i=0;i<260;++i)pixels[260+i]=(unsigned char)(i-1);
 CHECK(bh_capture_software(&capture,"capture-synthetic-fixture.bmp"),"all256 source palette indexes through complete software owner");
 n=load(actual,sizeof(actual));CHECK(n==54+256*3,"original header plus packed unpadded row");
 CHECK(actual[2]==0x36&&actual[3]==0x10&&actual[4]==0x0e&&actual[34]==0&&actual[35]==0x10,"preserved original fixed image-size/header fields");
 for(i=0;i<256;++i)CHECK(actual[54+i*3]==(i%42%6)*255/5&&actual[55+i*3]==(i%42/6)*255/6&&actual[56+i*3]==(unsigned char)((i/42)*255/5),"independent palette quotient/remainder channels");
 capture.left=capture.top=0;capture.right=3;capture.bottom=2;capture.pixel_bytes=2;capture.masks[0]=31;capture.masks[1]=0x7e0;capture.masks[2]=0xf800;capture.extract[0]=0;capture.extract[1]=5;capture.extract[2]=11;capture.expand[0]=capture.expand[2]=3;capture.expand[1]=2;fade.pitch=8;
 for(y=0;y<2;++y)for(x=0;x<3;++x){row=(unsigned short*)(pixels+y*8);row[x]=(unsigned short)(y?0xf800:31);}
 CHECK(bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"software dispatch includes16-bit conversion");n=load(actual,sizeof(actual));
 CHECK(n==72&&actual[54]==0&&actual[56]==248&&actual[63]==248&&actual[65]==0,"bottom-up rows crop pitch and retain no padding");
 capture.pixel_bytes=4;fade.pitch=12;capture.masks[0]=255;capture.masks[1]=0xff00;capture.masks[2]=0xff0000;capture.extract[1]=8;capture.extract[2]=16;capture.expand[0]=capture.expand[1]=capture.expand[2]=0;
 for(i=0;i<6;++i)((unsigned long*)pixels)[i]=0x00332211+i;
 CHECK(bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"software32-bit conversion");n=load(actual,sizeof(actual));CHECK(n==72&&actual[54]==0x14&&actual[55]==0x22&&actual[56]==0x33,"independent32-bit expected channels");
 CHECK(!bh_capture_write(&capture,"missing-capture-dir/file.bmp"),"real file-open failure");
 capture.right=1281;CHECK(!bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"source guard rejects original fixed row overflow");capture.right=3;
 CHECK(bh_scene_view_init(&view,64,64),"actual private offscreen native owner");if(!view.device)goto done;
 textures.software=0;textures.device=view.device;present.color=view.color;present.auxiliary=view.depth;begin.present=&present;
 capture.pixel_bytes=2;capture.masks[0]=31;capture.masks[1]=0x7e0;capture.masks[2]=0xf800;capture.extract[1]=5;capture.extract[2]=11;capture.expand[0]=capture.expand[2]=3;capture.expand[1]=2;
 memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);CHECK(IDirectDrawSurface_Lock(view.color,NULL,&desc,0,NULL)==DD_OK,"write synthetic actual target pixels");
 for(y=0;y<64;++y){row=(unsigned short*)((unsigned char*)desc.lpSurface+y*desc.lPitch);for(x=0;x<64;++x)row[x]=0x7e0;}
 CHECK(IDirectDrawSurface_Unlock(view.color,NULL)==DD_OK,"unlock synthetic target pixels");
 CHECK(bh_present_begin(&begin),"real begin before native capture barrier");before=textures.frame;
 CHECK(bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"complete native capture EndScene BeginScene Lock write Unlock");n=load(actual,sizeof(actual));
 CHECK(n==72&&actual[54]==0&&actual[55]==252&&actual[56]==0,"actual offscreen fixture native packed BMP pixels");
 CHECK(textures.frame==before+1&&capture.native_calls==4,"native residency epoch and COM sequence");
 textures.frame=0x7fffffffL;
 CHECK(bh_capture_write(&capture,"capture-synthetic-fixture.bmp")&&textures.frame==(-2147483647L-1L),"native residency epoch explicitly wraps signed32-bit maximum");load(actual,sizeof(actual));
 capture.right=65;
 CHECK(!bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"native crop outside actual locked descriptor width rejected");capture.right=3;
 capture.bottom=65;
 CHECK(!bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"native crop outside actual locked descriptor height rejected");capture.bottom=2;
 CHECK(bh_capture_write(&capture,"capture-synthetic-fixture.bmp"),"native bounds failures unlock actual surface for subsequent valid write");load(actual,sizeof(actual));
 CHECK(!bh_capture_write(&capture,"missing-capture-dir/file.bmp"),"native open failure releases actual locked surface");
 CHECK(bh_present_end(&present),"native scene remains usable after capture error");
done:
 bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"capture\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false,\"desktop_capture\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_CAPTURE_STANDALONE
int main(void){unsigned long n;return bh_capture_checkpoint(&n);}
#endif
