#include "../src/bh_gdi_text.h"
#include "../src/bh_scene_view.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL GDI text %s\n",n);}}while(0)
int bh_gdi_text_checkpoint(unsigned long *count){BHDrawQueue q;BHSceneView view;BHFrameDisplay display;BHGdiText c;DDSURFACEDESC desc;BHDrawCommand *command;LOGFONTA font;unsigned long next,at;char longtext[60];int x,y,pixels=0;HRESULT hr;
 checks=0;failures=0;memset(&q,0,sizeof(q));memset(&view,0,sizeof(view));memset(&display,0,sizeof(display));memset(&c,0,sizeof(c));CHECK(bh_draw_queue_init(&q,16,NULL)&&bh_draw_queue_begin(&q),"queue");
 CHECK(bh_gdi_text_queue(&q,-2,12,"Native text",0x123456)==1,"producer");command=q.commands+9;CHECK(command->kind==BH_DRAW_GDI_TEXT&&(short)command->resource==-2&&(short)(command->resource>>16)==12&&command->words[0]==0x563412,"signed XY and RGB conversion");CHECK(!strcmp((char*)(command->words+1),"Native text"),"owned string");
 CHECK(bh_editor_message_queue(&q)==1&&q.commands[10].words[0]==255&&q.commands[10].resource==0&&!strcmp((char*)(q.commands[10].words+1),"No objects, press 'N' to create new"),"editor message original text red at origin");CHECK(bh_draw_command_next(command)==10&&bh_draw_command_next(q.commands+10)==BH_DRAW_END,"whole next accessor");memset(longtext,'X',59);longtext[59]=0;CHECK(bh_gdi_text_queue(&q,0,0,longtext,0)==-1,"unterminated original field guard");
 CHECK(bh_scene_view_init(&view,160,80),"real offscreen surfaces");if(view.color){DDBLTFX fill;memset(&fill,0,sizeof(fill));fill.dwSize=sizeof(fill);CHECK(SUCCEEDED(IDirectDrawSurface_Blt(view.color,NULL,NULL,NULL,DDBLT_COLORFILL|DDBLT_WAIT,&fill)),"clear real surface");display.color=view.color;c.display=&display;
  CHECK(bh_gdi_text_execute(&c,command,&next)&&next==10&&c.font!=NULL&&c.native_calls==2&&!c.failures,"actual GetDC text ReleaseDC");CHECK(GetObjectA(c.font,sizeof(font),&font)==sizeof(font)&&font.lfHeight==24&&font.lfWeight==400&&!strcmp(font.lfFaceName,"Comic Sans MS"),"exact original lazy font");
  memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);hr=IDirectDrawSurface_Lock(view.color,NULL,&desc,DDLOCK_WAIT,NULL);CHECK(SUCCEEDED(hr),"lock native text readback");if(SUCCEEDED(hr)){unsigned long bpp=desc.ddpfPixelFormat.dwRGBBitCount/8;for(y=0;y<80;++y)for(x=0;x<160;++x){unsigned long i;for(i=0;i<bpp;++i)if(((unsigned char*)desc.lpSurface)[y*desc.lPitch+x*bpp+i])++pixels;}CHECK(pixels>0,"native GDI pixels drawn");IDirectDrawSurface_Unlock(view.color,NULL);}
  memset(&desc,0,sizeof(desc));desc.dwSize=sizeof(desc);hr=IDirectDrawSurface_Lock(view.color,NULL,&desc,DDLOCK_WAIT,NULL);CHECK(SUCCEEDED(hr),"caller-owned lock");if(SUCCEEDED(hr)){display.locked=1;c.relock=1;CHECK(bh_gdi_text_execute(&c,q.commands+10,&next)&&display.locked&&display.color_pixels&&display.color_pitch&&next==BH_DRAW_END,"unlock GetDC ReleaseDC relock actual surface");CHECK(!c.failures&&c.native_calls==6,"all native operations succeeded");IDirectDrawSurface_Unlock(view.color,NULL);display.locked=0;}
  at=bh_draw_command_next(command);CHECK(at==10,"command chain retained");bh_gdi_text_dispose(&c);CHECK(!c.font,"font ownership release");}
 bh_scene_view_dispose(&view);CHECK(bh_draw_queue_dispose(&q),"queue dispose");*count+=checks;printf("{\"scenario\":\"gdi-text\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;}
#ifdef BH_GDI_TEXT_STANDALONE
int main(void){unsigned long n=0;return bh_gdi_text_checkpoint(&n);}
#endif
