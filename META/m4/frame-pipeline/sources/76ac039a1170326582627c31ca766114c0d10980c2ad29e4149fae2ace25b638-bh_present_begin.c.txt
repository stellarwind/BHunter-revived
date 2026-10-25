#include "bh_present_begin.h"
#include <string.h>
static int result(BHPresentBegin *c,HRESULT hr) {
 ++c->native_calls;c->last_hresult=hr;if(FAILED(hr)){++c->failures;return 0;}return 1;
}
static int restore(BHPresentBegin *c,LPDIRECTDRAWSURFACE s,int *lost) {
 HRESULT hr;*lost=0;if(!s)return 1;
 hr=IDirectDrawSurface_IsLost(s);++c->native_calls;c->last_hresult=hr;
 if(hr==DDERR_SURFACELOST){*lost=1;++c->restores;return result(c,IDirectDrawSurface_Restore(s));}
 if(FAILED(hr)){++c->failures;return 0;}return 1;
}
int __cdecl bh_present_begin(BHPresentBegin *c) {
 BHPresentContext *p;BHFadeContext *fade;BHTextureContext *textures;
 DDSURFACEDESC desc;BHTextureSlot *slot;unsigned long i;int lost,ok=1;
 if(!c||!(p=c->present)||!(fade=p->fade)||!fade->render||!(textures=fade->render->textures))return 0;
 if(textures->software){if(!p->color||!p->auxiliary||p->locked)return 0;}
 else if(!textures->device)return 0;
 if(!restore(c,p->color,&lost))ok=0;if(!restore(c,p->auxiliary,&lost))ok=0;
 if(textures->software) {
  memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);
  /* Original Lock flags are zero, not DDLOCK_WAIT. */
  if(!result(c,IDirectDrawSurface_Lock(p->color,NULL,&desc,0,NULL)))return 0;
  fade->pixels=(unsigned char*)desc.lpSurface;fade->pitch=desc.lPitch;
  if(!result(c,IDirectDrawSurface_Lock(p->auxiliary,NULL,&desc,0,NULL))) {
   result(c,IDirectDrawSurface_Unlock(p->color,NULL));return 0;
  }
  c->auxiliary_pixels=(unsigned char*)desc.lpSurface;c->auxiliary_pitch=desc.lPitch;p->locked=1;
 } else {
  for(i=0;i<9;++i)for(slot=textures->formats[i].slots;slot;slot=slot->next) {
   if(!slot->surface)return 0;
   if(!restore(c,slot->surface,&lost))ok=0;
   if(lost){if(slot->image)slot->image->resident=NULL;slot->image=NULL;++c->invalidations;}
  }
  if(!result(c,IDirect3DDevice2_BeginScene(textures->device)))ok=0;
 }
 return ok;
}
