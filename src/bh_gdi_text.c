#include "bh_gdi_text.h"
#include <string.h>
unsigned long __cdecl bh_draw_command_next(const BHDrawCommand *c){return c->next;}
int __cdecl bh_gdi_text_queue(BHDrawQueue *q,short x,short y,const char *text,unsigned long rgb){BHDrawCommand *c;unsigned long at,tail;const char *end;
 if(!q||!q->commands||!text||q->count<9||q->count>q->capacity)return -1;
 end=(const char*)memchr(text,0,59);if(!end)return -1;
 if(q->count==q->capacity){q->error=BH_DRAW_FULL;return 0;}
 tail=q->commands[8].resource;if(tail>=q->count)return -1;
 at=q->count++;c=q->commands+at;c->next=q->commands[tail].next;q->commands[tail].next=at;q->commands[8].resource=at;c->kind=BH_DRAW_GDI_TEXT;
 c->resource=(unsigned short)x|((unsigned long)(unsigned short)y<<16);
 c->words[0]=((rgb>>16)&255UL)+((rgb&255UL)<<16)+(rgb&0xff00UL);
 strncpy((char*)(c->words+1),text,59);return 1;
}
int __cdecl bh_editor_message_queue(BHDrawQueue *q){return bh_gdi_text_queue(q,0,0,"No objects, press 'N' to create new",0xff0000UL);}
static void native(BHGdiText *c,HRESULT hr){++c->native_calls;c->last_hresult=hr;if(FAILED(hr))++c->failures;}
int __cdecl bh_gdi_text_execute(BHGdiText *c,const BHDrawCommand *command,unsigned long *next){BHFrameDisplay *d;HDC dc=NULL;DDSURFACEDESC desc;HRESULT hr;const char *text;
 if(!c||!(d=c->display)||!d->color||!command||command->kind!=BH_DRAW_GDI_TEXT||!next)return 0;
 text=(const char*)(command->words+1);if(!memchr(text,0,59))return 0;
 if(!c->font)c->font=CreateFontA(24,0,0,0,400,0,0,0,0,0,0,3,2,"Comic Sans MS");
 if(c->relock){hr=IDirectDrawSurface_Unlock(d->color,NULL);native(c,hr);if(SUCCEEDED(hr))d->locked=0;}
 hr=IDirectDrawSurface_GetDC(d->color,&dc);native(c,hr);
 if(SUCCEEDED(hr)){
  SetBkMode(dc,1);SelectObject(dc,c->font);SetTextColor(dc,command->words[0]);
  TextOutA(dc,(short)(command->resource&65535),(short)(command->resource>>16),text,lstrlenA(text));
  hr=IDirectDrawSurface_ReleaseDC(d->color,dc);native(c,hr);
 }
 if(c->relock){memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);hr=IDirectDrawSurface_Lock(d->color,NULL,&desc,0,NULL);native(c,hr);
  if(SUCCEEDED(hr)){d->locked=1;d->color_pixels=(unsigned char*)desc.lpSurface;d->color_pitch=desc.lPitch;if(d->atlas)d->atlas->scan_step=desc.lPitch;}
 }
 *next=bh_draw_command_next(command);return 1;
}
void __cdecl bh_gdi_text_dispose(BHGdiText *c){if(c&&c->font){if(DeleteObject(c->font))c->font=NULL;else ++c->failures;}}
