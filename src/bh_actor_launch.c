#include "bh_actor_launch.h"
#include "bh_round.h"
#include <math.h>
#include <stdlib.h>
static float rf(double v){float f;bh_round_float(&f,v);return f;}
BHEntity *__cdecl bh_actor_launch(BHActorLaunchContext *c,BHWorldGeometry *w,unsigned long type,long token,float speed,long tag){
 unsigned long r,wire,i;unsigned char payload[9];BHVec3 center;long quantized;int have_index=0;unsigned short index=0;
 if(!c||!c->actors){if(c)c->error=1;return 0;}c->error=0;r=(unsigned long)rand();r+=(unsigned long)rand()*65536UL;
 if(type==0||type==3){if(c->mode==3)return 0;if(c->mode==2){if(!w)return 0;if(!c->transport||!c->navigation||!_finite(speed)||fabs(speed)>=2147483648.0){c->error=1;return 0;}
  token=(token/5000)*5000;quantized=(long)(__int64)speed;speed=(float)quantized;center.x=rf(((double)w->maximum.x+w->minimum.x)*.5);center.y=rf(((double)w->maximum.y+w->minimum.y)*.5);center.z=rf(((double)w->maximum.z+w->minimum.z)*.5);
  for(i=0;i<c->navigation->node_count;++i)if(c->navigation->nodes[i].placement==w){index=(unsigned short)i;have_index=1;break;}if(!have_index){c->error=1;return 0;}
  payload[0]=(unsigned char)r;payload[1]=(unsigned char)(r>>8);payload[2]=(unsigned char)(r>>16);payload[3]=(unsigned char)(r>>24);payload[4]=(unsigned char)tag;payload[5]=(unsigned char)((unsigned long)tag>>8);payload[6]=(unsigned char)type;payload[7]=(unsigned char)(token/5000);payload[8]=(unsigned char)quantized;
  if(!bh_packet_begin(&c->writer,c->packet,sizeof(c->packet),10,c->tick)||!bh_packet_write_position(&c->writer,w,index,&center)||!bh_packet_append(&c->writer,payload,9)||!bh_packet_finish(&c->writer,&wire)){c->error=2;return 0;}
  c->send_result=bh_network_send(c->transport,0,c->packet,wire,0);++c->sent;
 }}
 return bh_actor_construct_room(c->actors,w,type,token,speed,(unsigned long)tag,r);
}
int __cdecl bh_actor_launch_receive(BHActorLaunchContext *c,void *bytes,unsigned long size,unsigned long capacity){
 BHWorldGeometry *world=0;BHVec3 position;unsigned char *p;unsigned long seed;short tag;unsigned short saved,tick;BHEntity *e;
 if(!c||!c->actors||!c->navigation||!c->stats||!c->good){if(c)c->error=1;return 0;}c->error=0;
 if(!bh_packet_receive(&c->reader,bytes,size,capacity,c->stats)||!bh_packet_read_position(&c->reader,c->navigation,&world,&position)){c->error=2;return 0;}
 if(c->reader.cursor>c->reader.size||c->reader.size-c->reader.cursor<9){c->reader.error=BH_PACKET_TRUNCATED;c->error=2;return 0;}
 p=c->reader.data+c->reader.cursor;c->reader.cursor+=9;seed=(unsigned long)p[0]|((unsigned long)p[1]<<8)|((unsigned long)p[2]<<16)|((unsigned long)p[3]<<24);tag=(short)((unsigned short)p[4]|((unsigned short)p[5]<<8));
 --c->stats->accepted;++*c->good;tick=(unsigned short)c->reader.data[2]|((unsigned short)c->reader.data[3]<<8);saved=c->actors->response->damage->route->tick;c->actors->response->damage->route->tick=tick;
 e=bh_actor_construct_position(c->actors,&position,(unsigned long)(long)(signed char)p[6],(long)(signed char)p[7]*5000,(float)(signed char)p[8],(unsigned long)(long)tag,seed);
 c->actors->response->damage->route->tick=saved;++c->received;if(!e){c->error=3;return 0;}return 1;
}
