#include "bh_player_camera.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
#include <float.h>

typedef char player_camera_start_size[sizeof(BHPlayerCameraStart)==32 ? 1:-1];
typedef char player_camera_pointer_size[sizeof(void*)==4 ? 1:-1];
typedef char player_camera_position_offset[offsetof(BHEntity,position)==0xc ? 1:-1];
typedef char player_camera_payload_offset[offsetof(BHEntity,payload)==0xe4 ? 1:-1];

static BHVec3 read_vector(const BHEntity *player,unsigned long offset) {
    BHVec3 value;memcpy(&value,(const unsigned char *)player+offset,sizeof(value));return value;
}
static void write_vector(BHEntity *player,unsigned long offset,const BHVec3 *value) {
    memcpy((unsigned char *)player+offset,value,sizeof(*value));
}
static int finite_vector(const BHVec3 *v) {return _finite(v->x) && _finite(v->y) && _finite(v->z);}
static void translate(BHVec3 *v,const BHVec3 *delta,double delta_x) {
    bh_round_float(&v->x,(double)v->x+delta_x);
    bh_round_float(&v->y,(double)v->y+delta->y);
    bh_round_float(&v->z,(double)v->z+delta->z);
}
BHCameraState *__cdecl bh_player_camera_state(BHEntity *player) {
    return player?(BHCameraState *)((unsigned char *)player+0xe8):0;
}
int __cdecl bh_player_camera_reset(BHPlayerCameraContext *context,unsigned long index) {
    BHEntity *player;BHCameraState *camera;BHCameraInput input;
    BHVec3 position,delta,zero={0,0,0},axis;BHMat34 matrix={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    unsigned long i;unsigned short active=1;
    double delta_x;
    if(!context || !context->player || !context->camera || !context->starts || index>=context->start_count)return 0;
    player=context->player;camera=bh_player_camera_state(player);
    position=context->starts[index].position;input.angles=context->starts[index].angles;
    if(!finite_vector(&position) || !finite_vector(&input.angles) || !finite_vector(&player->position) ||
       !finite_vector(&camera->current.position) || !finite_vector(&camera->previous.position) ||
       !finite_vector(&camera->position) || !finite_vector(&camera->current.angles) ||
       !_finite(camera->current.seed_yaw) || camera->current.mode<0 || camera->current.mode>=12 ||
       !_finite(camera->elapsed) || !_finite(camera->blend_remaining) || !_finite(camera->look_yaw) || !_finite(camera->look_pitch))return 0;
    if(context->player_count>1)
        bh_round_float(&position.x,(double)position.x-(-1.0-(double)context->local_player_index*-.25));
    delta_x=(double)position.x-player->position.x;
    bh_round_float(&delta.y,(double)position.y-player->position.y);
    bh_round_float(&delta.z,(double)position.z-player->position.z);
    player->position=position;
    translate(&camera->position,&delta,delta_x);translate(&camera->current.position,&delta,delta_x);translate(&camera->previous.position,&delta,delta_x);
    for(i=0;i<4;++i)write_vector(player,0x48+i*12,&zero);
    write_vector(player,0x3c,&input.angles);
    bh_mat34_rotate_y(input.angles.x,&matrix);bh_mat34_rotate_x(input.angles.y,&matrix);bh_mat34_rotate_z(input.angles.z,&matrix);
    axis.x=1;axis.y=axis.z=0;bh_mat34_transform_direction(&matrix,&axis,&input.basis_x);
    axis.y=1;axis.x=0;bh_mat34_transform_direction(&matrix,&axis,&input.basis_y);
    axis.z=1;axis.y=0;bh_mat34_transform_direction(&matrix,&axis,&input.basis_z);
    write_vector(player,0x18,&input.basis_x);write_vector(player,0x24,&input.basis_y);write_vector(player,0x30,&input.basis_z);
    matrix.m[0][0]=input.basis_x.x;matrix.m[1][0]=input.basis_x.y;matrix.m[2][0]=input.basis_x.z;
    matrix.m[0][1]=input.basis_y.x;matrix.m[1][1]=input.basis_y.y;matrix.m[2][1]=input.basis_y.z;
    matrix.m[0][2]=input.basis_z.x;matrix.m[1][2]=input.basis_z.y;matrix.m[2][2]=input.basis_z.z;
    matrix.m[0][3]=position.x;matrix.m[1][3]=position.y;matrix.m[2][3]=position.z;
    memcpy((unsigned char *)player+0x84,&matrix,sizeof(matrix));
    memcpy((unsigned char *)player+0x2b0,&active,sizeof(active));
    if(!bh_camera_state_set_mode(camera,1))return 0;
    input.target=player->position;input.actor_token=(unsigned long)player;
    for(i=0;i<50;++i)
        if(!bh_camera_state_update(context->camera,camera,&input,0.033333333333333333333,0,0))return 0;
    write_vector(player,0x298,&player->position);write_vector(player,0x2a4,&zero);
    return 1;
}
