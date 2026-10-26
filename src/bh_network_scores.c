#include "bh_network_scores.h"
#include <string.h>
static int valid(BHNetworkScores *c) {
 if(!c || !c->scores || !c->scores->remote || !c->scores->local_name ||
   !c->text || !c->text->queue || !c->transport) {if(c)c->error=1;return 0;}
 c->error=0;return 1;
}
static void word(unsigned char *p,unsigned short value) {p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);}
static int snapshot(BHNetworkScores *c) {
 unsigned char payload[16];unsigned long i;unsigned short value;
 for(i=0;i<8;++i) {
  value=0;
  if((long)i==c->scores->local_index)value=(unsigned short)c->scores->local_score;
  else if(c->scores->remote[i])memcpy(&value,(const unsigned char *)c->scores->remote[i]+0x11c,2);
  word(payload+i*2,value);
 }
 if(!bh_packet_begin(&c->writer,c->packet,sizeof(c->packet),12,c->frame) ||
   !bh_packet_append(&c->writer,payload,16)){c->error=2;return 0;}
 c->send_result=bh_network_packet_send(c->transport,&c->writer,0,0);++c->broadcasts;
 if(!bh_hud_scores_text(c->text,c->scores)){c->error=3;return 0;}
 return 1;
}
int __cdecl bh_network_scores_broadcast(BHNetworkScores *c) {
 if(!valid(c))return 0;return snapshot(c);
}
int __cdecl bh_network_scores_death(BHNetworkScores *c,long attacker,BHEntity *victim) {
 long index=-1;unsigned long i,bits;unsigned short score;unsigned char payload[2];
 if(!valid(c))return 0;
 for(i=0;i<8;++i)if(c->scores->remote[i]==victim)index=(long)i;
 if(victim==c->local_player)index=attacker;
 if(c->mode==2) {
  if(attacker>=0 && attacker<8) {
   bits=(unsigned long)c->scores->local_score-1;memcpy(&c->scores->local_score,&bits,4);
  }
  if(index>=0 && index<8) {
   if(index==attacker) {
    bits=(unsigned long)c->scores->local_score+1;memcpy(&c->scores->local_score,&bits,4);
   }else if(c->scores->remote[index]) {
    memcpy(&score,(unsigned char *)c->scores->remote[index]+0x11c,2);++score;
    memcpy((unsigned char *)c->scores->remote[index]+0x11c,&score,2);
   }
  }
  return snapshot(c);
 }
 payload[0]=(unsigned char)attacker;payload[1]=(unsigned char)index;
 if(!bh_packet_begin(&c->writer,c->packet,sizeof(c->packet),11,c->frame) ||
   !bh_packet_append(&c->writer,payload,2)){c->error=2;return 0;}
 c->send_result=bh_network_packet_send(c->transport,&c->writer,1,0);++c->reports;return 1;
}
