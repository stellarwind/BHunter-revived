#include "../src/bh_view_cull.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<18)printf("FAIL view cull %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<=8e-6*(1+fabs(b));}
/* Independent analytical sphere interval oracle. Depth represents a slab;
   other planes classify the interval of signed distances over the sphere. */
static unsigned long oracle(const BHViewConfig *c,const BHMainCamera *cam,const BHVec3 *p,float radius,unsigned long mask,double *out_depth) {
    double distance[6],lo,hi;unsigned long slot,bit;
    for(slot=0;slot<6;++slot)if(slot!=1)distance[slot]=((double)cam->planes[slot].normal.x*p->x+(double)cam->planes[slot].normal.y*p->y)+(double)cam->planes[slot].normal.z*p->z-cam->planes[slot].offset;
    *out_depth=distance[0];
    if(mask&3){lo=distance[0]-radius;hi=distance[0]+radius;if(hi<c->near_z || lo>c->far_z)return 0xffffffffUL;
        if(lo>c->near_z)mask&=0x3e;if(hi<c->far_z)mask&=0x3d;}
    for(slot=2,bit=4;slot<6;++slot,bit*=2)if(mask&bit){lo=distance[slot]-radius;hi=distance[slot]+radius;if(lo>0)return 0xffffffffUL;if(hi<0)mask^=bit;}
    return mask;
}
static void boundaries(void) {
    BHMainCamera camera;BHViewConfig c;float depth;unsigned long bit,i;
    memset(&camera,0,sizeof(camera));memset(&c,0,sizeof(c));c.near_z=10;c.far_z=30;
    camera.planes[0].normal.z=1;for(i=2;i<6;++i)camera.planes[i].normal.x=1;
    CHECK(bh_view_cull_sphere(NULL,NULL,0,0,0,1,0,NULL)==0,"mask0 and absent output reads neither context");
    CHECK(bh_view_cull_sphere(NULL,NULL,0,0,0,1,0x80000000UL,NULL)==0x80000000UL,"unknown-only mask without depth output reads no context and preserves high bits");
    depth=-99;CHECK(bh_view_cull_sphere(NULL,&camera,0,0,17,1,0,&depth)==0 && depth==17,"mask0 still writes requested signed depth");
    depth=-99;CHECK(bh_view_cull_sphere(NULL,&camera,0,0,17,1,0x80000000UL,&depth)==0x80000000UL && depth==17,"unknown-only mask still writes requested depth without config");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,8,2,1,NULL)==1,"near bit remains at lower depth tangent");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,12,2,1,NULL)==1,"near bit remains at upper depth tangent");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,28,2,2,NULL)==2 && bh_view_cull_sphere(&c,&camera,0,0,32,2,2,NULL)==2,"far bit remains at both tangencies");
    depth=-99;CHECK(bh_view_cull_sphere(&c,&camera,0,0,7.5f,2,2,&depth)==0xffffffffUL && depth==7.5f,"far-only mask still invokes near rejection and writes depth before rejection");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,32.5f,2,1,NULL)==0xffffffffUL,"near-only mask still invokes far rejection");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,20,2,0x80000003UL,NULL)==0,"depth clears truncate original unknown high mask bits");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,29,2,0x80000003UL,NULL)==2,"near-clear AND0x3e truncates high bits while retaining far intersection");
    CHECK(bh_view_cull_sphere(&c,&camera,0,0,11,2,0x80000003UL,NULL)==1,"far-clear AND0x3d truncates high bits while retaining near intersection");
    c.far_z=10;CHECK(bh_view_cull_sphere(&c,&camera,0,0,10,2,0x80000003UL,NULL)==0x80000003UL,"overlapping near/far spheres preserve high bits when neither clear executes");c.far_z=30;
    for(bit=4;bit<=32;bit*=2){CHECK(bh_view_cull_sphere(NULL,&camera,2,0,0,2,bit,NULL)==bit,"side outside tangent retained");
        CHECK(bh_view_cull_sphere(NULL,&camera,-2,0,0,2,bit,NULL)==bit,"side inside tangent retained");
        CHECK(bh_view_cull_sphere(NULL,&camera,2.125f,0,0,2,bit,NULL)==0xffffffffUL,"side positive distance beyond radius rejects");
        CHECK(bh_view_cull_sphere(NULL,&camera,-2.125f,0,0,2,0x80000000UL|bit,NULL)==0x80000000UL,"side subtraction preserves unknown high bits");}
    /* FST stores 1.0, but retained depth is1+2^-30. Reloading the output would
       incorrectly retain the near bit at a strict equality boundary. */
    c.near_z=1;c.far_z=4;camera.planes[0].normal.x=1;camera.planes[0].normal.y=1;camera.planes[0].normal.z=0;
    depth=-99;CHECK(bh_view_cull_sphere(&c,&camera,1,0.000000000931322574615478515625f,0,0,1,&depth)==0 && depth==1,"depth FST binary32 output does not round retained depth comparisons");
    CHECK(bh_view_cull_sphere(&c,&camera,1,0,0,0,1,NULL)==1,"positive control exact near equality retains bit");
}
static void poses(void) {
    BHMainCamera camera;BHViewConfig c;BHVec3 p;unsigned long n,i,mask,expected,actual;double expected_depth;float stored_depth,depth,radius;
    for(n=0;n<32;++n){memset(&camera,0,sizeof(camera));bh_view_config_init(&c,640+(long)n,480);c.near_z=1;c.far_z=80;
        CHECK(bh_main_camera_pose(&c,&camera,3,-5,7,n*.03125,n*-.015625,n*.0078125),"real global camera pose owner provides original five-plane context");
        for(i=0;i<40;++i){p.x=(float)((int)(i%9)*3-12);p.y=(float)((int)(i%7)*2-6);p.z=(float)((int)i*3-10);radius=(float)(i%5)*.5f;
            for(mask=0;mask<64;++mask){expected=oracle(&c,&camera,&p,radius,mask,&expected_depth);bh_round_float(&stored_depth,expected_depth);
                depth=-999;actual=bh_view_cull_sphere(&c,&camera,p.x,p.y,p.z,radius,mask,&depth);
                CHECK(actual==expected,"varied pose/radius/all64 masks match independent sphere interval oracle");
                CHECK(depth==stored_depth,"optional depth uses exactly stored signed global forward-plane distance");
                CHECK(bh_view_cull_sphere(&c,&camera,p.x,p.y,p.z,radius,mask,NULL)==actual,"absent depth output preserves classification");}
        }
        p=camera.position;depth=-999;bh_view_cull_sphere(NULL,&camera,p.x,p.y,p.z,0,0,&depth);CHECK(close_value(depth,0),"global eye has zero analytical forward depth");
    }
}
int bh_view_cull_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    boundaries();poses();CHECK(bh_math_environment_supported(),"view cull preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"view-cull\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_VIEW_CULL_STANDALONE
int main(void){unsigned long count;return bh_view_cull_checkpoint(&count);}
#endif
