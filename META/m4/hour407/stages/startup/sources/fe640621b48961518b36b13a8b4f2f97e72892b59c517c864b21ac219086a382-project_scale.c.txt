#include "../src/bh_project_scale.h"
#include "../src/bh_fp.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures;
#define CHECK(x,s) do{++checks;if(!(x)){++failures;if(failures<12)printf("FAIL project-scale %lu: %s\n",checks,s);}}while(0)
static void fixture(BHMainCamera *camera,BHViewConfig *view){memset(camera,0,sizeof(*camera));memset(view,0,sizeof(*view));camera->projected.m[0][0]=camera->projected.m[1][1]=camera->projected.m[2][2]=1;camera->focal=8;view->near_z=1;}
static void direct(void){BHMainCamera camera,saved;BHViewConfig view;float x,y,z,in[3],shared;unsigned long i,bits=0x7fc01234UL;float depths[5]={.5f,1,2,8,-1};
 fixture(&camera,&view);saved=camera;
 for(i=0;i<5;++i){x=4;y=6;z=depths[i];in[0]=x;in[1]=y;in[2]=z;
  if(i==0||i==4){CHECK(!bh_project_point_scale(&camera,&view,&x,&y,&z),"behind-near world point rejected");CHECK(!memcmp(&x,in,4)&&!memcmp(&y,in+1,4)&&!memcmp(&z,in+2,4),"depth rejection leaves all caller scalars bit-identical");}
  else{CHECK(bh_project_point_scale(&camera,&view,&x,&y,&z),"near equality and in-front points accepted");CHECK(x==4/depths[i]&&y==6/depths[i]&&z==8/depths[i],"screen division and focal/depth scale replace original world coordinates");}
 }
 CHECK(!memcmp(&saved,&camera,sizeof(camera)),"actual projected camera remains borrowed and unmodified");
 x=4;y=6;z=0;view.near_z=0;CHECK(bh_project_point_scale(&camera,&view,&x,&y,&z)&&!_finite(x)&&!_finite(y)&&!_finite(z),"accepted zero depth retains masked division by zero behavior");
 fixture(&camera,&view);x=4;y=6;memcpy(&z,&bits,4);memcpy(in+2,&z,4);CHECK(!bh_project_point_scale(&camera,&view,&x,&y,&z)&&x==4&&y==6&&!memcmp(&z,in+2,4),"unordered transformed depth rejects without overwriting NaN input bits");
 x=4;y=6;z=2;memcpy(&view.near_z,&bits,4);CHECK(!bh_project_point_scale(&camera,&view,&x,&y,&z)&&x==4&&y==6&&z==2,"unordered near plane follows original C0 rejection");
 fixture(&camera,&view);shared=2;y=6;CHECK(bh_project_point_scale(&camera,&view,&shared,&y,&shared)&&shared==4&&y==3,"aliased X/Z uses original input then final scale store wins");
 shared=2;CHECK(bh_project_point_scale(&camera,&view,&shared,&shared,&shared)&&shared==4,"all scalar aliases use snapshotted world inputs and final scale wins");
 x=4;y=6;z=2;CHECK(!bh_project_point_scale(NULL,&view,&x,&y,&z)&&x==4&&y==6&&z==2,"invalid source binding rejects before output writes");
}
static void real_camera(void){BHMainCamera camera;BHViewConfig view;float x=2,y=3,z=0;
 memset(&camera,0,sizeof(camera));CHECK(bh_view_config_init(&view,640,480)&&bh_main_camera_settings(&view,90,1,1000)&&bh_main_camera_pose(&view,&camera,0,0,-10,0,0,0),"actual source viewport/settings/pose build projected camera");
 CHECK(bh_project_point_scale(&camera,&view,&x,&y,&z),"whole projection consumes actual camera build");CHECK(fabs((double)x-384)<.0001&&fabs((double)y-336)<.0001&&fabs((double)z-32)<.0001,"known640x480 camera point maps to384,336 and32 scale");
 x=2;y=3;z=-10;CHECK(!bh_project_point_scale(&camera,&view,&x,&y,&z)&&x==2&&y==3&&z==-10,"actual camera rejects point at its eye retaining source inputs");
}
int __cdecl bh_project_scale_checkpoint(unsigned long *count){BHMathEnvironment fp;checks=failures=0;if(!bh_math_environment_enter(&fp))return 2;direct();real_camera();bh_math_environment_leave(&fp);if(count)*count=checks;
 printf("{\"scenario\":\"project-scale\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);return failures!=0;}
#ifdef BH_PROJECT_SCALE_STANDALONE
int main(void){unsigned long count;return bh_project_scale_checkpoint(&count);}
#endif
