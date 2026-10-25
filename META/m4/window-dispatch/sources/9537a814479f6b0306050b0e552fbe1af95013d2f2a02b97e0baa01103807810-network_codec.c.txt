#include "../src/bh_network_codec.h"
#include "../src/bh_physics_response.h"
#include "saved_contact.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failed;
#define CHECK(x,label) do {++checks;if(!(x)){if(++failed<8)printf("FAIL packet %d: %s\n",__LINE__,label);}} while(0)
static unsigned short word(const unsigned char *p) {return (unsigned short)(p[0]|((unsigned short)p[1]<<8));}
static void envelope_cases(void) {
    unsigned char data[264],saved[264],payload[256];unsigned long size,n,i;
    BHPacketWriter w,saved_writer;BHPacketReader r,saved_reader;BHPacketStats stats={0,0},saved_stats;
    for(i=0;i<256;++i) payload[i]=(unsigned char)(i*13+7);
    for(n=0;n<=255;++n) {
        memset(data,0xcd,sizeof(data));memset(&stats,0,sizeof(stats));
        CHECK(bh_packet_begin(&w,data,sizeof(data),6,0x1234) && bh_packet_append(&w,payload,n) && bh_packet_finish(&w,&size),"all payload lengths finalize");
        CHECK(size==n+8 && data[1]==n && w.cursor==((size+3)&~3UL),"wire size excludes padding and writer cursor includes it");
        for(i=size;i<w.cursor;++i) data[i]=0xa5;
        CHECK(bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && stats.accepted==1 && !stats.rejected && r.cursor==8 && r.size==size,"original receive restores padding before checksum");
        for(i=size;i<w.cursor;++i) CHECK(data[i]==0,"padding bytes cleared");
        if(n) {
            data[8]^=1;
            CHECK(bh_packet_receive(&r,data,size,sizeof(data),&stats)==0 && stats.accepted==1 && stats.rejected==1 && r.cursor==8 && r.error==BH_PACKET_CHECKSUM,"checksum rejection rolls back receive count");
        }
    }
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && !bh_packet_append(&w,payload,256) && w.cursor==8,"payload byte length limit is bounded");
    CHECK(bh_packet_append(&w,payload,255) && bh_packet_finish(&w,&size),"maximum payload plus aligned padding");
    memcpy(saved,data,sizeof(data));CHECK(!bh_packet_append(&w,payload,1) && !memcmp(data,saved,sizeof(data)),"finished writer cannot append");
    CHECK(bh_packet_begin(&w,data,263,6,0) && bh_packet_append(&w,payload,255),"unpadded maximum fits smaller declared capacity");
    memcpy(saved,data,sizeof(data));saved_writer=w;size=99;
    CHECK(!bh_packet_finish(&w,&size) && size==99 && w.cursor==saved_writer.cursor && !memcmp(data,saved,sizeof(data)),"final padding capacity failure is atomic");
    w.capacity=sizeof(data);CHECK(bh_packet_finish(&w,&size),"same owned buffer finishes after capacity corrected");
    memset(&r,0x5a,sizeof(r));saved_reader=r;saved_stats=stats;memcpy(saved,data,sizeof(data));
    CHECK(bh_packet_receive(&r,data,size-1,sizeof(data),&stats)==-1 && !memcmp(&r,&saved_reader,sizeof(r)) && !memcmp(&stats,&saved_stats,sizeof(stats)) && !memcmp(data,saved,sizeof(data)),"short declared envelope rejects before mutation");
    CHECK(bh_packet_receive(&r,data,size,263,&stats)==-1,"receive requires writable padding capacity");
}
static void position_cases(void) {
    unsigned char data[264],saved[264];unsigned long size,i;
    static const unsigned char golden[16]={6,8,0x34,0x12,7,0x8a,0x34,0x92,1,2,0,0x40,0,0x80,0,0xc0};
    BHPacketWriter w;BHPacketReader r;BHPacketStats stats={0,0};
    BHWorldGeometry worlds[2],*selected,*prior;BHNavigation navigation;BHNavigationNode nodes[2];
    BHVec3 point={-10,0,30},decoded={7,8,9},saved_point;unsigned short invalid=2;
    memset(worlds,0,sizeof(worlds));memset(&navigation,0,sizeof(navigation));memset(nodes,0,sizeof(nodes));
    nodes[0].placement=worlds;nodes[1].placement=worlds+1;navigation.nodes=nodes;navigation.node_count=2;
    worlds[0].minimum.x=-10;worlds[0].minimum.y=-20;worlds[0].minimum.z=-30;
    worlds[0].maximum.x=10;worlds[0].maximum.y=20;worlds[0].maximum.z=30;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0x1234) && bh_packet_write_position(&w,worlds,0x0201,&point)==1 && bh_packet_finish(&w,&size) && size==16 && !memcmp(data,golden,16),"independent golden LE header/index/position/checksum bytes");
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0),"new position packet");memcpy(saved,data,sizeof(data));
    CHECK(bh_packet_write_position(&w,NULL,0,NULL)==0 && w.cursor==8 && !memcmp(saved,data,sizeof(data)),"missing world returns without append");
    CHECK(bh_packet_write_position(&w,worlds,0,&point)==1 && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1,"real position encoder and checksum consumer");
    selected=NULL;CHECK(bh_packet_read_position(&r,&navigation,&selected,&decoded)==1 && selected==worlds && decoded.x==-10 && decoded.y==0 && decoded.z==30 && r.cursor==16,"world registry resolves exact bound endpoints/midpoint");
    worlds[1].maximum.x=worlds[1].maximum.y=worlds[1].maximum.z=1;
    point.x=-.50001f;point.y=1.5f;point.z=1.499969482421875f;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_write_position(&w,worlds+1,1,&point)==1 && word(data+10)==0 && word(data+12)==65208 && word(data+14)==65535,"lower clamp and discontinuous upper clamp retain original words");
    CHECK(bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && bh_packet_read_position(&r,&navigation,&selected,&decoded)==1 && decoded.x==-.5f && decoded.y==1.489990234375f && decoded.z==1.499969482421875f,"decoder preserves representable outside-room fractions");
    for(i=0;i<129;++i) {
        point.x=(float)i/128;point.y=1-point.x;point.z=.5f;
        CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_write_position(&w,worlds+1,1,&point)==1 && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && bh_packet_read_position(&r,&navigation,&selected,&decoded)==1 && !memcmp(&point,&decoded,sizeof(point)),"independent dyadic lattice roundtrips exactly");
    }
    point.x=point.y=point.z=.12345f;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_write_position(&w,worlds+1,1,&point)==1 && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && bh_packet_read_position(&r,&navigation,&selected,&decoded)==1 && decoded.x<=point.x && (double)point.x-decoded.x<1.0/32768,"non-dyadic quantization error has original truncation direction and bound");
    worlds[1].maximum=worlds[1].minimum;point.x=0;point.y=1;point.z=-1;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_write_position(&w,worlds+1,1,&point)==1 && word(data+10)==0 && word(data+12)==65208 && word(data+14)==0,"zero extents keep unordered lower and positive infinite upper branches");
    stats.accepted=stats.rejected=0;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_append(&w,&invalid,2) && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1,"bounded invalid-index envelope");
    prior=selected;saved_point=decoded;
    CHECK(bh_packet_read_position(&r,&navigation,&selected,&decoded)==0 && selected==prior && !memcmp(&decoded,&saved_point,sizeof(decoded)) && r.cursor==10 && stats.accepted==0 && stats.rejected==1,"invalid world index consumes only its word and adjusts statistics");
    invalid=0;CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_append(&w,&invalid,2) && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && bh_packet_read_position(&r,&navigation,&selected,&decoded)==-1 && r.cursor==8 && selected==prior && !memcmp(&decoded,&saved_point,sizeof(decoded)),"valid index with short coordinates rejects without touching outputs");
}
static void vector_actor_cases(void) {
    unsigned char data[264];unsigned long size,used;unsigned short id;
    BHPacketWriter w;BHPacketReader r;BHPacketStats stats={0,0};BHVec3 v;
    BHEntityPool pool;BHEntity *local,*other,*remote[8],*out;
    BHSweepHit hit;BHPhysicsResponse response;BHVec3 acceleration={0,0,0};
    v.x=-129;v.y=-1.9f;v.z=128.9f;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_write_vector(&w,&v,128) && data[8]==128 && data[9]==255 && data[10]==127,"signed vector truncates toward zero before saturation");
    CHECK(bh_packet_write_vector(&w,&saved_contact_normal,1) && data[11]==127 && !data[12] && !data[13],"saved M3 normal contract compresses to expected signed bytes");
    v.x=4294967296.0f;v.y=2147483648.0f;v.z=-2147483648.0f;
    CHECK(bh_packet_write_vector(&w,&v,128) && data[14]==0 && data[15]==128 && data[16]==128,"original CRT 64-bit conversion then signed low EAX clamp, not int32 saturation");
    v.x=1;v.y=0;v.z=-1;
    CHECK(bh_packet_write_vector(&w,&v,0) && !data[17] && !data[18] && !data[19],"real CRT masked infinite/NaN conversion low word retained");
    CHECK(bh_packet_begin(&w,data,10,6,0),"small vector target");used=w.cursor;
    CHECK(!bh_packet_write_vector(&w,&v,1) && w.cursor==used,"vector capacity failure leaves cursor unchanged");
    memset(&hit,0,sizeof(hit));hit.surface_present=1;hit.motion.x=-2;hit.surface.normal=saved_contact_normal;
    CHECK(bh_physics_contact_response(&hit,SAVED_CONTACT_FRACTION,.5f,2,-1,&acceleration,&response),"real physics response packet producer");
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_write_vector(&w,&response.first.normal,1) && data[8]==127 && !data[9] && !data[10],"actual source physics packet vector passes through restored encoder");
    CHECK(bh_entities_init(&pool,2,NULL),"real source entity pool");local=bh_entity_acquire(&pool);other=bh_entity_acquire(&pool);CHECK(local && other,"real source player identities");
    memset(remote,0,sizeof(remote));remote[3]=other;
    for(id=0;id<10;++id) {
        stats.accepted=stats.rejected=0;out=local;
        CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_append(&w,&id,2) && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1,"player lookup receives real checksummed packet");
        if(id==9) CHECK(bh_packet_read_actor(&r,9,local,remote,&out)==1 && out==local,"local ID takes priority before remote range check");
        else if(id==3) CHECK(bh_packet_read_actor(&r,9,local,remote,&out)==1 && out==other,"remote slot resolves borrowed actual entity");
        else if(id<8) CHECK(bh_packet_read_actor(&r,9,local,remote,&out)==0 && out==local && stats.accepted==1 && !stats.rejected,"missing remote preserves output without rejecting packet");
        else CHECK(bh_packet_read_actor(&r,9,local,remote,&out)==0 && out==local && !stats.accepted && stats.rejected==1,"nonlocal out-of-range ID rejects packet");
        CHECK(r.cursor==10,"all player outcomes consume index word");
    }
    id=9;CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_append(&w,&id,2) && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && bh_packet_read_actor(&r,9,NULL,remote,&out)==1 && !out,"original local ID success includes NULL local pointer");
    id=65535;out=local;stats.accepted=stats.rejected=0;
    CHECK(bh_packet_begin(&w,data,sizeof(data),6,0) && bh_packet_append(&w,&id,2) && bh_packet_finish(&w,&size) && bh_packet_receive(&r,data,size,sizeof(data),&stats)==1 && bh_packet_read_actor(&r,0xffffffffUL,local,remote,&out)==0 && out==local && !stats.accepted && stats.rejected==1,"32-bit unassigned local ID must not narrow to wire ushort 65535");
    CHECK(bh_entities_dispose(&pool),"source entity lifetime cleanup");
}
int bh_network_codec_checkpoint(unsigned long *count) {
    checks=0;failed=0;envelope_cases();position_cases();vector_actor_cases();*count=checks;
    printf("{\"scenario\":\"network-codec\",\"checks\":%lu,\"failures\":%d,\"network_sends\":0,\"original_code_required\":false}\n",checks,failed);
    return failed!=0;
}
