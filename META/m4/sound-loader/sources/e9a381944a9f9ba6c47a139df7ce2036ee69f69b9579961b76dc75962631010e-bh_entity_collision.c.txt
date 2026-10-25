#include "bh_entity_collision.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <float.h>
#include <string.h>

int __cdecl bh_collision_candidate_world(BHCollisionCandidate *out,
    BHWorldGeometry *world,BHSweepQuery *sweep) {
    BHCollisionCandidate next;
    BHGeometry *geometry;
    if(!out || !world || !sweep || !sweep->initialized) return 0;
    geometry=world->model ? world->model->geometry : NULL;
    if(sweep->b!=geometry) return 0;
    memset(&next,0,sizeof(next));next.object.kind=BH_WORLD_HIT_GEOMETRY;
    next.object.object.geometry=world;next.sweep=sweep;next.world=world->transform;
    bh_mat34_inverse_rigid(&next.world,&next.local);
    *out=next;return 1;
}
int __cdecl bh_collision_candidate_actor(BHCollisionCandidate *out,BHEntity *actor,
    BHSweepQuery *sweep,const BHMat34 *world,const BHMat34 *local,
    const BHVec3 *movement,unsigned long flags) {
    BHCollisionCandidate next;
    if(!out || !actor || !sweep || !sweep->initialized || !world || !local || !movement) return 0;
    memset(&next,0,sizeof(next));next.object.kind=BH_WORLD_HIT_ACTOR;
    next.object.object.actor=actor;next.sweep=sweep;next.world=*world;next.local=*local;
    next.movement=*movement;next.flags=flags;*out=next;return 1;
}
static void apply(BHMat34 *pose,const BHVec3 *movement,const BHVec3 *angles) {
    bh_mat34_translate_world(movement->x,movement->y,movement->z,pose);
    bh_mat34_rotate_x(angles->x,pose);bh_mat34_rotate_y(angles->y,pose);bh_mat34_rotate_z(angles->z,pose);
}
static int candidate(BHCollisionCandidate *c,BHMat34 *pose,const BHVec3 *movement,
    const BHVec3 *angles,int fallback,BHSweepHit *out,int single) {
    BHVec3 relative;
    int result,moving;
    if(c->object.kind==BH_WORLD_HIT_ACTOR) {
        if(!(c->flags&2)) return 0;
        moving=c->movement.x!=0 || c->movement.y!=0 || c->movement.z!=0;
        if(moving) {
            bh_round_float(&relative.x,(double)movement->x-c->movement.x);
            bh_round_float(&relative.y,(double)movement->y-c->movement.y);
            bh_round_float(&relative.z,(double)movement->z-c->movement.z);
            bh_mat34_translate_world(c->movement.x,c->movement.y,c->movement.z,pose);
            result=bh_sweep_run(c->sweep,pose,&relative,angles,&c->world,&c->local,fallback,out);
            if(result==0 && single)
                bh_mat34_translate_world(-c->movement.x,-c->movement.y,-c->movement.z,pose);
            if(result==1) {
                bh_round_float(&out->motion.x,(double)out->motion.x*1.5);
                bh_round_float(&out->motion.y,(double)out->motion.y*1.5);
                bh_round_float(&out->motion.z,(double)out->motion.z*1.5);
            }
            return result;
        }
    }
    return bh_sweep_run(c->sweep,pose,movement,angles,&c->world,&c->local,fallback,out);
}
static int finite_matrix(const BHMat34 *m) {
    unsigned long i;
    for(i=0;i<12;++i) if(!_finite(((const float *)m)[i])) return 0;
    return 1;
}
int __cdecl bh_entity_collision_dispatch(BHCollisionCandidate *c,unsigned long count,
    BHMat34 *pose,BHVec3 *movement,BHVec3 *angles,int fallback,BHSweepHit *out) {
    BHMat34 next,best_pose;
    BHVec3 delta,rotation;
    BHSweepHit hit,best;
    BHCollisionCandidate swap;
    BHGeometry *source=NULL;
    unsigned long i,j,selected=0;
    float fraction=1;
    int result,found=0;
    if(!pose || !movement || !angles || !out || (count && !c) || count>65535 ||
       !bh_math_environment_supported() || !finite_matrix(pose)) return -1;
    for(j=0;j<3;++j)
        if(!_finite(((float *)movement)[j]) || !_finite(((float *)angles)[j])) return -1;
    for(i=0;i<count;++i) {
        if(!c[i].sweep || !c[i].sweep->initialized || c[i].sweep->busy ||
           !finite_matrix(&c[i].world) || !finite_matrix(&c[i].local)) return -1;
        if(i==0) source=c[i].sweep->a;
        if(c[i].sweep->a!=source) return -1;
        if(c[i].object.kind==BH_WORLD_HIT_GEOMETRY) {
            if(!c[i].object.object.geometry) return -1;
        } else if(c[i].object.kind==BH_WORLD_HIT_ACTOR) {
            if(!c[i].object.object.actor) return -1;
        } else return -1;
        for(j=0;j<3;++j) if(!_finite(((float *)&c[i].movement)[j])) return -1;
    }
    next=*pose;delta=*movement;rotation=*angles;hit=*out;
    if(!count) {apply(&next,&delta,&rotation);*pose=next;return 0;}
    if(count==1) {
        result=candidate(c,&next,&delta,&rotation,fallback,&hit,1);
        if(result>=0) {*pose=next;*out=hit;}
        return result;
    }
    for(i=0;i<count;++i) {
        next=*pose;
        result=candidate(c+i,&next,&delta,&rotation,fallback,&hit,0);
        if(result<0) return -1;
        if(result) {
            for(j=0;j<3;++j) {
                bh_round_float(((float *)&delta)+j,(double)((float *)&delta)[j]*hit.fraction);
                bh_round_float(((float *)&rotation)+j,(double)((float *)&rotation)[j]*hit.fraction);
            }
            bh_round_float(&fraction,(double)fraction*hit.fraction);
            found=1;selected=i;best_pose=next;best=hit;
        }
    }
    if(found) {
        best.fraction=fraction;*out=best;*pose=best_pose;*movement=delta;*angles=rotation;
        swap=c[0];c[0]=c[selected];c[selected]=swap;
    } else {*pose=next;*out=hit;}
    return found;
}
