#include "bh_world_geometry.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <float.h>

static const BHMat34 identity={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
static void reset_bounds(BHVec3 *minimum,BHVec3 *maximum) {
    unsigned long lo=0x7e876abdUL,hi=0xfe876abdUL;
    memcpy(&minimum->x,&lo,4);minimum->y=minimum->z=minimum->x;
    memcpy(&maximum->x,&hi,4);maximum->y=maximum->z=maximum->x;
}
static void include_point(BHVec3 *minimum,BHVec3 *maximum,const BHVec3 *p) {
    if(p->x<minimum->x)minimum->x=p->x;if(maximum->x<p->x)maximum->x=p->x;
    if(p->y<minimum->y)minimum->y=p->y;if(maximum->y<p->y)maximum->y=p->y;
    if(p->z<minimum->z)minimum->z=p->z;if(maximum->z<p->z)maximum->z=p->z;
}
static void __cdecl model_bounds(BHFaceInfo *face,void *user) {
    BHGeometryModel *m=(BHGeometryModel*)user;BHVec3 p;unsigned long i;
    for(i=0;i<face->vertices;++i) {
        if(!bh_geometry_vertex(face->geometry,face->face,i,&p)){m->error=face->geometry->error;return;}
        include_point(&m->minimum,&m->maximum,&p);
    }
}
static unsigned long material_flag(const char *name) {
    if(!_stricmp(name,"PORTAL.GIF"))return 1;
    if(!_stricmp(name,"ROOF.GIF"))return 2;
    if(!_stricmp(name,"WATER.GIF"))return 4;
    if(!_stricmp(name,"SCRAPR2.TGA") || !_stricmp(name,"SCRAPR.TGA"))return 8;
    return 0;
}
static void __cdecl classify(BHFaceInfo *face,void *user) {
    BHGeometryModel *m=(BHGeometryModel*)user;unsigned long flag=material_flag(face->texture),old;
    if(flag==1)m->portal_faces[m->portal_count++]=face->face;
    if(flag==4)m->flags|=0x40;
    if(flag) {
        if(!bh_geometry_get_flags(face->geometry,face->face,&old) ||
           !bh_geometry_set_flags(face->geometry,face->face,old|flag))m->error=face->geometry->error;
    }
}
int __cdecl bh_geometry_model_init(BHGeometryModel *m,BHGeometry *g,long grid,unsigned long flags) {
    BHGeometryModel next;const BHBsp *b;unsigned long i,portals=0;
    if(!m || !g || !g->bsp || !g->face_flags || g->enumerating || grid<=0)return 0;
    b=g->bsp;
    for(i=0;i<b->counts[3];++i)if(material_flag(b->materials[b->faces[i].material].texture)==1 && ++portals>6)return 0;
    memset(&next,0,sizeof(next));next.geometry=g;next.grid=grid;next.flags=flags;
    reset_bounds(&next.minimum,&next.maximum);
    if(!bh_geometry_each_face(g,model_bounds,&next) || next.error)return 0;
    if(!bh_geometry_each_face(g,classify,&next) || next.error)return 0;
    *m=next;return 1;
}
static int valid_scene(BHWorldGeometry *p,unsigned long count) {
    BHWorldGeometry *at=p;unsigned long i;
    if(!p || !count || count>65535)return 0;
    for(i=0;i<count;++i) {
        if(!at || (i && at==p) || !at->previous ||
           at->previous->navigation.next!=&at->navigation || !at->model ||
           !at->model->geometry || !at->model->geometry->bsp || at->model->portal_count>6 ||
           at->model->grid<=0 || !_finite(at->position.x) || !_finite(at->position.y) || !_finite(at->position.z))return 0;
        at=(BHWorldGeometry*)at->navigation.next;
    }
    return at==p;
}
static int plane_coordinate(float coordinate,long grid,long *out) {
    double value=(double)coordinate/grid;
    long rounded;
    value-=coordinate>0 ? -0.5:0.5;
    if(value<=-2147483649.0 || value>=2147483648.0)return 0;
    rounded=(long)value;
    value=(double)rounded*grid;
    if(value<-2147483648.0 || value>2147483647.0)return 0;
    *out=(long)value;return 1;
}
typedef struct WorldBoundsWork {BHWorldGeometry *placement;int failed;} WorldBoundsWork;
static void __cdecl world_bounds(BHFaceInfo *face,void *user) {
    WorldBoundsWork *work=(WorldBoundsWork*)user;BHWorldGeometry *p=work->placement;BHVec3 v,out;unsigned long i;
    for(i=0;i<face->vertices;++i) {
        if(!bh_geometry_vertex(face->geometry,face->face,i,&v)){work->failed=1;return;}
        bh_mat34_transform_point(&p->transform,&v,&out);
        if(!_finite(out.x) || !_finite(out.y) || !_finite(out.z)){work->failed=1;return;}
        include_point(&p->minimum,&p->maximum,&out);
    }
}
int __cdecl bh_world_geometry_build(BHWorldGeometry *p,unsigned long ring_count) {
    BHWorldGeometry next,*previous;const BHGeometryModel *m;BHFaceInfo face;
    BHVec3 normal,origin,point,vertex,zero={0,0,0};BHPortal *portal;
    unsigned long i,j,axis,u,v;float angle;
    WorldBoundsWork bounds;
    if(!valid_scene(p,ring_count) || !bh_math_environment_supported())return 0;
    next=*p;m=p->model;next.owner=NULL;next.transform=next.inverse=identity;
    bh_mat34_translate_local(p->position.x,p->position.y,p->position.z,&next.transform);
    bh_round_float(&angle,(double)(p->flags&3UL)*1.5707963267948966);
    bh_mat34_rotate_y(angle,&next.transform);bh_mat34_rotate_y(-angle,&next.inverse);
    bh_mat34_translate_local(-p->position.x,-p->position.y,-p->position.z,&next.inverse);
    if(!m->portal_count) {
        previous=p->previous;
        while(previous!=p && !previous->model->portal_count)previous=previous->previous;
        if(previous->model->portal_count)next.owner=previous;
    }
    next.navigation.count=m->portal_count;memset(&face,0,sizeof(face));
    for(i=0;i<m->portal_count;++i) {
        if(!bh_geometry_face(m->geometry,m->portal_faces[i],&face))return 0;
        bh_mat34_transform_point(&next.transform,&face.normal,&normal);
        bh_mat34_transform_point(&next.transform,&zero,&origin);
        bh_round_float(&normal.x,(double)normal.x-origin.x);
        bh_round_float(&normal.y,(double)normal.y-origin.y);
        bh_round_float(&normal.z,(double)normal.z-origin.z);
        bh_mat34_transform_point(&next.transform,&face.point,&point);
        portal=&next.navigation.portals[i];memset(portal,0,sizeof(*portal));
        axis=0;u=1;v=2;
        if(normal.x>0.9)portal->direction=0;
        else if(normal.x< -0.9)portal->direction=4;
        else {axis=1;u=0;if(normal.y>0.9)portal->direction=1;
              else if(normal.y< -0.9)portal->direction=5;
              else {axis=2;v=1;portal->direction=normal.z>0.9 ? 2:6;}}
        if(!plane_coordinate(((float*)&point)[axis],m->grid,&portal->plane))return 0;
        {unsigned long lo=0x7e876abdUL,hi=0xfe876abdUL;
         memcpy(portal->aperture,&lo,4);memcpy(portal->aperture+1,&hi,4);
         portal->aperture[2]=portal->aperture[0];portal->aperture[3]=portal->aperture[1];}
        for(j=0;j<face.vertices;++j) {
            if(!bh_geometry_vertex(m->geometry,face.face,j,&vertex))return 0;
            bh_mat34_transform_point(&next.transform,&vertex,&point);
            if(((float*)&point)[u]<portal->aperture[0])portal->aperture[0]=((float*)&point)[u];
            if(portal->aperture[1]<((float*)&point)[u])portal->aperture[1]=((float*)&point)[u];
            if(((float*)&point)[v]<portal->aperture[2])portal->aperture[2]=((float*)&point)[v];
            if(portal->aperture[3]<((float*)&point)[v])portal->aperture[3]=((float*)&point)[v];
        }
        memcpy(portal->overlap,portal->aperture,16);memcpy(portal->scratch,portal->aperture,16);
    }
    reset_bounds(&next.minimum,&next.maximum);
    bounds.placement=&next;bounds.failed=0;
    if(!bh_geometry_each_face(m->geometry,world_bounds,&bounds) || bounds.failed)return 0;
    if(!_finite(next.minimum.x) || !_finite(next.minimum.y) || !_finite(next.minimum.z) ||
       !_finite(next.maximum.x) || !_finite(next.maximum.y) || !_finite(next.maximum.z))return 0;
    *p=next;return 1;
}
