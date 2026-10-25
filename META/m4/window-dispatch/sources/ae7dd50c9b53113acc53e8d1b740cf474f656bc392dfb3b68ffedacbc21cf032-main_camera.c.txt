#include "../src/bh_main_camera.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<16)printf("FAIL main camera %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<=5e-6*(1+fabs(b));}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static double distance(const BHViewPlane *p,const BHVec3 *v){return ((double)p->normal.x*v->x+(double)p->normal.y*v->y)+(double)p->normal.z*v->z-p->offset;}
static void settings_and_planes(void) {
    BHMainCamera camera,prior;BHViewConfig c,prior_config;BHMat34 m;BHVec3 p;unsigned long i;
    memset(&camera,0,sizeof(camera));memset(&camera.planes[1],0x5a,sizeof(camera.planes[1]));camera.projection_state=59;
    camera.angles.x=2;camera.view.m[0][3]=71;prior=camera;
    CHECK(bh_view_config_init(&c,640,480),"explicit viewport context adapter");prior_config=c;
    CHECK(bh_main_camera_settings(&c,90,2,900) && close_value(c.fov,1.57079632679) && c.near_z==2 && c.far_z==900,"degree FOV and binary32 near/far settings");
    CHECK(c.width==prior_config.width && c.height==prior_config.height && c.left==prior_config.left,"settings preserve viewport and rectangle");
    identity(&m);CHECK(bh_main_camera_build(&c,&m,&camera),"separate complete global camera build");
    CHECK(camera.projection_state==0 && camera.angles.x==2 && camera.view.m[0][3]==71,"build resets original projection state but retains pose storage");
    CHECK(!memcmp(camera.planes+1,prior.planes+1,sizeof(camera.planes[1])),"original reserved plane slot1 remains byte-identical");
    CHECK(camera.planes[0].normal.x==0 && camera.planes[0].normal.y==0 && camera.planes[0].normal.z==1 && camera.planes[0].offset==0,"global plane0 is positive forward through eye");
    CHECK(close_value(camera.focal,320) && camera.projected.m[0][2]==320 && camera.projected.m[1][2]==240,"analytic 90-degree projection");
    p.x=p.y=0;p.z=3;CHECK(distance(camera.planes,&p)>0,"forward plane has original opposite sign to near clipping plane");
    for(i=2;i<6;++i)CHECK(distance(camera.planes+i,&p)<0,"interior point satisfies outward side-plane inequality");
    p.z=.01f;for(i=2;i<6;++i)CHECK(distance(camera.planes+i,&p)<0,"depth settings do not shift side planes or add near rejection");
    prior=camera;c.near_z=30;c.far_z=31;CHECK(bh_main_camera_build(&c,&m,&camera) && !memcmp(&camera,&prior,sizeof(camera)),"projection ignores depth settings unlike per-model owner");
    prior_config=c;CHECK(!bh_main_camera_settings(&c,0,1,3) && !memcmp(&c,&prior_config,sizeof(c)),"guard failure preserves configuration");
    c.height=0;prior=camera;CHECK(!bh_main_camera_build(&c,&m,&camera) && !memcmp(&camera,&prior,sizeof(camera)),"invalid viewport does not publish partial global state");
    CHECK(sizeof(BHViewPlane)==16 && offsetof(BHViewPlane,offset)==12,"original plane slot size and offset");
}
static void projection_controls(void) {
    BHMainCamera camera,other;BHViewConfig c;BHMat34 m;BHVec3 world,local,screen;double angle,cs,sn,focal,expected,vertical;unsigned long n,i,j;
    for(n=0;n<48;++n) {
        memset(&camera,0,sizeof(camera));bh_view_config_init(&c,321+(long)n*7,239+(long)n*3);
        c.fov=(float)(.4+n*.035);c.origin_x=.1f;c.origin_y=-3.125f;identity(&m);
        angle=n*.07;cs=(float)cos(angle);sn=(float)sin(angle);
        m.m[0][0]=m.m[2][2]=(float)cs;m.m[0][2]=(float)sn;m.m[2][0]=(float)-sn;m.m[0][3]=3;m.m[1][3]=-5;m.m[2][3]=7;
        CHECK(bh_main_camera_build(&c,&m,&camera),"varied global projection");
        focal=c.width*.5/tan(c.fov*.5);
        for(i=0;i<2;++i)for(j=0;j<4;++j) {
            expected=focal*m.m[i][j]+((i?c.origin_y:c.origin_x)+(i?c.height:c.width)*.5)*m.m[2][j];
            CHECK(close_value(camera.projected.m[i][j],expected),"independent pinhole coefficient formula");
        }
        CHECK(close_value(camera.position.x,-(cs*3-sn*7)) && camera.position.y==5 && close_value(camera.position.z,-(sn*3+cs*7)),"analytic rotated inverse eye");
        local.x=.1f;local.y=.2f;local.z=3;bh_mat34_transform_point(&camera.inverse,&local,&world);bh_mat34_transform_point(&camera.projected,&world,&screen);
        CHECK(close_value(screen.x/screen.z,c.origin_x+c.width*.5+focal*local.x/local.z) && close_value(screen.y/screen.z,c.origin_y+c.height*.5+focal*local.y/local.z),"projected inverse world point agrees with analytic screen coordinates");
        CHECK(close_value(camera.planes[0].normal.x,-sn) && close_value(camera.planes[0].normal.z,cs),"global forward normal uses inverse basis column");
        for(i=0;i<6;++i)if(i!=1)CHECK(fabs(distance(camera.planes+i,&camera.position))<2e-6*(1+fabs(camera.planes[i].offset)),"all five written planes pass through global eye");
        for(i=2;i<6;++i)CHECK(distance(camera.planes+i,&world)<0,"rotated translated interior point inside side planes");
        for(i=4;i<6;++i)CHECK(close_value((double)camera.planes[i].normal.x*camera.planes[i].normal.x+(double)camera.planes[i].normal.y*camera.planes[i].normal.y+(double)camera.planes[i].normal.z*camera.planes[i].normal.z,1),"vertical global planes normalized by real vector owner");
        vertical=((double)(float)cos((double)(float)(c.fov*.5))*c.width)/c.height;
        CHECK(vertical>0,"vertical positive control");
        memset(&other,0,sizeof(other));other.view=m;CHECK(bh_main_camera_build(&c,&other.view,&other) && !memcmp(&camera.projected,&other.projected,sizeof(m)),"input may alias owning view matrix");
    }
    memset(&camera,0,sizeof(camera));bh_view_config_init(&c,32,32);identity(&camera.projected);m=camera.projected;
    CHECK(bh_main_camera_build(&c,&camera.projected,&camera),"projected self-alias source supported");
    CHECK(camera.inverse.m[0][0]==camera.projected.m[0][0] && camera.inverse.m[2][0]==camera.projected.m[0][2],"projected self-alias inverses modified projected matrix as original");
    memset(&camera,0,sizeof(camera));identity(&m);m.m[0][0]=2;m.m[0][3]=3;
    CHECK(bh_main_camera_build(&c,&m,&camera) && camera.inverse.m[0][0]==2 && camera.position.x==-6,"nonrigid input preserves rigid-inverse algorithm");
}
static void pose_and_rotation(void) {
    BHMainCamera camera;BHViewConfig c;BHMat34 m,before;BHVec3 p,transformed;unsigned long n,i,j;float fyaw,fpitch,froll;
    double yaw,pitch,roll,cy,sy,cx,sx,cz,sz,r[3][3],eye[3],expected,x,y;
    bh_view_config_init(&c,640,480);
    for(n=0;n<40;++n) {
        memset(&camera,0,sizeof(camera));bh_round_float(&fyaw,n*.03125);bh_round_float(&fpitch,n*-.02);bh_round_float(&froll,n*.015625);yaw=fyaw;pitch=fpitch;roll=froll;
        eye[0]=3.25;eye[1]=-5;eye[2]=7.5;
        CHECK(bh_main_camera_pose(&c,&camera,eye[0],eye[1],eye[2],yaw,pitch,roll),"full real world Y/X/Z pose dependencies");
        cy=(float)cos(-yaw);sy=(float)sin(-yaw);cx=(float)cos(-pitch);sx=(float)sin(-pitch);cz=(float)cos(-roll);sz=(float)sin(-roll);
        r[0][0]=cz*cy-sz*sx*sy;r[0][1]=-sz*cx;r[0][2]=cz*sy+sz*sx*cy;
        r[1][0]=sz*cy+cz*sx*sy;r[1][1]=cz*cx;r[1][2]=sz*sy-cz*sx*cy;
        r[2][0]=-cx*sy;r[2][1]=sx;r[2][2]=cx*cy;
        for(i=0;i<3;++i) {
            for(j=0;j<3;++j)CHECK(close_value(camera.view.m[i][j],r[i][j]),"independent expanded Rz Rx Ry pose basis");
            expected=-(r[i][0]*eye[0]+r[i][1]*eye[1]+r[i][2]*eye[2]);CHECK(close_value(camera.view.m[i][3],expected),"pose translation equals negative rotated eye");
        }
        CHECK(camera.angles.x==fyaw && camera.angles.y==fpitch && camera.angles.z==froll,"pose angles retain original yaw pitch roll order and binary32 storage");
        p.x=3.25f;p.y=-5;p.z=7.5f;bh_mat34_transform_point(&camera.view,&p,&transformed);
        CHECK(fabs(transformed.x)<2e-6 && fabs(transformed.y)<2e-6 && fabs(transformed.z)<2e-6,"stored pose transforms eye to camera origin");
        CHECK(close_value(camera.position.x,eye[0]) && close_value(camera.position.y,eye[1]) && close_value(camera.position.z,eye[2]),"recomputed global eye agrees within recovered float precision");
        for(i=0;i<3;++i)for(j=0;j<4;++j)m.m[i][j]=(float)(i*4+j+1);
        before=m;bh_mat34_rotate_world_z((float)roll,&m);cz=(float)cos((float)roll);sz=(float)sin((float)roll);
        for(j=0;j<4;++j){x=before.m[0][j];y=before.m[1][j];CHECK(close_value(m.m[0][j],x*cz-y*sz) && close_value(m.m[1][j],x*sz+y*cz) && m.m[2][j]==before.m[2][j],"world Z rotation changes rows0/1 including translation and preserves depth row");}
    }
}
int bh_main_camera_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    settings_and_planes();projection_controls();pose_and_rotation();CHECK(bh_math_environment_supported(),"global camera preserves caller PC53 profile");
    bh_math_environment_leave(&fp);*count=checks;
    printf("{\"scenario\":\"main-camera\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
#ifdef BH_MAIN_CAMERA_STANDALONE
int main(void){unsigned long count;return bh_main_camera_checkpoint(&count);}
#endif
