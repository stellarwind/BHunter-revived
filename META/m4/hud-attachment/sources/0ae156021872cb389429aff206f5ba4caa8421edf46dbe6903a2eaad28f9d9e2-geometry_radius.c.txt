#include "../src/bh_geometry_radius.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL geometry radius %d: %s\n",__LINE__,label);}} while(0)
static void word(unsigned char *p,unsigned long value){memcpy(p,&value,4);}
static int fixture(const char *name,const float *minimum,const float *maximum) {
    unsigned char data[272];FILE *f;memset(data,0,sizeof(data));word(data,3);
    strcpy((char *)data+45,"PATHIMG.PCX");word(data+100,1);word(data+164,1);
    word(data+172,0xffffffffUL);word(data+184,1);word(data+188,1);word(data+200,1);
    memcpy(data+204,minimum,12);memcpy(data+216,maximum,12);{float one=1;memcpy(data+228,&one,4);}
    word(data+244,0xffffffffUL);word(data+248,0xffffffffUL);word(data+252,0xffffffffUL);
    word(data+256,0xffffffffUL);word(data+260,1);word(data+268,0xffffffffUL);
    f=fopen(name,"wb");if(!f)return 0;if(fwrite(data,1,sizeof(data),f)!=sizeof(data)){fclose(f);return 0;}return !fclose(f);
}
int bh_geometry_radius_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;BHBsp bsp;BHBspNode nodes[2];double result;
    float rounded;float minimum[3]={-10,-1,0},maximum[3]={1,10,0};unsigned long nanbits=0x7fc00000UL;
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    CHECK(fixture("RADIUS.BSP",minimum,maximum),"real mixedsign BSP fixture");
    CHECK(bh_bsp_init(&bsp,NULL) && bh_bsp_load(&bsp,"RADIUS.BSP","",BH_BSP_BARE_ONLY),"real VFILE/BSP decoder allocations and validation");
    result=bh_geometry_radius(&bsp);
    CHECK(result==sqrt(101.0) && result<11 && bsp.root_radius>14,"corner norms differ from existing peraxismax rootradius and retain firstnorm precision");
    CHECK(bh_geometry_radius(&bsp)==result,"source owner remains stable across repeated calls");bh_bsp_dispose(&bsp);
    memset(&bsp,0,sizeof(bsp));memset(nodes,0,sizeof(nodes));bsp.nodes=nodes;bsp.counts[4]=2;
    nodes[0].maximum[0]=1;nodes[0].maximum[1]=1;nodes[1].maximum[0]=10000;
    bh_round_float(&rounded,sqrt(2.0));
    CHECK(bh_geometry_radius(&bsp)==(double)rounded && fabs(bh_geometry_radius(&bsp)-1.4142135623730951)>1e-8,"secondnorm rounds tofloat and only firstnode is consumed");
    nodes[0].minimum[0]=-1;nodes[0].minimum[1]=-1;nodes[0].minimum[2]=-1;
    nodes[0].maximum[0]=nodes[0].maximum[1]=nodes[0].maximum[2]=0;
    bh_round_float(&rounded,sqrt(3.0));
    CHECK(fabs(bh_geometry_radius(&bsp)-1.7320508075688772)<1e-15 && fabs(bh_geometry_radius(&bsp)-(double)rounded)>1e-8,"winning firstnorm retains double PC53 precision");
    nodes[0].minimum[0]=-3;nodes[0].minimum[1]=-4;nodes[0].minimum[2]=0;
    nodes[0].maximum[0]=3;nodes[0].maximum[1]=4;
    CHECK(bh_geometry_radius(&bsp)==5,"equal exact cornernorm tie chooses equal roundedsecond");
    memset(nodes,0,sizeof(nodes));CHECK(bh_geometry_radius(&bsp)==0,"zero corner norms");
    nodes[0].minimum[0]=-2;nodes[0].minimum[1]=-3;nodes[0].minimum[2]=-6;
    CHECK(bh_geometry_radius(&bsp)==7,"independent2/3/6 cornernorm");
    memcpy(&nodes[0].maximum[1],&nanbits,4);CHECK(bh_geometry_radius(&bsp)==0,"explicit nonfinite input guard");
    bsp.counts[4]=0;CHECK(bh_geometry_radius(&bsp)==0,"empty firstnode guard");
    bsp.counts[4]=1;bsp.nodes=NULL;CHECK(bh_geometry_radius(&bsp)==0 && bh_geometry_radius(NULL)==0,"NULL owner/node sourceguards");
    remove("RADIUS.BSP");bh_vfile_shutdown();bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"geometry-radius\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_GEOMETRY_RADIUS_LINKED
int main(void){unsigned long count;return bh_geometry_radius_checkpoint(&count);}
#endif
