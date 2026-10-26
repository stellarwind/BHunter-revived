#include "bh_network_effect_receive.h"
#include "bh_round.h"
#include <string.h>
int __cdecl bh_network_effect_receive(BHNetworkEffectReceive *c,void *bytes,unsigned long size,unsigned long capacity) {
 BHVec3 position;unsigned char *payload;unsigned long id;int result;unsigned long i;
 if(!c || !c->navigation || !c->pool || !c->stats || !c->good){if(c)c->error=1;return -1;}
 c->error=0;
 result=bh_packet_receive(&c->reader,bytes,size,capacity,c->stats);if(result!=1)return result;
 result=bh_packet_read_position(&c->reader,c->navigation,&c->world,&position);if(result!=1)return result;
 if(c->reader.cursor>=c->reader.size){c->error=2;return -1;}
 payload=c->reader.data+c->reader.cursor++;if(*payload>=8)return 1;
 if(c->reader.size-c->reader.cursor<4){c->error=2;return -1;}
 memset(&c->packet,0,sizeof(c->packet));
 id=c->reader.data[2]|((unsigned long)c->reader.data[3]<<8)|((unsigned long)*payload<<16)|0x40000000UL;
 c->packet.words[0]=18;c->packet.words[1]=(long)id;c->packet.words[2]=payload[1];
 memcpy(c->packet.floats+4,&position,12);
 for(i=0;i<3;++i)bh_round_float(c->packet.floats+7+i,(double)(signed char)payload[2+i]*.01171875f);
 c->reader.cursor+=4;++c->dispatched;
 if(!bh_entities_query(c->pool,0,0,NULL,0,&c->packet)){c->error=3;return -1;}
 if(c->packet.words[1]==-1){--c->stats->accepted;++*c->good;}
 return 1;
}
