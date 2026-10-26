#include "../src/bh_remote_query.h"
#include <stdio.h>
#include <string.h>
static void pointer(BHEntity *e,unsigned long off,void *p){memcpy((char*)e+off,&p,4);}
static BHEntity *get(BHEntity *e,unsigned long off){BHEntity *p;memcpy(&p,(char*)e+off,4);return p;}
static unsigned short u16(const unsigned char *p){return (unsigned short)(p[0]|p[1]<<8);}
int bh_remote_query_checkpoint(unsigned long *count){
    BHRemoteQuery c;BHRemoteQueryPacket packet;BHNetworkTransport transport;
    BHEntityPool pool;BHEntity *e[3],*remote[8],*head;BHWorldGeometry world;
    BHPickupRoom room;BHEffectWorldIndex index;BHPacketReader reader;BHPacketStats stats;
    unsigned long checks=0,failures=0,actors=3,i,j,recipient=19;unsigned short stamp=100;
    short slot;float value;
#define CHECK(x) do{++checks;if(!(x)){++failures;printf("FAIL remote query %d\n",__LINE__);}}while(0)
    memset(&c,0,sizeof(c));memset(&packet,0,sizeof(packet));memset(&world,0,sizeof(world));memset(&room,0,sizeof(room));memset(remote,0,sizeof(remote));memset(&stats,0,sizeof(stats));
    world.maximum.x=world.maximum.y=world.maximum.z=8;
    CHECK(bh_entities_init(&pool,3,NULL));CHECK(bh_network_transport_init(&transport,NULL,4));
    for(i=0;i<3;i++){
        e[i]=bh_entity_acquire(&pool);CHECK(e[i]!=NULL);slot=(short)i;
        memcpy((char*)e[i]+0x114,&slot,2);memcpy((char*)e[i]+0x17c,&stamp,2);memset((char*)e[i]+0x180,0,24);
        pointer(e[i],0x198,&world);remote[i]=e[i];
    }
    for(i=0;i<3;i++){
        pointer(e[i],0xec,i?e[i-1]:NULL);pointer(e[i],0xf0,i<2?e[i+1]:NULL);
        pointer(e[i],0xf8,i?e[i-1]:NULL);pointer(e[i],0xf4,i<2?e[i+1]:NULL);
    }
    room.world=&world;room.actors=e[0];head=e[0];index.world=&world;index.index=42;
    c.pool=&pool;c.rooms=&room;c.room_count=1;c.head=&head;c.count=&actors;c.remote=remote;
    c.local=e[2];c.local_id=123;c.worlds=&index;c.world_count=1;c.transport=&transport;c.tick=stamp;
    memcpy((char*)e[0]+0x138,&recipient,4);
    for(j=0;j<2;j++)for(i=0;i<3;i++){value=(float)(i+1)*.125f;memcpy((char*)e[0]+0x48+j*0x18+i*4,&value,4);}
    packet.words[0]=3;CHECK(bh_remote_query(&c,e[0],&packet)&&packet.floats[1]==0&&packet.floats[2]==0&&packet.floats[3]==0);
    packet.words[0]=16;CHECK(bh_remote_query(&c,e[0],&packet));
    for(i=0;i<3;i++){CHECK(packet.floats[4+i]==(float)(i+1)*3.75f);CHECK(packet.floats[7+i]==(float)(i+1)*112.5f);}
    for(i=0;i<4;i++){
        packet.damage.event=0;packet.damage.severity=i==0?.5f:i==1?9.0f:i==2?-.5f:1;
        packet.damage.attacker=i==0?e[2]:i==1?e[1]:i==2?e[0]:(BHEntity*)&packet;
        CHECK(bh_remote_query(&c,e[0],&packet)&&c.send_result==S_FALSE&&!transport.native_calls);
        CHECK(c.packet[0]==7&&u16(c.packet+2)==100&&u16(c.packet+8)==42);
        CHECK(bh_packet_receive(&reader,c.packet,c.wire_size,sizeof(c.packet),&stats));
        if(i==3)CHECK(c.wire_size==17&&c.packet[16]==128);
        else{CHECK(c.wire_size==19&&u16(c.packet+16)==(i==0?123:i==1?1:0));CHECK(c.packet[18]==(i==0?64:i==1?254:192));}
    }
    for(i=0;i<9;i++)pointer(e[0],0x1a4+i*12,i&1?e[2]:e[1]);
    packet.removed.event=17;packet.removed.actor=e[1];CHECK(bh_remote_query(&c,e[0],&packet));
    for(i=0;i<9;i++)CHECK(get(e[0],0x1a4+i*12)==(i&1?e[2]:NULL));
    packet.words[0]=99;CHECK(bh_remote_query(&c,e[0],&packet));
    packet.words[0]=20;pointer(e[1],0xec,NULL);
    CHECK(!bh_remote_query(&c,e[1],&packet)&&actors==3&&pool.used==3);
    pointer(e[1],0xec,e[0]);CHECK(bh_remote_query(&c,e[1],&packet));
    CHECK(actors==2&&pool.used==2&&!remote[1]&&head==e[0]&&room.actors==e[0]);
    CHECK(get(e[0],0xf0)==e[2]&&get(e[2],0xec)==e[0]&&get(e[0],0xf4)==e[2]&&get(e[2],0xf8)==e[0]);
    CHECK(bh_remote_query(&c,e[0],&packet)&&head==e[2]&&room.actors==e[2]&&actors==1);
    CHECK(bh_remote_query(&c,e[2],&packet)&&!head&&!room.actors&&!actors&&!pool.used);
    CHECK(bh_entities_dispose(&pool));CHECK(bh_network_transport_dispose(&transport));
    if(count)*count=checks;printf("{\"scenario\":\"remote-query\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;
#undef CHECK
}
