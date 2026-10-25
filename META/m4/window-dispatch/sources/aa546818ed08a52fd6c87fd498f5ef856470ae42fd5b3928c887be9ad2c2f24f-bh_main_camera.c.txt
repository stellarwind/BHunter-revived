#include "bh_main_camera.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
#include <string.h>

int __cdecl bh_main_camera_settings(BHViewConfig *c,double degrees,double near_z,double far_z) {
    float fov,n,f;
    if(!c || !_finite(degrees) || !_finite(near_z) || !_finite(far_z))return 0;
    bh_round_float(&fov,degrees*0.017453292519943295);
    bh_round_float(&n,near_z);bh_round_float(&f,far_z);
    if(!_finite(fov) || fov<=0 || fov>=3.141592653589793 || !_finite(n) || !_finite(f) || n<=0 || f<n)return 0;
    c->near_z=n;c->far_z=f;c->fov=fov;return 1;
}
void __cdecl bh_mat34_rotate_world_z(float radians,BHMat34 *m) {
    float cosine,sine;double x,y;unsigned long column;
    bh_round_float(&cosine,cos((double)radians));
    bh_round_float(&sine,sin((double)radians));
    for(column=0;column<4;++column) {
        x=m->m[0][column];y=m->m[1][column];
        bh_round_float(&m->m[0][column],x*cosine-y*sine);
        bh_round_float(&m->m[1][column],x*sine+y*cosine);
    }
}
static void main_plane(BHViewPlane *out,const BHMainCamera *camera,float x,float y,float z,int normalize) {
    BHVec3 local;local.x=x;local.y=y;local.z=z;
    bh_mat34_transform_direction(&camera->inverse,&local,&out->normal);
    if(normalize)bh_vec3_normalize(&out->normal);
    bh_round_float(&out->offset,((double)out->normal.x*camera->position.x+
        (double)out->normal.y*camera->position.y)+(double)out->normal.z*camera->position.z);
}
int __cdecl bh_main_camera_build(const BHViewConfig *c,const BHMat34 *camera,BHMainCamera *out) {
    BHMainCamera next;BHVec3 zero;unsigned long i,j;float half,ox,oy,cosine,sine,vertical;
    double focal,retained_x;const BHMat34 *inverse_source;
    if(!c || !camera || !out || !_finite(c->fov) || c->fov<=0 || c->fov>=3.141592653589793 ||
        !_finite(c->width) || !_finite(c->height) || c->width<=0 || c->height<=0 ||
        !_finite(c->origin_x) || !_finite(c->origin_y))return 0;
    for(i=0;i<12;++i)if(!_finite(((const float *)camera)[i]))return 0;
    next=*out;next.projected=*camera;next.projection_state=0;
    bh_round_float(&half,(double)c->fov*.5);
    focal=((double)c->width*.5)/tan((double)c->fov*.5);
    retained_x=(double)c->origin_x-(double)c->width*-.5;
    bh_round_float(&ox,retained_x);bh_round_float(&oy,(double)c->origin_y-(double)c->height*-.5);
    bh_round_float(&next.focal,focal);bh_round_float(&cosine,cos((double)half));
    bh_round_float(&sine,-sin((double)half));
    for(j=0;j<4;++j) {
        bh_round_float(&next.projected.m[0][j],focal*camera->m[0][j]+(j?ox:retained_x)*camera->m[2][j]);
        bh_round_float(&next.projected.m[1][j],focal*camera->m[1][j]+(double)oy*camera->m[2][j]);
    }
    bh_round_float(&vertical,((double)cosine*c->width)/c->height);
    inverse_source=camera==&out->projected?&next.projected:camera;
    bh_mat34_inverse_rigid(inverse_source,&next.inverse);
    zero.x=zero.y=zero.z=0;bh_mat34_transform_point(&next.inverse,&zero,&next.position);
    main_plane(next.planes,&next,0,0,1,0);
    main_plane(next.planes+2,&next,cosine,0,sine,0);
    main_plane(next.planes+3,&next,-cosine,0,sine,0);
    main_plane(next.planes+4,&next,0,-vertical,sine,1);
    main_plane(next.planes+5,&next,0,vertical,sine,1);
    for(i=0;i<12;++i)if(!_finite(((const float *)&next.projected)[i]))return 0;
    for(i=0;i<6;++i)if(i!=1 && (!_finite(next.planes[i].normal.x) || !_finite(next.planes[i].normal.y) || !_finite(next.planes[i].normal.z) || !_finite(next.planes[i].offset)))return 0;
    *out=next;return 1;
}
int __cdecl bh_main_camera_pose(const BHViewConfig *c,BHMainCamera *out,
    double x,double y,double z,double yaw,double pitch,double roll) {
    BHMainCamera next;float tx,ty,tz,ry,rx,rz;
    if(!out || !_finite(x) || !_finite(y) || !_finite(z) || !_finite(yaw) || !_finite(pitch) || !_finite(roll))return 0;
    next=*out;memset(&next.view,0,sizeof(next.view));
    next.view.m[0][0]=next.view.m[1][1]=next.view.m[2][2]=1;
    bh_round_float(&next.position.x,x);bh_round_float(&next.position.y,y);bh_round_float(&next.position.z,z);
    bh_round_float(&next.angles.x,yaw);bh_round_float(&next.angles.y,pitch);bh_round_float(&next.angles.z,roll);
    bh_round_float(&tx,-x);bh_round_float(&ty,-y);bh_round_float(&tz,-z);
    bh_round_float(&ry,-yaw);bh_round_float(&rx,-pitch);bh_round_float(&rz,-roll);
    bh_mat34_translate_world(tx,ty,tz,&next.view);
    bh_mat34_rotate_world_y(ry,&next.view);bh_mat34_rotate_world_x(rx,&next.view);bh_mat34_rotate_world_z(rz,&next.view);
    if(!bh_main_camera_build(c,&next.view,&next))return 0;
    *out=next;return 1;
}
