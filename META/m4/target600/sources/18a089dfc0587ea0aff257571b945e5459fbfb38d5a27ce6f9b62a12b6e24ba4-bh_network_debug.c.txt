#include "bh_network_debug.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
static void queue(BHNetworkDebug *c,short y,const char *text,unsigned long color) {
 int result=bh_gdi_text_queue(c->queue,0,y,text,color);++c->attempted;
 if(result>0)++c->queued;else if(!result)++c->dropped;else c->error=2;
}
int __cdecl bh_network_debug_draw(BHNetworkDebug *c) {
 char text[132];long i,next;unsigned long bits;short y=90;unsigned long color;
 if(!c || !c->queue || !c->game || c->game->player_count<0 || c->game->player_count>8 ||
  (c->mode==2 && c->game->player_count>1 && !c->ping_ticks)){if(c)c->error=1;return 0;}
 if(c->mode==2)for(i=1;i<c->game->player_count;++i)if(c->game->players[i].active) {
  if(!memchr(c->game->players[i].name,0,32) || !_finite(c->ping_ticks[i])){c->error=1;return 0;}
  sprintf(text,"PING-time %s = %4.1f ms",c->game->players[i].name,(double)c->ping_ticks[i]*33.333);
  if(strlen(text)>58){c->error=1;return 0;}
 }
 c->attempted=c->queued=c->dropped=0;c->error=0;
 color=(c->frame&0x20)?0xff0000:0xffffff;
 bits=(unsigned long)c->defcon+1;memcpy(&next,&bits,4);
 sprintf(text,c->mode==2?"HOST %d AT DEFCON %d":"SLAVE %d AT DEFCON %d",(int)c->frame,(int)next);queue(c,0,text,color);
 sprintf(text,"Good packs : %d",(int)c->good);queue(c,20,text,0xff00);
 sprintf(text,"Bad packs : %d",(int)c->bad);queue(c,40,text,0xffa000);
 sprintf(text,"Ugly packs : %d",(int)c->ugly);queue(c,60,text,0xff0000);
 if(c->mode==2)for(i=1;i<c->game->player_count;++i)if(c->game->players[i].active) {
  sprintf(text,"PING-time %s = %4.1f ms",c->game->players[i].name,(double)c->ping_ticks[i]*33.333);
  queue(c,y,text,0xffffff);y=(short)(y+20);
 }
 return c->error==0;
}
