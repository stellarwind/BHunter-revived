#include "bh_light.h"
#include "bh_round.h"
#include <math.h>
#include <float.h>
static int finite_vector(const BHVec3 *v) {
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int valid(const BHLightContext *c,const BHVec3 *reference,float radius) {
    const BHLight *p,*slow,*fast;unsigned long i;
    if(!c || !_finite(radius) || !finite_vector(c->reference_override?c->reference_override:reference))return 0;
    for(i=0;i<12;++i)if(!_finite(((const float *)&c->world_to_model)[i]))return 0;
    slow=fast=c->lights;
    while(fast && fast->next){slow=slow->next;fast=fast->next->next;if(slow==fast)return 0;}
    for(p=c->lights;p;p=p->next) {
        if(p->type==2 && !finite_vector(&p->direction))return 0;
        if(p->type==3 || p->type==4) {
            if(!finite_vector(&p->position) || !_finite(p->range) || !_finite(p->intensity))return 0;
            if(p->type==4 && (!finite_vector(&p->direction) || !_finite(p->inner_cos) || !_finite(p->outer_cos)))return 0;
        }
    }
    return 1;
}
int __cdecl bh_light_prepare(BHLightContext *c,const BHVec3 *reference,float radius) {
    BHLight *p;BHVec3 delta;float distance,attenuation,alignment;double z,reciprocal,retained;
    if(!valid(c,reference,radius))return 0;
    if(c->reference_override) {
        reference=c->reference_override;
        bh_mat34_transform_point(&c->world_to_model,reference,&c->local_reference);
    }
    for(p=c->lights;p;p=p->next) {
        if(p->type==2) {
            bh_mat34_transform_direction(&c->world_to_model,&p->direction,&p->local_direction);
        } else if(p->type==3 || p->type==4) {
            bh_round_float(&delta.x,(double)reference->x-p->position.x);
            bh_round_float(&delta.y,(double)reference->y-p->position.y);
            z=(double)reference->z-p->position.z;bh_round_float(&delta.z,z);
            /* Original retains the Z subtraction across its float store, then
               multiplies by the stored Z. X/Y are both reloaded float values. */
            bh_round_float(&distance,sqrt(((double)delta.x*delta.x+(double)delta.y*delta.y)+z*delta.z));
            if(p->range>0 && (double)radius+p->range<distance) {p->prepared_type=0;continue;}
            if((double)distance*.125<=radius) {
                bh_mat34_transform_point(&c->world_to_model,&p->position,&p->local_position);
                if(p->type==4)bh_mat34_transform_direction(&c->world_to_model,&p->direction,&p->local_direction);
                p->prepared_type=p->type;continue;
            }
            retained=p->range>0?1.0-(double)distance/p->range:1.0;
            bh_round_float(&attenuation,retained);
            if(p->type==4) {
                retained=(((double)p->direction.x*delta.x+(double)p->direction.y*delta.y)+(double)p->direction.z*delta.z)/distance;
                bh_round_float(&alignment,retained);
                /* Outer comparison uses the retained value; inner comparison
                   and ramp reload the float, as the original instructions do. */
                if(retained<p->outer_cos)attenuation=0;
                else if(alignment<p->inner_cos)bh_round_float(&attenuation,(((double)alignment-p->outer_cos)/((double)p->inner_cos-p->outer_cos))*attenuation);
                retained=attenuation;
            }
            if(retained<=0) {p->prepared_type=0;continue;}
            reciprocal=1.0/distance;
            bh_round_float(&delta.x,(double)delta.x*reciprocal);
            bh_round_float(&delta.y,(double)delta.y*reciprocal);
            bh_round_float(&delta.z,(double)delta.z*reciprocal);
            p->prepared_type=2;
            bh_mat34_transform_direction(&c->world_to_model,&delta,&p->local_direction);
            bh_round_float(&p->effective_intensity,(double)p->intensity*attenuation);
        }
    }
    return 1;
}
