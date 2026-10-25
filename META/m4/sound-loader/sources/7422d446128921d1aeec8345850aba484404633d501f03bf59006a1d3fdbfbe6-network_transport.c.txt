#include "../src/bh_network_transport.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL transport %d: %s\n",__LINE__,label);}} while(0)
static void cases(void) {
    LPDIRECTPLAY3A peer=NULL;HRESULT hr,direct,forwarded;
    BHNetworkTransport transport;BHPacketWriter writer;BHPacketReader reader;
    BHPacketStats stats={0,0};unsigned char bytes[264],before[264],payload[5]={1,2,3,4,5};
    unsigned long calls;int flag;
    CHECK(bh_network_transport_init(&transport,NULL,0x12345678UL),"fresh explicit no-peer owner");
    CHECK(bh_network_send(&transport,17,NULL,0,0)==S_FALSE && !transport.native_calls,"original NULL interface path makes no Send call");
    CHECK(bh_packet_begin(&writer,bytes,sizeof(bytes),6,0x1234) && bh_packet_append(&writer,payload,5),"real source packet writer");
    CHECK(bh_network_packet_send(&transport,&writer,17,0)==S_FALSE && writer.finished && writer.cursor==16 && bytes[1]==5 && !transport.native_calls,"sender finalizes even without native peer");
    CHECK(bh_packet_receive(&reader,bytes,13,sizeof(bytes),&stats)==1 && reader.size==13 && !bytes[13] && !bytes[14] && !bytes[15],"sender checksum accepted by actual source receiver; zero pad not wire bytes");
    CHECK(bh_network_transport_dispose(&transport),"no-peer ownership disposal");
    hr=CoInitialize(NULL);CHECK(SUCCEEDED(hr),"native COM initialized for unopened interface");
    if(FAILED(hr))return;
    hr=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void **)&peer);
    printf("{\"native_directplay_create_hresult\":%ld,\"sessions_opened\":0,\"connections_initialized\":0}\n",(long)hr);
    CHECK(hr==DP_OK && peer,"real native DirectPlay3A COM creation without provider/session");
    if(hr!=DP_OK || !peer) {CoUninitialize();return;}
    CHECK(bh_network_transport_init(&transport,peer,0x12345678UL),"source owner AddRefs real interface");
    for(flag=0;flag<3;++flag) {
        direct=IDirectPlay3_Send(peer,0x12345678UL,17,flag ? DPSEND_GUARANTEED:0,payload,5);
        forwarded=bh_network_send(&transport,17,payload,5,flag==2 ? -7:flag);
        CHECK(FAILED(direct) && forwarded==direct,"forwarded native failure equals direct unopened-interface control");
        CHECK(transport.native_calls==(unsigned long)flag+1 && transport.last_recipient==17 && transport.last_size==5 && transport.last_flags==(flag ? 1UL:0UL),"full source/from/recipient/size and zero/nonzero guaranteed mapping");
    }
    CHECK(bh_packet_begin(&writer,bytes,sizeof(bytes),6,0x1234) && bh_packet_append(&writer,payload,5),"new native-bound packet");
    calls=transport.native_calls;
    direct=IDirectPlay3_Send(peer,0x12345678UL,0,1,bytes,13);
    forwarded=bh_network_packet_send(&transport,&writer,0,1);
    CHECK(FAILED(direct) && forwarded==direct && transport.native_calls==calls+1 && transport.last_size==13 && transport.last_recipient==0 && transport.last_flags==1,"complete sender reaches real Send with broadcast ID and unpadded byte count");
    CHECK(bh_packet_receive(&reader,bytes,13,sizeof(bytes),&stats)==1 && writer.cursor==16,"native failure preserves completed valid source packet");
    memcpy(before,bytes,sizeof(bytes));calls=transport.native_calls;
    CHECK(bh_network_packet_send(&transport,&writer,0,1)==E_INVALIDARG && transport.native_calls==calls && !memcmp(before,bytes,sizeof(bytes)),"bounded repeated-send guard makes no new native call");
    CHECK(bh_packet_begin(&writer,bytes,13,6,0) && bh_packet_append(&writer,payload,5),"insufficient padding fixture");
    CHECK(bh_network_packet_send(&transport,&writer,17,0)==E_INVALIDARG && transport.native_calls==calls && !writer.finished,"padding failure cannot send partial packet");
    CHECK(bh_network_send(NULL,0,NULL,0,0)==E_INVALIDARG && bh_network_packet_send(NULL,&writer,0,0)==E_INVALIDARG,"invalid source contexts reject");
    CHECK(bh_network_transport_dispose(&transport),"source AddRef ownership released");
    CHECK(IDirectPlay3_Release(peer)==0,"final actual native interface released");
    CoUninitialize();
    printf("{\"native_directplay_unopened_send_hresult\":%ld,\"successful_sends\":0}\n",(long)forwarded);
}
int bh_network_transport_checkpoint(unsigned long *count) {
    checks=0;failed=0;cases();*count=checks;
    printf("{\"scenario\":\"network-transport\",\"checks\":%lu,\"failures\":%d,\"sessions_opened\":0,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
