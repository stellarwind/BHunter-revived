#include "../src/bh_network_effect_receive.h"
#include "../src/bh_effect_projectile.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x) do {++checks;if(!(x)){++failures;if(failures<10)printf("FAIL network effect receive line%d\n",__LINE__);}} while(0)
static void word(BHEntity *e,unsigned long at,unsigned long n) {memcpy((unsigned char *)e+at,&n,4);}
static unsigned long encode(unsigned char bytes[32],BHWorldGeometry *world,unsigned short index,unsigned char actor,unsigned char kind) {
 BHPacketWriter writer;BHVec3 point={5,-10,15};unsigned long size;unsigned char tail[5];
 tail[0]=actor;tail[1]=kind;tail[2]=128;tail[3]=64;tail[4]=255;
 CHECK(bh_packet_begin(&writer,bytes,32,9,0x20));CHECK(bh_packet_write_position(&writer,world,index,&point));CHECK(bh_packet_append(&writer,tail,5));CHECK(bh_packet_finish(&writer,&size));return size;
}
int __cdecl bh_network_effect_receive_checkpoint(unsigned long *count) {
 BHMathEnvironment env;BHNetworkEffectReceive c;BHPacketStats stats={0,0};unsigned long good=0,n;
 BHNavigation nav;BHNavigationNode node;BHWorldGeometry world;BHEntityPool pool;BHEntity *effect;
 BHSoundQueue sound;BHDrawQueue queue;BHScriptParticleContext particles;BHFlashFlareContext flash;
 BHScriptDebrisContext debris;BHHomingProjectileContext effects;BHEffectProjectileContext core;unsigned char bytes[32];
 checks=0;failures=0;CHECK(bh_math_environment_enter(&env));memset(&c,0,sizeof(c));memset(&nav,0,sizeof(nav));memset(&node,0,sizeof(node));memset(&world,0,sizeof(world));
 memset(&flash,0,sizeof(flash));memset(&debris,0,sizeof(debris));memset(&effects,0,sizeof(effects));memset(&core,0,sizeof(core));
 world.minimum.x=-10;world.minimum.y=-20;world.minimum.z=-30;world.maximum.x=10;world.maximum.y=20;world.maximum.z=30;
 node.placement=&world;nav.nodes=&node;nav.node_count=1;
 CHECK(bh_entities_init(&pool,4,NULL));CHECK(bh_sound_queue_init(&sound,NULL,NULL,NULL));CHECK(bh_draw_queue_init(&queue,32,NULL));CHECK(bh_script_particles_init(&particles,&pool,&queue));
 flash.pool=&pool;flash.queue=&queue;debris.pool=&pool;debris.queue=&queue;effects.pool=&pool;effects.sound=&sound;effects.particles=&particles;effects.flash=&flash;
 core.effects=&effects;core.debris=&debris;effect=bh_entity_acquire(&pool);CHECK(effect!=NULL);word(effect,0x128,BH_SOUND_INVALID);word(effect,0x130,0x40030020UL);
 CHECK(bh_effect_projectile_bind(&core,effect,0));c.navigation=&nav;c.pool=&pool;c.stats=&stats;c.good=&good;
 n=encode(bytes,&world,0,3,1);CHECK(n==21);CHECK(bh_network_effect_receive(&c,bytes,n,32)==1);
 CHECK(c.dispatched==1 && good==1 && stats.accepted==0 && !stats.rejected && c.reader.cursor==21 && c.world==&world);
 CHECK(c.packet.words[0]==18 && c.packet.words[1]==-1 && c.packet.words[2]==1);
 CHECK(effect->position.x==5 && effect->position.y==-10 && effect->position.z==15);
 CHECK(c.packet.floats[7]==-1.5f && c.packet.floats[8]==.75f && c.packet.floats[9]==-.01171875f);
 n=encode(bytes,&world,0,2,1);CHECK(bh_network_effect_receive(&c,bytes,n,32)==1 && good==1 && stats.accepted==1 && c.packet.words[1]==0x40020020L);
 n=encode(bytes,&world,0,9,1);CHECK(bh_network_effect_receive(&c,bytes,n,32)==1 && c.reader.cursor==17 && c.dispatched==2 && stats.accepted==2);
 n=encode(bytes,&world,1,3,1);CHECK(bh_network_effect_receive(&c,bytes,n,32)==0 && c.reader.cursor==10 && stats.accepted==2 && stats.rejected==1);
 n=encode(bytes,&world,0,3,1);bytes[4]^=1;CHECK(bh_network_effect_receive(&c,bytes,n,32)==0 && stats.accepted==2 && stats.rejected==2);
 n=encode(bytes,&world,0,3,1);CHECK(bh_network_effect_receive(&c,bytes,n-1,32)==-1 && stats.accepted==2 && stats.rejected==2);
 CHECK(bh_sound_queue_dispose(&sound));CHECK(bh_draw_queue_dispose(&queue));CHECK(bh_entities_dispose(&pool));bh_math_environment_leave(&env);if(count)*count=checks;
 printf("{\"scenario\":\"network-effect-receive\",\"checks\":%lu,\"failures\":%d,\"sessions_opened\":0,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_NETWORK_EFFECT_RECEIVE_STANDALONE
int main(void) {return bh_network_effect_receive_checkpoint(NULL);}
#endif
