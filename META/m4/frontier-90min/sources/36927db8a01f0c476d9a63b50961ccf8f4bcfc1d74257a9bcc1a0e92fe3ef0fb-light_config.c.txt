#include "../src/bh_light_config.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) do {++checks;if(!(x)){++failures;printf("FAIL light config %d: %s\n",__LINE__,label);}} while(0)
static unsigned long bits(float x){unsigned long value;memcpy(&value,&x,4);return value;}
static int vector(const BHVec3 *v,float x,float y,float z){return fabs(v->x-x)<1e-7 && fabs(v->y-y)<1e-7 && fabs(v->z-z)<1e-7;}
static int rgb(const float *p,float r,float g,float b){return p[0]==r && p[1]==g && p[2]==b;}
int bh_light_config_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;BHLightConfig config,saved;unsigned long i;
    static const struct {unsigned long color;float strength,r,g,b;} cases[]={
        {0x123456,.5f,9,26,43},{0xff123456,-.25f,-4.5f,-13,-21.5f},
        {0xff0000,2,510,0,0},{0x00ff00,.125f,0,31.875f,0},
        {0x0000ff,1,0,0,255},{0xffffff,-2,-510,-510,-510},{0xffffffff,0,0,0,0}
    };
    checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    memset(&config,0xa5,sizeof(config));saved=config;saved.enabled=0;
    CHECK(bh_light_configure(&config,0,3,4,5,1,2,0x123456,0x789abc) && !memcmp(&config,&saved,sizeof(config)),"disabled stores only raw zero enable, retains every other bit");
    CHECK(!bh_light_configure(NULL,1,0,0,0,0,0,0,0),"source NULL output guard");
    CHECK(bh_light_configure(&config,7,3,4,0,.5f,.25f,0x123456,0x789abc) && config.enabled==7,"raw nonboolean enable value retained");
    CHECK(vector(&config.direction,-.6f,-.8f,0) && bits(config.direction.z)==0x80000000UL,"actual admitted normalizer receives negated direction and retains negativezero");
    CHECK(rgb(config.directional,9,26,43) && rgb(config.ambient,30,38.5f,47),"independent handcomputed RGB groups use distinct strengths and channels");
    saved=config;CHECK(bh_light_configure(&config,0,0,0,0,99,99,0xffffffff,0xffffffff),"disable existing configured light");saved.enabled=0;
    CHECK(!memcmp(&config,&saved,sizeof(config)),"disable retains previous normalized direction and both groups");
    CHECK(bh_light_configure(&config,-1,0,0,0,1,1,0,0) && config.enabled==-1,"negative raw enable remains enabled");
    CHECK(bits(config.direction.x)==0x80000000UL && bits(config.direction.y)==0x80000000UL && bits(config.direction.z)==0x80000000UL,"zero direction follows actual normalizer zero path without division");
    CHECK(bh_light_configure(&config,1,0,-12,5,1,1,0,0) && vector(&config.direction,0,12.0f/13,-5.0f/13),"independent5/12/13 normalized direction");
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        CHECK(bh_light_configure(&config,1,1,0,0,cases[i].strength,cases[i].strength,cases[i].color,cases[i].color),"varied packed RGB and signed strength");
        CHECK(rgb(config.directional,cases[i].r,cases[i].g,cases[i].b) && rgb(config.ambient,cases[i].r,cases[i].g,cases[i].b),"independent exact dyadic color fixture without normalization/clamp/alpha");
    }
    config.direction.x=3;config.direction.y=4;config.direction.z=0;config.directional[0]=.5f;config.ambient[0]=.25f;
    CHECK(bh_light_configure(&config,config.enabled,config.direction.x,config.direction.y,config.direction.z,config.directional[0],config.ambient[0],0x123456,0x789abc) && vector(&config.direction,-.6f,-.8f,0) && rgb(config.directional,9,26,43) && rgb(config.ambient,30,38.5f,47),"byvalue original arguments safely snapshot aliased output fields");
    CHECK(bh_math_environment_supported(),"configuration retains admitted numeric environment");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"light-config\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifndef BH_LIGHT_CONFIG_LINKED
int main(void){unsigned long count;return bh_light_config_checkpoint(&count);}
#endif
