#include "../src/bh_present_begin.h"
#include "../src/bh_scene_view.h"
#include "../src/bh_fp.h"
#include <string.h>
#include <stdio.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL present-begin: %s\n",s);}}while(0)
int __cdecl bh_present_begin_checkpoint(unsigned long *count) {
 BHSceneView view;BHTextureContext textures;BHRenderState render;BHFadeContext fade;BHPresentContext present;BHPresentBegin begin;
 DDSURFACEDESC desc;BHMathEnvironment fp;unsigned long before,i;BHTextureSlot slots[9];
 checks=failures=0;memset(&view,0,sizeof(view));memset(&textures,0,sizeof(textures));memset(&render,0,sizeof(render));
 memset(&fade,0,sizeof(fade));memset(&present,0,sizeof(present));memset(&begin,0,sizeof(begin));memset(slots,0,sizeof(slots));
 if(!bh_math_environment_enter(&fp))return 2;
 CHECK(!bh_present_begin(NULL)&&!bh_present_begin(&begin),"invalid bindings fail without native calls");
 CHECK(bh_scene_view_init(&view,64,64),"actual private offscreen Direct3D2 target");if(!view.device)goto done;
 textures.device=view.device;render.textures=&textures;fade.render=&render;present.fade=&fade;begin.present=&present;
 present.color=view.color;present.auxiliary=view.depth;
 for(i=0;i<9;++i){slots[i].surface=view.color;textures.formats[i].slots=slots+i;}
 CHECK(bh_present_begin(&begin),"native restores/checks nine slot lists and begins actual scene");
 CHECK(begin.native_calls==12&&begin.invalidations==0&&begin.restores==0,"healthy target auxiliary nine textures and BeginScene count");
 CHECK(bh_present_end(&present),"linked real end-scene completes native frame");
 textures.software=1;fade.amount=0;
 CHECK(bh_present_begin(&begin)&&present.locked,"software locks both actual color and depth surfaces");
 CHECK(fade.pixels&&fade.pitch>=128&&begin.auxiliary_pixels&&begin.auxiliary_pitch>=128,"actual mappings published to explicit borrowers");
 CHECK(!bh_present_begin(&begin),"source guard rejects duplicate software locking");
 memset(fade.pixels,0,fade.pitch*64);
 CHECK(bh_present_end(&present)&&!present.locked,"real matching owner releases both locks");
 memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
 CHECK(IDirectDrawSurface_Lock(view.depth,NULL,&desc,0,NULL)==DD_OK,"prelock actual auxiliary failure control");
 before=begin.failures;
 CHECK(!bh_present_begin(&begin)&&!present.locked&&begin.failures>before,"failed second Lock rolls back actual first Lock");
 CHECK(IDirectDrawSurface_Unlock(view.depth,NULL)==DD_OK,"release failure-control auxiliary lock");
 memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
 CHECK(IDirectDrawSurface_Lock(view.color,NULL,&desc,0,NULL)==DD_OK,"color lock available after rollback");
 CHECK(IDirectDrawSurface_Unlock(view.color,NULL)==DD_OK,"release verified color mapping");
 CHECK(IDirectDrawSurface_Lock(view.color,NULL,&desc,0,NULL)==DD_OK,"prelock actual color first-lock failure control");
 before=begin.native_calls;CHECK(!bh_present_begin(&begin)&&!present.locked&&begin.native_calls==before+3,"first Lock failure stops before auxiliary Lock");
 CHECK(IDirectDrawSurface_Unlock(view.color,NULL)==DD_OK,"release first-lock failure control");
 present.auxiliary=NULL;CHECK(!bh_present_begin(&begin),"software missing auxiliary rejects before side effects");
done:
 bh_scene_view_dispose(&view);bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"present-begin\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false,\"gameplay_executed\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_PRESENT_BEGIN_STANDALONE
int main(void){unsigned long n;return bh_present_begin_checkpoint(&n);}
#endif
