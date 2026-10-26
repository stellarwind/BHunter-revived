#include "../src/bh_network_debug.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<10)printf("FAIL network debug line%d\n",__LINE__);}} while(0)
static const char *text(const BHDrawCommand *c) {return (const char *)(c->words+1);}
int __cdecl bh_network_debug_checkpoint(unsigned long *count) {
 BHNetworkDebug c;BHNetworkGame game;BHDrawQueue q;float ping[8]={0,1.5f,2,3,4,5,6,7};int i;
 checks=0;failures=0;memset(&c,0,sizeof(c));memset(&game,0,sizeof(game));CHECK(bh_draw_queue_init(&q,32,NULL));
 c.queue=&q;c.game=&game;c.ping_ticks=ping;c.mode=2;c.frame=-1;c.defcon=4;c.good=-3;c.bad=7;c.ugly=11;
 game.player_count=8;game.players[1].active=1;strcpy(game.players[1].name,"Alice");game.players[7].active=1;strcpy(game.players[7].name,"Bob");
 CHECK(bh_network_debug_draw(&c) && c.attempted==6 && c.queued==6 && !c.dropped);
 CHECK(!strcmp(text(q.commands+9),"HOST -1 AT DEFCON 5") && q.commands[9].resource==0 && q.commands[9].words[0]==255);
 CHECK(!strcmp(text(q.commands+10),"Good packs : -3") && q.commands[10].resource==(20UL<<16) && q.commands[10].words[0]==0xff00);
 CHECK(!strcmp(text(q.commands+11),"Bad packs : 7") && q.commands[11].words[0]==0xa0ff);
 CHECK(!strcmp(text(q.commands+12),"Ugly packs : 11") && q.commands[12].words[0]==255);
 CHECK(!strcmp(text(q.commands+13),"PING-time Alice = 50.0 ms") && q.commands[13].resource==(90UL<<16));
 CHECK(!strcmp(text(q.commands+14),"PING-time Bob = 233.3 ms") && q.commands[14].resource==(110UL<<16));
 CHECK(q.commands[8].next==9 && q.commands[14].next==BH_DRAW_END);
 c.mode=1;c.frame=0;c.defcon=2147483647L;bh_draw_queue_begin(&q);CHECK(bh_network_debug_draw(&c) && c.queued==4);
 CHECK(!strcmp(text(q.commands+9),"SLAVE 0 AT DEFCON -2147483648") && q.commands[9].words[0]==0xffffff);
 q.count=q.capacity;CHECK(bh_network_debug_draw(&c) && c.attempted==4 && !c.queued && c.dropped==4);
 c.mode=2;memset(game.players[1].name,'A',32);i=(int)q.count;CHECK(!bh_network_debug_draw(&c) && q.count==(unsigned long)i);
 CHECK(bh_draw_queue_dispose(&q));if(count)*count=checks;
 printf("{\"scenario\":\"network-debug\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_NETWORK_DEBUG_STANDALONE
int main(void) {return bh_network_debug_checkpoint(NULL);}
#endif
