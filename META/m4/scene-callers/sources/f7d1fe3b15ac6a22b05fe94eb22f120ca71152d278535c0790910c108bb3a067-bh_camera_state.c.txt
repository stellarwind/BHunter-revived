#include "bh_camera_state.h"
#include "bh_round.h"
#include "bh_rotation.h"
#include "bh_camera_modes.h"
#include <stddef.h>
#include <string.h>
#include <float.h>
#include <math.h>

typedef char camera_mode_size[sizeof(BHCameraModeState)==0x30 ? 1:-1];
typedef char camera_state_size[sizeof(BHCameraState)==0x9c ? 1:-1];
typedef char camera_blend_offset[offsetof(BHCameraState,blend_remaining)==0x68 ? 1:-1];
typedef char camera_elapsed_offset[offsetof(BHCameraState,elapsed)==0x70 ? 1:-1];
typedef char camera_position_offset[offsetof(BHCameraState,position)==0x78 ? 1:-1];
typedef char camera_angles_offset[offsetof(BHCameraState,angles)==0x84 ? 1:-1];
typedef char camera_velocity_offset[offsetof(BHCameraState,velocity)==0x90 ? 1:-1];

int __cdecl bh_camera_state_init(BHCameraState *state,long mode) {
    if(!state)return 0;
    state->elapsed=0;state->blend_remaining=0;
    state->look_pitch=0;state->look_yaw=0;
    state->current.mode=mode;state->current.first_update=1;
    state->velocity.x=state->velocity.y=state->velocity.z=0;
    return 1;
}

int __cdecl bh_camera_state_set_mode(BHCameraState *state,long mode) {
    double yaw;
    const double pi=3.14159265358979323846;
    if(!state)return 0;
    if(state->current.mode==mode)return 1;
    if((mode==8 || mode==11) && !_finite(state->angles.x))return 0;
    state->blend_remaining=0;
    memcpy(&state->previous,&state->current,sizeof(state->current));
    state->blend_remaining=.5;
    state->current.mode=mode;state->current.first_update=1;
    if(mode==8 || mode==11) {
        yaw=(double)state->angles.x-(-pi);
        if(yaw>=pi)yaw=fmod(yaw-(-pi),pi+pi)-pi;
        else if(yaw< -pi)yaw=fmod(yaw-pi,pi+pi)-(-pi);
        bh_round_float(&state->current.seed_yaw,yaw);
        state->elapsed=0;
    }
    return 1;
}

int __cdecl bh_camera_state_set_correction(BHCameraStateContext *context,
    const BHCameraState *ignored_state,BHCameraCorrection correction) {
    (void)ignored_state;
    if(!context)return 0;
    context->correction=correction;
    context->contextual_correction=0;context->correction_user=0;return 1;
}
int __cdecl bh_camera_state_bind_correction(BHCameraStateContext *context,
    BHCameraCorrectionContextual correction,void *user) {
    if(!context)return 0;
    context->correction=0;context->contextual_correction=correction;
    context->correction_user=user;return 1;
}
static void correct(BHCameraStateContext *context,BHVec3 *eye,
    const BHVec3 *target,unsigned long actor_token) {
    if(context->contextual_correction)
        context->contextual_correction(eye,target,actor_token,context->correction_user);
    else if(context->correction)context->correction(eye,target,actor_token);
}

static double wrap_stored(double value) {
    const double pi=3.14159265358979323846;
    float stored;
    bh_round_float(&stored,value);
    if(value>=pi)return fmod((double)stored+pi,pi+pi)-pi;
    if((double)stored< -pi)return fmod((double)stored-pi,pi+pi)+pi;
    return stored;
}

void __cdecl bh_camera_mode_smooth(BHCameraStateContext *context,
    BHCameraModeState *mode,BHVec3 *position,const BHVec3 *angles,
    double position_retention,double angle_retention) {
    float old_x,old_y,old_z,delta;
    double weight;
    correct(context,position,&context->target,context->actor_token);
    if(mode->first_update) {
        mode->first_update=0;mode->position=*position;mode->angles=*angles;
        return;
    }
    bh_round_float(&old_x,(double)mode->position.x*position_retention);
    bh_round_float(&old_y,(double)mode->position.y*position_retention);
    bh_round_float(&old_z,(double)mode->position.z*position_retention);
    weight=1-position_retention;
    bh_round_float(&mode->position.x,(double)position->x*weight+old_x);
    bh_round_float(&mode->position.y,(double)position->y*weight+old_y);
    bh_round_float(&mode->position.z,(double)position->z*weight+old_z);
    weight=1-angle_retention;
    bh_round_float(&delta,(double)angles->x-mode->angles.x);
    bh_round_float(&mode->angles.x,wrap_stored(wrap_stored(delta)*weight+mode->angles.x));
    bh_round_float(&mode->angles.y,wrap_stored(wrap_stored((double)angles->y-mode->angles.y)*weight+mode->angles.y));
    bh_round_float(&mode->angles.z,wrap_stored(wrap_stored((double)angles->z-mode->angles.z)*weight+mode->angles.z));
}

static int vector_finite(const BHVec3 *v) {
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int mode_finite(const BHCameraModeState *mode) {
    return mode->mode>=0 && mode->mode<12 && _finite(mode->seed_yaw) &&
        vector_finite(&mode->position) && vector_finite(&mode->angles);
}

static void look_rotation(BHCameraState *state) {
    BHMat34 rotation={{{1,0,0,0},{0,1,0,0},{0,0,1,0}}};
    BHVec3 axis={0,0,1},out;
    float angle;
    const double scale=1.04719755119659774615;
    bh_mat34_rotate_y(state->angles.x,&rotation);
    bh_mat34_rotate_x(state->angles.y,&rotation);
    bh_mat34_rotate_z(state->angles.z,&rotation);
    bh_round_float(&angle,(double)state->look_yaw*scale);
    bh_mat34_rotate_y(angle,&rotation);
    bh_round_float(&angle,(double)state->look_pitch*scale);
    bh_mat34_rotate_x(angle,&rotation);
    bh_mat34_transform_point(&rotation,&axis,&out);
    if(out.x!=0 || out.z!=0)bh_round_float(&state->angles.x,atan2(out.x,out.z));
    bh_mat34_rotate_world_y(-state->angles.x,&rotation);
    bh_mat34_transform_point(&rotation,&axis,&out);
    bh_round_float(&state->angles.y,atan2(-(double)out.y,out.z));
    bh_mat34_rotate_world_x(-state->angles.y,&rotation);
    axis.x=1;axis.z=0;
    bh_mat34_transform_point(&rotation,&axis,&out);
    bh_round_float(&state->angles.z,atan2(out.y,out.x));
}

int __cdecl bh_camera_state_update(BHCameraStateContext *context,
    BHCameraState *state,const BHCameraInput *input,double dt,
    float look_yaw,float look_pitch) {
    BHVec3 before;
    float old_weight,new_weight,delta;
    double amount,inverse_dt,remaining;
    if(!context || !state || !input || !_finite(dt) || dt<=0 ||
       !_finite(look_yaw) || !_finite(look_pitch) ||
       !vector_finite(&input->target) || !vector_finite(&input->angles) ||
       !vector_finite(&input->basis_x) || !vector_finite(&input->basis_y) ||
       !vector_finite(&input->basis_z) || !vector_finite(&state->position) ||
       !mode_finite(&state->current) || !_finite(state->blend_remaining) ||
       !_finite(state->elapsed) || !_finite(state->elapsed+dt) ||
       !_finite(state->look_yaw) || !_finite(state->look_pitch))return 0;
    remaining=state->blend_remaining-dt;
    if(remaining>0 && !mode_finite(&state->previous))return 0;
    state->elapsed=dt+state->elapsed;context->elapsed=state->elapsed;
    before=state->position;
    context->actor_token=input->actor_token;context->target=input->target;
    if(!bh_camera_mode_update(context,&state->current,input))return 0;
    state->blend_remaining=remaining;
    if(remaining<0)state->blend_remaining=0;
    if(state->blend_remaining<=0) {
        state->position=state->current.position;state->angles=state->current.angles;
    } else {
        bh_round_float(&old_weight,state->blend_remaining+state->blend_remaining);
        bh_round_float(&new_weight,1-(state->blend_remaining+state->blend_remaining));
        if(!bh_camera_mode_update(context,&state->previous,input))return 0;
        correct(context,&state->previous.position,&input->target,input->actor_token);
        bh_round_float(&state->position.x,(double)state->previous.position.x*old_weight+(double)state->current.position.x*new_weight);
        bh_round_float(&state->position.y,(double)state->previous.position.y*old_weight+(double)state->current.position.y*new_weight);
        bh_round_float(&state->position.z,(double)state->previous.position.z*old_weight+(double)state->current.position.z*new_weight);
        bh_round_float(&state->angles.x,wrap_stored(wrap_stored((double)state->current.angles.x-state->previous.angles.x)*new_weight+state->previous.angles.x));
        bh_round_float(&state->angles.y,wrap_stored(wrap_stored((double)state->current.angles.y-state->previous.angles.y)*new_weight+state->previous.angles.y));
        bh_round_float(&state->angles.z,wrap_stored(wrap_stored((double)state->current.angles.z-state->previous.angles.z)*new_weight+state->previous.angles.z));
        correct(context,&state->position,&input->target,input->actor_token);
    }
    amount=((double)look_yaw-state->look_yaw)*.25;
    bh_round_float(&delta,amount);
    if(amount<dt*-8)amount=dt*-8;
    else if(delta>dt*8)amount=dt*8;
    else amount=delta;
    bh_round_float(&state->look_yaw,amount+state->look_yaw);
    amount=((double)look_pitch-state->look_pitch)*.25;
    bh_round_float(&delta,amount);
    if(amount<dt*-8)bh_round_float(&delta,dt*-8);
    else if(delta>dt*8)bh_round_float(&delta,dt*8);
    bh_round_float(&state->look_pitch,(double)delta+state->look_pitch);
    if(state->look_yaw!=0 || state->look_pitch!=0)look_rotation(state);
    inverse_dt=1/dt;
    bh_round_float(&state->velocity.x,((double)state->position.x-before.x)*inverse_dt);
    bh_round_float(&state->velocity.y,((double)state->position.y-before.y)*inverse_dt);
    bh_round_float(&state->velocity.z,((double)state->position.z-before.z)*inverse_dt);
    return 1;
}
