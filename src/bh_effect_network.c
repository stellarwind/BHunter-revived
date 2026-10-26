#include "bh_effect_network.h"
#include <string.h>
int __cdecl bh_effect_network_emit(BHEffectNetworkContext *c,const BHEntity *e,unsigned long world_offset,unsigned long sync_offset,unsigned char kind,const BHVec3 *direction){unsigned long sync,i;BHWorldGeometry *world;BHPacketWriter writer;unsigned char payload[2];if(!c||!e||!direction||!c->transport)return 0;c->error=0;memcpy(&sync,(const unsigned char*)e+sync_offset,4);if(c->peers<=1||!sync)return 1;memcpy(&world,(const unsigned char*)e+world_offset,4);if(!world)return 1;
 for(i=0;i<c->world_count;i++)if(c->world_indices[i].world==world)break;if(i==c->world_count){c->error=1;return 0;}
 if(!bh_packet_begin(&writer,c->packet,sizeof(c->packet),9,(unsigned short)sync)||!bh_packet_write_position(&writer,world,c->world_indices[i].index,&e->position)){c->error=1;return 0;}payload[0]=(unsigned char)(sync>>16);payload[1]=kind;if(!bh_packet_append(&writer,payload,2)||!bh_packet_write_vector(&writer,direction,1)||!bh_packet_finish(&writer,&c->bytes)){c->error=1;return 0;}bh_network_send(c->transport,0,c->packet,c->bytes,0);++c->packets;return 1;
}
