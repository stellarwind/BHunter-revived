#include "bh_display_shutdown.h"
void __cdecl bh_display_release(BHDisplayShutdown *c) {
 if(!c)return;
 if(c->viewport){IDirect3DViewport2_Release(c->viewport);c->viewport=NULL;++c->releases;}
 if(c->device){IDirect3DDevice2_Release(c->device);c->device=NULL;++c->releases;}
 if(c->auxiliary){IDirectDrawSurface_Release(c->auxiliary);c->auxiliary=NULL;++c->releases;}
}
int __cdecl bh_display_shutdown(BHDisplayShutdown *c) {
 int ok=1;if(!c)return 0;c->requested=1;
 /*0047d680 is a verified single RET runtime callback; no source stub needed.*/
 bh_display_release(c);
 ++c->messages;if(!PostMessage(c->window,WM_CLOSE,0,0)){++c->failures;ok=0;}
 if(c->log){if(fclose(c->log)){++c->failures;ok=0;}c->log=NULL;++c->files_closed;}
 if(c->delete_temporary) {
  ++c->delete_attempts;if(!c->temporary_first||remove(c->temporary_first)){++c->failures;ok=0;}
  ++c->delete_attempts;if(!c->temporary_second||remove(c->temporary_second)){++c->failures;ok=0;}
 }
 return ok;
}
