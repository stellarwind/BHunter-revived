#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;printf("FAIL callback-helpers %d: %s\n",__LINE__,s);}}while(0)
static void put(BHEntity *e,unsigned long offset,unsigned long value){memcpy((unsigned char*)e+offset,&value,4);}
static unsigned long get(BHEntity *e,unsigned long offset){unsigned long value;memcpy(&value,(unsigned char*)e+offset,4);return value;}
static void query_cases(void){
    BHEntityPool pool;BHEntity *e,*target;unsigned long packet[4],i,j;unsigned char before[sizeof(BHEntity)];
    static const unsigned long offsets[3]={0x1b4,0xf4,0x104};
    BHEntityCallback callbacks[3]={bh_actor_target_removed,bh_effect_target_removed,bh_pickup_target_removed};
    CHECK(sizeof(void*)==4&&offsetof(BHEntity,payload)==0xe4,"real entity x86 layout");
    CHECK(bh_entities_init(&pool,2,NULL),"real pool init");e=bh_entity_acquire(&pool);target=bh_entity_acquire(&pool);
    CHECK(e&&target,"real entity ownership");if(!e||!target)return;
    for(i=0;i<3;++i){
        CHECK(bh_entity_set_query(&pool,e,callbacks[i]),"actual query registration");
        put(e,offsets[i],(unsigned long)target);put(e,0xfc,0xaabbccdd);memcpy(before,e,sizeof(*e));
        packet[0]=16;packet[1]=(unsigned long)target;callbacks[i](e,packet);CHECK(!memcmp(before,e,sizeof(*e)),"wrong event preserves whole slot");
        packet[0]=17;packet[1]=(unsigned long)e;callbacks[i](e,packet);CHECK(!memcmp(before,e,sizeof(*e)),"different target preserves whole slot");
        packet[1]=(unsigned long)target;
        CHECK(bh_entities_query(&pool,0,0,NULL,0,packet),"real query owner calls removal callback");
        CHECK(!get(e,offsets[i])&&(i!=1||!get(e,0xfc)),"matched target clear and paired effect state");
        memset(before+offsets[i],0,4);if(i==1)memset(before+0xfc,0,4);
        CHECK(!memcmp(before,e,sizeof(*e)),"no other slot byte changed");
        for(j=0;j<4;++j){packet[0]=17;packet[1]=0;callbacks[i](e,packet);}
    }
    CHECK(bh_entity_set_query(&pool,e,bh_marker_query),"real marker query registration");e->position.x=1;e->position.y=-2;e->position.z=3;
    packet[0]=3;packet[1]=packet[2]=packet[3]=0;
    CHECK(bh_entities_query(&pool,0,0,NULL,0,packet)&&!memcmp(packet+1,&e->position,12),"position response through actual query traversal");
    put(e,0x108,0x12345678);packet[0]=20;bh_marker_query(e,packet);CHECK(!get(e,0x108),"detach event clears owner reference");
    memcpy(before,e,sizeof(*e));packet[0]=99;bh_marker_query(e,packet);CHECK(!memcmp(before,e,sizeof(*e)),"unhandled query preserves slot");
    CHECK(bh_entity_release(&pool,target)&&bh_entity_release(&pool,e)&&bh_entities_dispose(&pool),"actual pool teardown");
}
static void numeric_cases(void){
    BHVec3 a,b,out,alias,expected;float scales[5]={0,1,-1,128,0.125f};
    unsigned char data[264];BHPacketWriter w;BHPacketReader r;BHPacketStats stats;unsigned long wire,i,j,k;BHMathEnvironment fp;
    CHECK(bh_math_environment_enter(&fp),"PC53 enter");
    a.x=-4;a.y=8;a.z=16;b.x=4;b.y=16;b.z=-16;
    for(i=0;i<5;++i){float f=scales[i];expected.x=(float)(((double)b.x-a.x)*f+a.x);expected.y=(float)(((double)b.y-a.y)*f+a.y);expected.z=(float)(((double)b.z-a.z)*f+a.z);
        bh_vec3_interpolate(f,&out,&a,&b);CHECK(!memcmp(&out,&expected,12),"interpolation endpoints and extrapolation");
        alias=a;bh_vec3_interpolate(f,&alias,&alias,&b);CHECK(!memcmp(&alias,&expected,12),"in-place first endpoint");
        alias=b;bh_vec3_interpolate(f,&alias,&a,&alias);CHECK(!memcmp(&alias,&expected,12),"in-place second endpoint");
    }
    a.x=a.y=a.z=1;b.x=b.y=b.z=1.0f/33554432.0f;
    bh_vec3_interpolate(0.75f,&out,&a,&b);
    {unsigned long bits[3];memcpy(bits,&out,12);CHECK(bits[0]==0x3e800001&&bits[1]==0x3e800001&&bits[2]==0x3e800001,"retained x87 differences including non-popping z spill");}
    for(i=0;i<256;++i)for(j=0;j<5;++j){
        unsigned char bytes[3];bytes[0]=(unsigned char)i;bytes[1]=(unsigned char)(255-i);bytes[2]=0x80;memset(&stats,0,sizeof(stats));
        CHECK(bh_packet_begin(&w,data,sizeof(data),7,0)&&bh_packet_append(&w,bytes,3)&&bh_packet_finish(&w,&wire),"real packet construction");
        CHECK(bh_packet_receive(&r,data,wire,sizeof(data),&stats)==1&&bh_packet_read_vector(&r,&out,scales[j]),"real receive and signed vector decoding");
        expected.x=(float)((int)(signed char)bytes[0]*((double)scales[j]/128));expected.y=(float)((int)(signed char)bytes[1]*((double)scales[j]/128));expected.z=(float)(-128*((double)scales[j]/128));
        CHECK(!memcmp(&out,&expected,12)&&r.cursor==11&&stats.accepted==1,"all signed bytes and retained denominator");
        for(k=0;k<3;++k){unsigned long saved;out=a;r.cursor=wire-k;saved=r.cursor;
            CHECK(!bh_packet_read_vector(&r,&out,1)&&r.cursor==saved&&r.error==BH_PACKET_TRUNCATED&&!memcmp(&out,&a,12),"bounded truncated input atomic");}
    }
    bh_math_environment_leave(&fp);
}
int bh_callback_helpers_checkpoint(unsigned long *count){checks=0;failures=0;query_cases();numeric_cases();*count=checks;printf("{\"scenario\":\"callback-helpers\",\"checks\":%lu,\"failures\":%d}\n",checks,failures);return failures;}
