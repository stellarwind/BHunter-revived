#include "bh_player_scene.h"
#include "bh_camera_queue.h"
#include "bh_horizon_queue.h"
#include "bh_material_queue.h"
#include "bh_quad_queue.h"
#include "bh_screen_quad.h"
#include "bh_draw_submit.h"
#include "bh_queue_reserve.h"
#include "bh_round.h"
#include "bh_pickup.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

typedef char scene_player_pointer_size[sizeof(void *)==4?1:-1];
typedef char scene_player_payload_offset[offsetof(BHEntity,payload)==0xe4?1:-1];
typedef char scene_random_range[RAND_MAX==32767?1:-1];

static BHVec3 vector_at(const BHEntity *p,unsigned long offset) {
    BHVec3 v;memcpy(&v,(const unsigned char *)p+offset,12);return v;
}
static float float_at(const BHEntity *p,unsigned long offset) {
    float v;memcpy(&v,(const unsigned char *)p+offset,4);return v;
}
static long long_at(const BHEntity *p,unsigned long offset) {
    long v;memcpy(&v,(const unsigned char *)p+offset,4);return v;
}
static unsigned long word_at(const BHEntity *p,unsigned long offset) {
    unsigned long v;memcpy(&v,(const unsigned char *)p+offset,4);return v;
}
static unsigned char byte_at(const BHEntity *p,unsigned long offset) {
    return *((const unsigned char *)p+offset);
}
static void put_float(BHEntity *p,unsigned long offset,float value) {
    memcpy((unsigned char *)p+offset,&value,4);
}
static void put_byte(BHEntity *p,unsigned long offset,unsigned char value) {
    *((unsigned char *)p+offset)=value;
}
static float stored(double n){float f;bh_round_float(&f,n);return f;}
static unsigned long bits(float value){unsigned long u;memcpy(&u,&value,4);return u;}
static int random_value(BHPlayerScene *s){++s->random_calls;return rand();}
static void queued(BHPlayerScene *s,int result) {
    ++s->attempted;if(result>0)++s->queued;
    else if(result==0)++s->dropped;
    else if(s->error==BH_PLAYER_SCENE_OK)s->error=BH_PLAYER_SCENE_QUEUE;
}
static int fail(BHPlayerScene *s,BHPlayerSceneError error) {
    if(s)s->error=error;return 0;
}
static int finite_vec(const BHVec3 *v) {
    return _finite(v->x)&&_finite(v->y)&&_finite(v->z);
}
static BHVec3 combined(const BHVec3 *position,const BHVec3 *right,
    const BHVec3 *up,const BHVec3 *forward,float x,float y,float z) {
    BHVec3 v;
    v.x=stored((((double)up->x*y+(double)forward->x*z)+(double)right->x*x)+position->x);
    v.y=stored((((double)up->y*y+(double)forward->y*z)+(double)right->y*x)+position->y);
    v.z=stored((((double)up->z*y+(double)forward->z*z)+(double)right->z*x)+position->z);
    return v;
}
static BHVec3 direction(const BHVec3 *right,const BHVec3 *up,const BHVec3 *forward,
    const BHVec3 *local) {
    BHVec3 v;
    v.x=stored(((double)forward->x*local->z+(double)up->x*local->y)+(double)right->x*local->x);
    v.y=stored(((double)up->y*local->y+(double)forward->y*local->z)+(double)right->y*local->x);
    v.z=stored(((double)right->z*local->x+(double)up->z*local->y)+(double)forward->z*local->z);
    return v;
}
static void aim(BHPlayerScene *s) {
    BHEntity *p=s->player;BHVec3 pos=vector_at(p,0xc),right=vector_at(p,0x18);
    BHVec3 up=vector_at(p,0x24),forward=vector_at(p,0x30);
    BHVec3 aim_offset=vector_at(p,0x28c),aim_vector=vector_at(p,0x280);
    BHVec3 center,axis,normal;float gauge,velocity,alpha,span,shift;
    unsigned long i;
    normal=direction(&right,&up,&forward,&aim_offset);
    normal.x=-normal.x;normal.y=-normal.y;normal.z=-normal.z;
    if(long_at(p,0xe8)==0) {
        axis.x=-up.x;axis.y=-up.y;axis.z=-up.z;
        center=combined(&pos,&right,&up,&forward,
            stored((double)aim_offset.x*1.1),
            stored(-.30000001192092896-(double)aim_offset.y*-1.1),
            stored((double)aim_offset.z*1.1));
        queued(s,bh_quad_queue(s->queue,3,s->aim_image,&center,&axis,&normal,
            .25f,.25f,0,0,.5f,.00390625f,.9921875f,.00390625f,.9921875f));
        gauge=float_at(p,0x23c);if(!(gauge>=0))gauge=0;else if(gauge>1)gauge=1;
        velocity=stored(((double)vector_at(p,0x48).x*forward.x+
            (double)vector_at(p,0x48).y*forward.y+
            (double)vector_at(p,0x48).z*forward.z)*.006666666666666667);
        if(!(velocity>=0))velocity=0;else if(velocity>1)velocity=1;
        if(gauge>0 || velocity>0) {
            shift=stored((-.3-(double)(velocity-gauge)*.05275)-
                (double)aim_offset.y*-1.1);
            center=combined(&pos,&right,&up,&forward,
                stored((double)aim_offset.x*1.1),shift,
                stored((double)aim_offset.z*1.1));
            alpha=stored((velocity+gauge)*.1055);
            span=stored(.5-(double)velocity*.422);
            queued(s,bh_quad_queue(s->queue,3,s->aim_overlay_image,&center,&axis,&normal,
                .25f,alpha,0,0,.5f,.00390625f,.9921875f,span,
                stored((velocity+gauge)*.422)));
        }
    } else {
        axis.x=-up.x;axis.y=-up.y;axis.z=-up.z;
        center=combined(&pos,&right,&up,&forward,0,-.30000001192092896f,0);
        center.x=stored((double)center.x-(double)aim_vector.x*-10.0);
        center.y=stored((double)center.y-(double)aim_vector.y*-10.0);
        center.z=stored((double)center.z-(double)aim_vector.z*-10.0);
        for(i=0;i<3;++i){
            if(i){float step=i==1?-10.0f:-20.0f;
                center.x=stored((double)center.x-(double)aim_vector.x*step);
                center.y=stored((double)center.y-(double)aim_vector.y*step);
                center.z=stored((double)center.z-(double)aim_vector.z*step);
            }
            queued(s,bh_quad_queue(s->queue,3,s->aim_image,&center,&axis,&normal,
                3,3,0,0,.6f,.00390625f,.9921875f,.00390625f,.9921875f));
        }
    }
}

static void status_state(BHPlayerScene *s,BHHudStatusState *state,
    const BHHudWeaponAmmo *const *ammunition) {
    BHEntity *p=s->player;
    state->gauge_a=float_at(p,0x24c);state->gauge_b=float_at(p,0x23c);
    state->weapons=ammunition;state->weapon_count=10;
    state->primary=byte_at(p,0x2b6);state->secondary=byte_at(p,0x2b7);
    state->flags=byte_at(p,0x2b8);state->countdown_a=byte_at(p,0x2b9);
    state->countdown_b=byte_at(p,0x2ba);
    state->counter_a=long_at(p,0x230);state->counter_b=long_at(p,0x234);
    state->pending=long_at(p,0x238);
}
static void status_publish(BHPlayerScene *s,const BHHudStatusState *state) {
    put_byte(s->player,0x2b9,state->countdown_a);
    put_byte(s->player,0x2ba,state->countdown_b);
}

static void particles(BHPlayerScene *s) {
    BHEntity *p=s->player;float effect=float_at(p,0x240),remaining=float_at(p,0x244);
    float size,alpha,fade;BHVec3 pos,offset,up={0,-1,0},normal;
    unsigned long i;
    if(!(effect>0))return;
    size=stored(3.0-sin((double)effect*3.0)*-.5);
    if(!(remaining>0))fade=effect<2?stored((double)effect*.5):1;
    else fade=stored(1.0-(double)remaining);
    alpha=stored((double)fade*.3);
    pos=vector_at(p,0xc);
    for(i=0;i<32;++i){
        offset.x=(float)(random_value(s)-0x4000);
        offset.y=(float)(random_value(s)-0x4000);
        offset.z=(float)(random_value(s)-0x4000);
        bh_vec3_normalize(&offset);
        offset.x=stored((double)offset.x*size);
        offset.y=stored((double)offset.y*size);
        offset.z=stored((double)offset.z*size);
        normal=offset;
        offset.x=stored((double)pos.x+offset.x);
        offset.y=stored((double)pos.y+offset.y);
        offset.z=stored((double)pos.z+offset.z);
        queued(s,bh_draw_oriented(s->queue,3,s->particle_image,&offset,&up,
            &normal,size,size,stored((double)random_value(s)*.0001953125),8,alpha));
    }
}

static void trail(BHPlayerScene *s) {
    unsigned long i;BHEntity *p=s->player;
    for(i=0;i<4;++i){
        BHVec3 motion=vector_at(p,0x2c0+i*12);
        BHVec3 from=vector_at(p,0x2fc+i*24);
        BHVec3 to=vector_at(p,0x308+i*24);
        int random=random_value(s);float width,alpha;
        width=stored(sqrt((double)motion.z*motion.z+
            ((double)motion.x*motion.x-(double)motion.y*motion.y*-4.0))+.5);
        alpha=stored(.5-(double)random*-1.52587890625e-05);
        queued(s,bh_draw_segment(s->queue,3,s->trail_image,&from,&to,
            .5f,width,1,alpha,0));
    }
}

static void tail(BHPlayerScene *s) {
    BHEntity *p=s->player;unsigned long resource=word_at(p,0x21c);
    BHVec3 position=vector_at(p,0xc),angles=vector_at(p,0x3c);
    if(resource)queued(s,bh_draw_submit_resource(s->queue,0,resource,&position,&angles,1));
    if(*s->frame_count<75){
        if(byte_at(p,0x2b4)&0x10){
            BHDrawCommand *command=bh_queue_reserve(s->queue,8);
            ++s->attempted;
            if(command){++s->queued;command->kind=BH_DRAW_STATE;command->words[0]=3;}
            else if(s->queue->error==BH_DRAW_FULL)++s->dropped;
            else if(s->error==BH_PLAYER_SCENE_OK)s->error=BH_PLAYER_SCENE_QUEUE;
            command=bh_queue_reserve(s->queue,8);++s->attempted;
            if(command){
                float half=(float)(*s->display_width/2);
                ++s->queued;command->kind=BH_DRAW_SCREEN_QUAD;
                command->resource=s->overlay_image;
                command->words[0]=bits(half);
                command->words[1]=bits(stored((double)*s->display_height*.75));
                command->words[2]=bits(half);command->words[3]=bits(half);
                command->words[4]=0;command->words[5]=bits(1);
                command->words[6]=0;
            }else if(s->queue->error==BH_DRAW_FULL)++s->dropped;
            else if(s->error==BH_PLAYER_SCENE_OK)s->error=BH_PLAYER_SCENE_QUEUE;
        }
        queued(s,bh_draw_submit_resource(s->queue,0,s->cockpit_resource,&position,&angles,1));
        trail(s);
    }
}

static int selected(BHPlayerScene *s,unsigned char index) {
    BHEntity *weapon;BHWeaponObjectState *state;unsigned long method;
    BHWeaponHudArcPlayer arc_player;
    if(!bh_weapon_registry_hud(s->weapons,index,&weapon,&state,&method) ||
       word_at(s->player,0x258+index*4)!=(unsigned long)weapon)
        return fail(s,BH_PLAYER_SCENE_WEAPON);
    switch(method) {
    case 0x0047d680UL: /* verified shared RET method */
        return 1;
    case 0x00432ff0UL:
        arc_player.position=vector_at(s->player,0xc);
        arc_player.forward=vector_at(s->player,0x30);
        arc_player.aim=vector_at(s->player,0x280);
        if(!bh_weapon_hud_arc(s->arc,&state->common.hud,&arc_player))
            return fail(s,BH_PLAYER_SCENE_OWNER);
        s->random_calls+=s->arc->random_calls;
        return 1;
    case 0x004314f0UL:
        s->target->viewport_width=*s->display_width;
        if(!bh_weapon_hud_target_draw(s->target,&state->common.hud))
            return fail(s,BH_PLAYER_SCENE_OWNER);
        return 1;
    default:return fail(s,BH_PLAYER_SCENE_WEAPON);
    }
}
static int status_weapons(BHPlayerScene *s,BHHudStatusState *status,
    const BHHudWeaponAmmo **ammunition) {
    unsigned char chosen[2];unsigned long i;
    chosen[0]=status->primary;chosen[1]=status->secondary;
    for(i=0;i<2;++i){
        BHEntity *weapon;BHWeaponObjectState *state;
        unsigned long identity,index=chosen[i];
        if(!bh_weapon_registry_hud(s->weapons,index,&weapon,&state,&identity) ||
           word_at(s->player,0x258+index*4)!=(unsigned long)weapon)
            return fail(s,BH_PLAYER_SCENE_WEAPON);
        ammunition[index]=(const BHHudWeaponAmmo *)weapon;
    }
    return 1;
}

int __cdecl bh_player_scene_draw(BHPlayerScene *s) {
    BHEntity *p;BHCameraState *camera;
    BHHudStatusState status;
    const BHHudWeaponAmmo *ammunition[10];
    unsigned long i;
    int active;
    if(!s || !(p=s->player) || !s->queue || !s->frame_count ||
       !s->display_width || !s->display_height || !s->mode_count)
        return fail(s,BH_PLAYER_SCENE_ARGUMENT);
    s->error=BH_PLAYER_SCENE_OK;
    s->attempted=s->queued=s->dropped=s->random_calls=0;
    active=byte_at(p,0x2bb)==1 && *s->mode_count!=0;
    if(active){
        float flag,flash,amount,gauge,oscillation;
        BHVec3 *eye,*angles,*velocity;
        if(!s->sound || !s->hud || !s->hud->frame || !s->markers || !s->radar ||
           !s->world_status || !s->flat_status || !s->scene || !s->background ||
           !s->actor_head || !s->pickup_head || !s->weapons || !s->arc ||
           !s->target || !s->confirmation_gate ||
           (*s->mode_count>1 && (!s->score_draw || !s->scores)))
            return fail(s,BH_PLAYER_SCENE_BINDING);
        if(!s->hud->attachment || s->hud->attachment->queue!=s->queue ||
           s->radar->queue!=s->queue || !s->markers->projection ||
           s->markers->projection->queue!=s->queue || s->markers->radar!=s->radar ||
           s->world_status->hud!=s->hud || s->flat_status->hud!=s->hud ||
           s->background->queue!=s->queue || s->arc->queue!=s->queue ||
           !s->target->projection || s->target->projection->queue!=s->queue ||
           (*s->mode_count>1 && s->score_draw->queue!=s->queue))
            return fail(s,BH_PLAYER_SCENE_BINDING);
        camera=bh_player_camera_state(p);
        eye=&camera->position;angles=&camera->angles;velocity=&camera->velocity;
        if(!finite_vec(eye) || !finite_vec(angles) || !finite_vec(velocity))
            return fail(s,BH_PLAYER_SCENE_ARGUMENT);
        if(!bh_sound_listener(s->sound,0,1,eye,angles,velocity))
            fail(s,BH_PLAYER_SCENE_OWNER);
        queued(s,bh_camera_queue_pose(s->queue,eye,angles));
        queued(s,bh_horizon_queue(s->queue,s->horizon_image,5000,5000,25000,12500,0x804000));
        flag=(float)byte_at(p,0x2b8);
        oscillation=stored(sin((double)flag*.02454369260617026)*.1);
        queued(s,bh_material_queue_words(s->queue,5,s->material_image,
            1,bits(.05f),0,bits(stored((double)flag*.00390625)),
            0,bits(.05f),bits(stored((double)oscillation-
                (double)flag*-.00390625))));
        for(i=0;i<8;++i)
            if(bh_scene_background_step(s->background,i,eye)<0)
                fail(s,BH_PLAYER_SCENE_OWNER);
        if(*s->frame_count<150 || *s->mode_count>1){
            flash=float_at(p,0x254);
            if(!((double)flash>.1)){
                queued(s,bh_fade_queue(s->queue,0,0));
            }else{
                unsigned long color;long component;
                amount=stored((double)flash*flash);
                if((double)amount>.9)amount=.9f;
                gauge=float_at(p,0x24c);
                component=(long)((1.0-(double)gauge)*-127.0);
                color=(0xff00UL-(unsigned long)component)<<8;
                if((double)amount>.05)queued(s,bh_fade_queue(s->queue,amount,color));
                put_float(p,0x254,stored((.65-(double)float_at(p,0x23c)*-.35)*flash));
            }
        }else{
            amount=stored((double)(*s->frame_count-150)*.019999999552965164);
            if((double)amount>1)amount=1;
            if((double)amount>.05)queued(s,bh_fade_queue(s->queue,amount,0));
        }
        aim(s);
        s->hud->attachment->viewport_width=*s->display_width;
        if(!bh_hud_setup(s->hud,(const BHHudPlayer *)p))
            fail(s,BH_PLAYER_SCENE_OWNER);
        s->markers->display_width=*s->display_width;
        if(!bh_hud_markers_draw(s->markers,*s->actor_head,
            &p->position,(const unsigned char *)p+0x2b8))
            fail(s,BH_PLAYER_SCENE_OWNER);
        {
            BHEntity *pickup=*s->pickup_head;
            unsigned long visited=0;
            while(pickup){
                BHEntity *next;
                if(visited++>=s->pickup_limit){fail(s,BH_PLAYER_SCENE_BINDING);break;}
                if(!bh_radar_marker(s->radar,&p->position,&pickup->position,2,0,0))
                    fail(s,BH_PLAYER_SCENE_OWNER);
                next=bh_pickup_state(pickup)->global_next;
                pickup=next;
            }
        }
        /* Resolve each selector only when its synchronous callback is due. */
        if(!selected(s,byte_at(p,0x2b6)))return 0;
        if(!selected(s,byte_at(p,0x2b7)))return 0;
        for(i=0;i<10;++i)ammunition[i]=0;
        status_state(s,&status,ammunition);
        if(!status_weapons(s,&status,ammunition))return 0;
        if(long_at(p,0xe8)==0){
            if(!bh_hud_world_draw(s->world_status,&status))
                fail(s,BH_PLAYER_SCENE_OWNER);
        }else{
            s->flat_status->viewport_height=*s->display_height;
            if(!bh_hud_flat_draw(s->flat_status,&status))
                fail(s,BH_PLAYER_SCENE_OWNER);
        }
        status_publish(s,&status);
        if(*s->mode_count>1){
            s->score_draw->display_width=*s->display_width;
            s->score_draw->near_z=s->hud->frame->near_z;
            if(!bh_hud_scores_draw(s->score_draw,s->scores))
                fail(s,BH_PLAYER_SCENE_OWNER);
        }
        if(!bh_scene_owner_step(s->scene,eye,angles,60,.01f,25000,
            *s->display_width,*s->display_height) &&
            s->scene->error!=BH_SCENE_OWNER_INACTIVE &&
            !(s->scene->error==BH_SCENE_OWNER_QUEUE && s->queue->error==BH_DRAW_FULL))
            fail(s,BH_PLAYER_SCENE_OWNER);
        if(*s->confirmation_gate){
            float half=stored((double)*s->display_width*.5);
            float size=stored((double)*s->display_width*.2);
            queued(s,bh_screen_quad_queue(s->queue,7,s->confirmation_image,
                half,stored((double)*s->display_height*.5),size,size,0,0,1));
        }
        particles(s);
    }
    tail(s);
    return s->error==BH_PLAYER_SCENE_OK;
}
