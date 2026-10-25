#include "bh_scene_owner.h"
#include "bh_draw_controls.h"
#include "bh_rotation.h"
#include "bh_main_camera.h"
#include "bh_route_query.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>

/* Original .rdata at 00489440: 39 9d 52 a2 46 df 91 3f (pi / 180),
   and 00489288: 00 00 00 3f (binary32 one half). */
static const double degrees_to_radians=0.017453292519943295;
static const float one_half=0.5f;

int __cdecl bh_scene_owner_init(BHSceneOwner *owner,BHSceneBindings *bindings,
    long target_width,long target_height) {
    if(!owner || !bindings || target_width<=0 || target_height<=0)return 0;
    memset(owner,0,sizeof(*owner));owner->bindings=bindings;
    owner->target_width=target_width;owner->target_height=target_height;
    return 1;
}

static int inside(const BHWorldGeometry *world,const BHVec3 *point) {
    return point->x>=world->minimum.x && point->x<=world->maximum.x &&
           point->y>=world->minimum.y && point->y<=world->maximum.y &&
           point->z>=world->minimum.z && point->z<=world->maximum.z;
}

int __cdecl bh_scene_owner_step(BHSceneOwner *owner,const BHVec3 *eye,
    const BHVec3 *angles,float fov_degrees,float near_z,float far_z,
    long camera_width,long camera_height) {
    BHSceneBindings *bindings;BHEntityScheduleContext *schedule;
    BHEntityScheduleRoom *scheduled;BHWorldGeometry *hint,*room;
    BHMat34 view,projected;BHVec3 reference,relative;float focal,half_width,half_height;
    unsigned long row,column;double tangent;
    if(!owner || !owner->bindings)return 0;
    bindings=owner->bindings;
    if(!bindings->first || owner->mode==1) {
        bindings->cached=0;owner->error=BH_SCENE_OWNER_INACTIVE;return 0;
    }
    schedule=bindings->schedule_context;
    if(!eye || !angles || !schedule || !schedule->queue || !schedule->viewport ||
       !bindings->scene_count || owner->target_width<=0 || owner->target_height<=0 ||
       camera_width<=0 || camera_height<=0 || !_finite(eye->x) || !_finite(eye->y) ||
       !_finite(eye->z) || !_finite(angles->x) || !_finite(angles->y) ||
       !_finite(angles->z) || !_finite(fov_degrees) || !_finite(near_z) ||
       !_finite(far_z) || near_z<=0 || far_z<near_z) {
        owner->error=BH_SCENE_OWNER_ARGUMENT;return 0;
    }
    tangent=tan((double)fov_degrees*degrees_to_radians*(double)one_half);
    if(!_finite(tangent) || tangent==0) {
        owner->error=BH_SCENE_OWNER_ARGUMENT;return 0;
    }
    memset(&view,0,sizeof(view));view.m[0][0]=view.m[1][1]=view.m[2][2]=1;
    bh_mat34_translate_world(-eye->x,-eye->y,-eye->z,&view);
    bh_mat34_rotate_world_y(-angles->x,&view);
    bh_mat34_rotate_world_x(-angles->y,&view);
    bh_mat34_rotate_world_z(-angles->z,&view);
    projected=view;
    bh_round_float(&focal,(double)(camera_width/2)/tangent);
    half_width=(float)(camera_width/2);half_height=(float)(camera_height/2);
    for(row=0;row<2;++row)for(column=0;column<4;++column) {
        double half=row==0?half_width:half_height;
        bh_round_float(&projected.m[row][column],
            (double)focal*(double)view.m[row][column]+half*(double)view.m[2][column]);
    }
    reference.x=reference.y=0;reference.z=1;
    bh_mat34_inverse_transform_point(&projected,&reference,&relative);
    relative.x-=eye->x;relative.y-=eye->y;relative.z-=eye->z;
    hint=bindings->cached?bindings->cached:bindings->first;
    room=bindings->cached;
    if(room && !inside(room,eye))room=0;
    if(!room) {
        room=bh_world_locate(bindings->first,bindings->scene_count,hint,eye,0);
        if(!room)room=bh_world_locate(bindings->first,bindings->scene_count,hint,eye,1);
        if(!room)room=hint;
    }
    scheduled=bh_scene_bindings_schedule(bindings,room);
    if(!scheduled){owner->error=BH_SCENE_OWNER_BINDING;return 0;}
    owner->eye=*eye;owner->angles=*angles;owner->view=view;
    owner->camera_width=camera_width;owner->camera_height=camera_height;
    owner->camera.projected=projected;owner->camera.position=*eye;
    owner->relative_camera=relative;owner->focal=focal;
    owner->near_z=near_z;owner->far_z=far_z;
    bh_round_float(&owner->eye_projection_dot,
        ((double)relative.x*eye->x+(double)relative.y*eye->y)+(double)relative.z*eye->z);
    bindings->cached=room;
    ++owner->frame;schedule->frame=owner->frame;
    schedule->viewport->near_z=near_z;schedule->viewport->far_z=far_z;
    schedule->viewport->left=schedule->viewport->top=0;
    schedule->viewport->right=(float)camera_width;
    schedule->viewport->bottom=(float)camera_height;
    schedule->camera=&owner->camera;
    if(!bh_entity_schedule(schedule,scheduled)) {
        owner->error=BH_SCENE_OWNER_SCHEDULE;return 0;
    }
    {
    int first=bh_draw_controls_clip_rect(schedule->queue,0,0,0,
            (float)owner->target_width,(float)owner->target_height);
    int second=bh_draw_controls_clip_rect(schedule->queue,3,0,0,
            (float)owner->target_width,(float)owner->target_height);
    if(first!=1 || second!=1) {
        owner->error=BH_SCENE_OWNER_QUEUE;return 0;
    }
    }
    owner->error=BH_SCENE_OWNER_OK;return 1;
}
