#include "../src/bh_network_scores.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<10)printf("FAIL network scores line%d\n",__LINE__);}} while(0)
static unsigned short word(const unsigned char *p) {return (unsigned short)(p[0]|((unsigned short)p[1]<<8));}
static void score(BHEntity *e,short value) {memcpy((unsigned char *)e+0x11c,&value,2);strcpy((char *)e+0x11e,"Remote");}
int __cdecl bh_network_scores_checkpoint(unsigned long *count) {
 BHNetworkScores c;BHNetworkTransport transport;BHHudScores scores;BHHudScoreText text;
 BHTextQueue queue;BHAtlasPage page;BHAtlasImage atlas;BHEntity remotes[8],local,unknown,*slots[8];
 BHPacketReader reader;BHPacketStats stats={0,0};LPDIRECTPLAY3A peer=NULL;HRESULT hr,direct;int i,j,attacker;long saved;unsigned long calls;
 checks=0;failures=0;memset(&c,0,sizeof(c));memset(&scores,0,sizeof(scores));memset(&text,0,sizeof(text));
 memset(&page,0,sizeof(page));memset(&atlas,0,sizeof(atlas));memset(remotes,0,sizeof(remotes));
 page.atlas=&atlas;atlas.rectangle_count=224;for(i=0;i<224;++i)atlas.rectangles[i].width=4;
 for(i=0;i<8;++i){slots[i]=remotes+i;score(remotes+i,(short)(100+i));}
 scores.remote=slots;scores.local_index=3;scores.local_score=0x12345678;scores.local_name="Local";
 text.queue=&queue;text.page=&page;bh_text_queue_init(&queue);CHECK(bh_network_transport_init(&transport,NULL,99));
 c.scores=&scores;c.text=&text;c.transport=&transport;c.local_player=&local;c.frame=0x9876;
 CHECK(bh_network_scores_broadcast(&c));CHECK(c.broadcasts==1 && c.send_result==S_FALSE && c.writer.finished && c.writer.cursor==24);
 CHECK(c.packet[0]==12 && c.packet[1]==16 && word(c.packet+2)==0x9876 && word(c.packet+14)==0x5678);
 for(i=0;i<8;++i)if(i!=3)CHECK(word(c.packet+8+i*2)==100+i);
 CHECK(bh_packet_receive(&reader,c.packet,24,sizeof(c.packet),&stats));
 CHECK(bh_text_queue_count(&queue)==19 && !strcmp((char *)queue.commands[18].text,"305419896"));
 /* Every attacker/victim combination, with an independent scoreboard rule. */
 for(attacker=-1;attacker<=8;++attacker)for(j=-1;j<8;++j) {
  scores.local_score=10;for(i=0;i<8;++i)score(remotes+i,0);c.mode=2;bh_text_queue_init(&queue);
  CHECK(bh_network_scores_death(&c,attacker,j<0?&unknown:remotes+j));
  saved=10-(attacker>=0 && attacker<8)+(j>=0 && j<8 && j==attacker);
  CHECK(scores.local_score==saved);
  for(i=0;i<8;++i)CHECK((short)word((unsigned char *)(remotes+i)+0x11c)==((j==i && j!=attacker)?1:0));
  CHECK(c.packet[0]==12 && c.packet[1]==16 && bh_packet_receive(&reader,c.packet,24,sizeof(c.packet),&stats));
 }
 scores.local_score=(-2147483647L-1);c.mode=2;score(remotes,32767);bh_text_queue_init(&queue);
 CHECK(bh_network_scores_death(&c,1,remotes) && scores.local_score==2147483647L && (short)word((unsigned char *)remotes+0x11c)==-32768);
 saved=scores.local_score;CHECK(bh_network_scores_death(&c,1,&local) && scores.local_score==saved);
 c.mode=1;bh_text_queue_init(&queue);scores.local_score=123;
 CHECK(bh_network_scores_death(&c,17,&unknown));CHECK(c.packet[0]==11 && c.packet[1]==2 && c.packet[8]==17 && c.packet[9]==255 && !c.packet[10] && !c.packet[11]);
 CHECK(scores.local_score==123 && !bh_text_queue_count(&queue));
 slots[0]=slots[7]=remotes+1;CHECK(bh_network_scores_death(&c,1,remotes+1) && c.packet[9]==7);
 CHECK(bh_network_scores_death(&c,-1,&local) && c.packet[8]==255 && c.packet[9]==255);
 slots[6]=slots[7]=NULL;CHECK(bh_network_scores_death(&c,2,NULL) && c.packet[9]==7);
 hr=CoInitialize(NULL);CHECK(SUCCEEDED(hr));
 if(SUCCEEDED(hr)) {
  hr=CoCreateInstance(&CLSID_DirectPlay,NULL,CLSCTX_INPROC_SERVER,&IID_IDirectPlay3A,(void **)&peer);CHECK(hr==DP_OK && peer);
  if(peer) {
   CHECK(bh_network_transport_dispose(&transport));CHECK(bh_network_transport_init(&transport,peer,99));
   calls=transport.native_calls;direct=IDirectPlay3_Send(peer,99,1,0,c.packet,10);
   CHECK(bh_network_scores_death(&c,2,&unknown) && transport.native_calls==calls+1 && c.send_result==direct && FAILED(direct));
   CHECK(transport.last_recipient==1 && transport.last_flags==0 && transport.last_size==10);
   bh_text_queue_init(&queue);calls=transport.native_calls;CHECK(bh_network_scores_broadcast(&c) && transport.native_calls==calls+1 && transport.last_recipient==0 && transport.last_size==24 && text.attempted>0);
   CHECK(bh_network_transport_dispose(&transport));CHECK(IDirectPlay3_Release(peer)==0);
  }
  CoUninitialize();
 }
 printf("{\"scenario\":\"network-scores\",\"checks\":%lu,\"failures\":%d,\"sessions_opened\":0,\"original_code_required\":false}\n",checks,failures);
 if(count)*count=checks;return failures?1:0;
}
#ifdef BH_NETWORK_SCORES_STANDALONE
int main(void) {return bh_network_scores_checkpoint(NULL);}
#endif
