#include "D:/Decomp/Bhunter/src/bh_gdi_text.h"
#include "D:/Decomp/Bhunter/src/bh_scene_view.h"
#include <stdio.h>
#include <string.h>
int main(void){BHSceneView view;BHDrawQueue queue;BHFrameDisplay display;BHGdiText text;HFONT old;LOGFONTA font;unsigned long next;int before,after;
 memset(&view,0,sizeof(view));memset(&queue,0,sizeof(queue));memset(&display,0,sizeof(display));memset(&text,0,sizeof(text));
 if(!bh_scene_view_init(&view,160,80)||!bh_draw_queue_init(&queue,16,NULL))return 2;
 display.color=view.color;text.display=&display;bh_gdi_text_queue(&queue,2,2,"Lifetime",0xffffff);
 if(!bh_gdi_text_execute(&text,queue.commands+9,&next)||!text.font)return 3;
 old=text.font;before=GetObjectA(old,sizeof(font),&font);bh_gdi_text_dispose(&text);after=GetObjectA(old,sizeof(font),&font);
 printf("{\"before_getobject\":%d,\"after_getobject\":%d,\"font_pointer_cleared\":%d,\"native_failures\":%lu,\"game_executed\":false}\n",before,after,text.font==NULL,text.failures);
 bh_draw_queue_dispose(&queue);bh_scene_view_dispose(&view);return before==sizeof(font)&&after==0?0:1;}
