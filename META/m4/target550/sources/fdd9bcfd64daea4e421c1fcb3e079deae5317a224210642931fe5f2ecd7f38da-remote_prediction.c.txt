#include "../src/bh_remote_prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,n) do{++checks;if(!(x)){++failures;printf("FAIL remote prediction %s\n",n);}}while(0)
int bh_remote_prediction_checkpoint(unsigned long *count) {
    BHEntity entity;BHVec3 anchor={10,20,30},velocity={2,4,6},acceleration={3,5,7},out;
    unsigned short stamp;unsigned long rows[2][32];long selected,expected,rank;unsigned long seed,i,product;int next,r;
    checks=0;failures=0;memset(&entity,0,sizeof(entity));memset(rows,0,sizeof(rows));
    memcpy((unsigned char*)&entity+0x180,&anchor,12);memcpy((unsigned char*)&entity+0x48,&velocity,12);memcpy((unsigned char*)&entity+0x60,&acceleration,12);
    stamp=100;memcpy((unsigned char*)&entity+0x17c,&stamp,2);
    CHECK(bh_remote_predict_position(&entity,102,&out)&&out.x==20&&out.y==38&&out.z==56,"velocity plus acceleration two ticks");
    CHECK(bh_remote_predict_position(&entity,100,&out)&&!memcmp(&out,&anchor,12),"same tick copies exact anchor");
    CHECK(bh_remote_predict_position(&entity,99,&out)&&!memcmp(&out,&anchor,12),"negative delta clamps anchor");
    stamp=65535;memcpy((unsigned char*)&entity+0x17c,&stamp,2);
    CHECK(bh_remote_predict_position(&entity,1,&out)&&out.x==20&&out.y==38&&out.z==56,"signed16 wrap forward");
    CHECK(bh_remote_predict_position(&entity,32767,&out)&&!memcmp(&out,&anchor,12),"half-range signed16 is negative");
    stamp=0;velocity.x=16777216.0f;anchor.x=1;acceleration.x=2;
    memcpy((unsigned char*)&entity+0x17c,&stamp,2);memcpy((unsigned char*)&entity+0x180,&anchor,12);
    memcpy((unsigned char*)&entity+0x48,&velocity,12);memcpy((unsigned char*)&entity+0x60,&acceleration,12);
    CHECK(bh_remote_predict_position(&entity,1,&out)&&out.x==16777216.0f,"binary32 intermediate velocity store before acceleration add");
    rows[0][0]=1;rows[0][7]=4;rows[0][30]=9;rows[0][31]=3;
    for(seed=0;seed<1024;++seed) {
        srand(seed);r=rand();next=rand();srand(seed);
        CHECK(bh_remote_choose_slot(rows,2,0,7,30,&selected)&&selected==(r<16384?7:30),"two valid preferences exact threshold");CHECK(rand()==next,"two preferences consumes one CRT random");
        srand(seed);r=rand();srand(seed);CHECK(bh_remote_choose_slot(rows,2,0,7,-1,&selected)&&selected==7&&rand()==r,"single first no RNG");
        srand(seed);CHECK(bh_remote_choose_slot(rows,2,0,-1,30,&selected)&&selected==30&&rand()==r,"single second no RNG");
        srand(seed);r=rand();next=rand();srand(seed);rank=r*3/32768;expected=rank==0?0:rank==1?7:30;
        CHECK(bh_remote_choose_slot(rows,2,0,-1,-1,&selected)&&selected==expected&&rand()==next,"ordinal fallback and exact RNG step");
    }
    rows[0][31]=0;CHECK(bh_remote_choose_slot(rows,2,0,-1,-1,&selected)&&selected==0,"zero authoritative count selects first present");
    rows[0][31]=0xffffffffUL;srand(73);product=(unsigned long)rand()*rows[0][31];memcpy(&rank,&product,4);rank/=32768;expected=0;
    for(i=0;i<3;++i)if((long)i==rank)expected=i==0?0:i==1?7:30;
    srand(73);CHECK(bh_remote_choose_slot(rows,2,0,-1,-1,&selected)&&selected==expected,"signed32 wrap product truncation");
    CHECK(bh_remote_choose_slot(rows,2,1,-1,-1,&selected)&&selected==0,"empty row loop exhausts to original zero");
    CHECK(!bh_remote_choose_slot(rows,2,0,31,-1,&selected),"source slot bound");
    if(count)*count+=checks;printf("{\"scenario\":\"remote-prediction\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures?1:0;
}
#ifdef BH_REMOTE_PREDICTION_STANDALONE
int main(void){unsigned long n=0;return bh_remote_prediction_checkpoint(&n);}
#endif
