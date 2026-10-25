#include "../src/bh_view_camera.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
static unsigned long checks;static int failures;
#define CHECK(x,label) check((x)!=0,label,__LINE__)
static void check(int ok,const char *label,int line){++checks;if(!ok){++failures;if(failures<12)printf("FAIL view camera %d: %s\n",line,label);}}
static int close_value(double a,double b){return fabs(a-b)<=3e-6*(1+fabs(b));}
static void identity(BHMat34 *m){memset(m,0,sizeof(*m));m->m[0][0]=m->m[1][1]=m->m[2][2]=1;}
static double distance(const BHViewPlane *p,const BHVec3 *v){return (double)p->normal.x*v->x+(double)p->normal.y*v->y+(double)p->normal.z*v->z-p->offset;}
static int on_plane(const BHViewPlane *p,const BHVec3 *v){int exponent;frexp(fabs(p->offset),&exponent);return fabs(distance(p,v))<=ldexp(1.0,exponent-25)+1e-12*(1+fabs(p->offset));}
static void settings(void) {
    BHViewConfig c;BHViewCamera view;BHMat34 m;unsigned long i;BHVec3 p={0,0,3};
    CHECK(bh_view_config_init(&c,640,480) && c.fov==1.0471975803375244f && c.near_z==1 && c.far_z==100000,"original executable FOV/depth defaults bound explicitly");
    CHECK(c.origin_x==0 && c.origin_y==0 && c.left==0 && c.top==0 && c.right==640 && c.bottom==480,"full viewport owner initializes actual target dimensions and bounds");
    identity(&m);CHECK(bh_view_camera_build(&c,&m,&view),"complete original projection and six world planes");
    CHECK(close_value(view.focal,554.25624) && view.projected.m[0][2]==320 && view.projected.m[1][2]==240 && view.projected.m[2][2]==1,"default60-degree focal and original minus-negative-half screen-center formula");
    CHECK(view.planes[BH_VIEW_NEAR].normal.z==-1 && view.planes[BH_VIEW_NEAR].offset==-1 && view.planes[BH_VIEW_FAR].normal.z==1 && view.planes[BH_VIEW_FAR].offset==100000,"exact near/far outward plane signs and depths");
    for(i=0;i<6;++i)CHECK(distance(view.planes+i,&p)<0,"known camera-center point lies inside every original plane");
    p.z=.5f;CHECK(distance(view.planes+BH_VIEW_NEAR,&p)>0,"near plane rejects in front of near depth");p.z=100001;CHECK(distance(view.planes+BH_VIEW_FAR,&p)>0,"far plane rejects beyond configured depth");
    CHECK(bh_view_clip_rect(&c,-10,-20,1000,1000) && c.left==0 && c.top==0 && c.right==640 && c.bottom==480,"clip rectangle asymmetric clamps preserve original sums before clamping");
    CHECK(bh_view_clip_rect(&c,700,600,-800,-700) && c.left==700 && c.top==600 && c.right==-100 && c.bottom==-100,"original inverted bounds are retained without silently repairing malformed rectangle");
    CHECK(c.width==640 && c.height==480 && c.origin_x==0 && c.origin_y==0,"clip rectangle does not redefine projection dimensions/origin");
    CHECK(bh_view_reset_viewport(&c,800,600) && c.right==800 && c.bottom==600 && c.left==0 && c.top==0 && c.fov==1.0471975803375244f && c.near_z==1,"viewport reset preserves FOV/depth while replacing target/bounds");
}
static void varied(void) {
    BHViewConfig c;BHViewCamera view;BHMat34 m;BHVec3 local,world,projected;unsigned long n,i,j;double angle,cosine,sine,half,focal,sx,sy;float cs,sn;
    for(n=0;n<64;++n) {
        bh_view_config_init(&c,321+(long)n*7,239+(long)n*3);c.fov=(float)(.4+n*.025);c.near_z=.5f;c.far_z=200;
        c.origin_x=.1f;c.origin_y=-3.125f;identity(&m);angle=n*.07;cosine=cos(angle);sine=sin(angle);cs=(float)cosine;sn=(float)sine;
        m.m[0][0]=m.m[2][2]=cs;m.m[0][2]=sn;m.m[2][0]=-sn;m.m[0][3]=3;m.m[1][3]=-5;m.m[2][3]=7;
        CHECK(bh_view_camera_build(&c,&m,&view),"varied translated/rotated camera owner");
        half=(double)c.fov*.5;focal=c.width*.5/tan(half);
        for(i=0;i<2;++i)for(j=0;j<4;++j) {
            double expected=focal*m.m[i][j]+((i?c.origin_y:c.origin_x)+(i?c.height:c.width)*.5)*m.m[2][j];
            CHECK(close_value(view.projected.m[i][j],expected),"projection coefficients match independent pinhole-camera formula");
        }
        for(j=0;j<4;++j)CHECK(view.projected.m[2][j]==m.m[2][j],"projection preserves entire input depth row");
        CHECK(close_value(view.position.x,-((double)cs*3-sn*7)) && view.position.y==5 && close_value(view.position.z,-((double)sn*3+cs*7)),"camera position comes from original rigid inverse rather than copied translation");
        local.x=.1f;local.y=.2f;local.z=3;bh_mat34_transform_point(&view.inverse,&local,&world);bh_mat34_transform_point(&view.projected,&world,&projected);
        sx=c.origin_x+c.width*.5+focal*local.x/local.z;sy=c.origin_y+c.height*.5+focal*local.y/local.z;
        CHECK(close_value(projected.x/projected.z,sx) && close_value(projected.y/projected.z,sy),"world inverse/projected point round-trip matches expected screen coordinates");
        for(i=0;i<6;++i)CHECK(distance(view.planes+i,&world)<0,"rotated translated interior point satisfies all world-space frustum planes");
        local.x=local.y=0;local.z=c.near_z;bh_mat34_transform_point(&view.inverse,&local,&world);CHECK(on_plane(view.planes+BH_VIEW_NEAR,&world),"world near-plane point lies on actual transformed boundary");
        local.z=c.far_z;bh_mat34_transform_point(&view.inverse,&local,&world);CHECK(on_plane(view.planes+BH_VIEW_FAR,&world),"world far-plane point lies on actual transformed boundary");
        for(i=BH_VIEW_TOP;i<=BH_VIEW_BOTTOM;++i)CHECK(close_value((double)view.planes[i].normal.x*view.planes[i].normal.x+(double)view.planes[i].normal.y*view.planes[i].normal.y+(double)view.planes[i].normal.z*view.planes[i].normal.z,1),"original vertical plane normalization uses real math owner");
    }
}
static void failures_and_semantics(void) {
    BHViewConfig c,saved;BHViewCamera out,prior;BHMat34 m;
    bh_view_config_init(&c,64,64);identity(&m);m.m[0][0]=2;m.m[0][3]=3;
    CHECK(bh_view_camera_build(&c,&m,&out) && out.inverse.m[0][0]==2 && out.position.x==-6,"nonrigid input retains original rigid-inverse semantics; no substituted general inverse");
    memset(&out,0x37,sizeof(out));prior=out;c.fov=0;
    CHECK(!bh_view_camera_build(&c,&m,&out) && !memcmp(&out,&prior,sizeof(out)),"invalid FOV guard retains output");
    c.fov=1;c.height=0;CHECK(!bh_view_camera_build(&c,&m,&out) && !memcmp(&out,&prior,sizeof(out)),"zero projection height rejects safely");
    bh_view_config_init(&c,64,64);saved=c;CHECK(!bh_view_reset_viewport(&c,0,64) && !memcmp(&c,&saved,sizeof(c)),"invalid native target dimensions do not publish partial viewport");
    {unsigned long nan=0x7fc00001UL;float invalid;memcpy(&invalid,&nan,4);CHECK(!bh_view_clip_rect(&c,invalid,0,1,1) && !memcmp(&c,&saved,sizeof(c)),"nonfinite rectangle guard preserves configuration");}
}
int bh_view_camera_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;checks=0;failures=0;if(!bh_math_environment_enter(&fp))return 2;
    settings();varied();failures_and_semantics();CHECK(bh_math_environment_supported(),"camera family preserves PC53 caller profile");
    bh_math_environment_leave(&fp);*count=checks;printf("{\"scenario\":\"view-camera\",\"checks\":%lu,\"failures\":%d,\"original_code_required\":false}\n",checks,failures);return failures!=0;
}
