#include "bh_actor_basis.h"
#include "bh_round.h"
#include <math.h>

typedef char actor_basis_sample_size[sizeof(BHActorBasisSample)==36 ? 1:-1];
typedef char actor_basis_group_size[sizeof(BHActorBasisGroup)==48 ? 1:-1];
typedef char actor_basis_indices_size[sizeof(BHActorBasisIndices)==64 ? 1:-1];
typedef char actor_basis_scale_size[sizeof(BHActorBasisScale)==8 ? 1:-1];

const long bh_actor_basis_definition_a[25]={1,0,4,1,6,7,12,4,3,4,9,10,2,2,5,2,8,11,2,2,11,2,5,8,-1};
const long bh_actor_basis_definition_b[22]={1,0,4,2,5,8,11,1,13,2,1,10,2,4,7,2,6,3,2,9,12,-1};

static int group_capacity(const BHActorBasisContext *c) {
    return c->samples && c->sample_count>=14 && c->groups_a && c->groups_b &&
        c->indices_a && c->indices_b && c->scales_a && c->scales_b &&
        c->sample_count<=c->sample_capacity && c->a_count<=c->a_capacity &&
        c->b_count<=c->b_capacity && c->a_capacity-c->a_count>=7 && c->b_capacity-c->b_count>=7;
}
static void add(BHVec3 *sum,const BHVec3 *value) {
    bh_round_float(&sum->x,(double)value->x+sum->x);
    bh_round_float(&sum->y,(double)value->y+sum->y);
    bh_round_float(&sum->z,(double)value->z+sum->z);
}
static double reciprocal(const BHVec3 *v) {
    return 1/sqrt(((double)v->x*v->x+(double)v->y*v->y)+(double)v->z*v->z);
}
static void scaled(const BHVec3 *v,double scale,BHVec3 *out) {
    bh_round_float(&out->x,(double)v->x*scale);
    bh_round_float(&out->y,(double)v->y*scale);
    bh_round_float(&out->z,(double)v->z*scale);
}
static void groups(BHActorBasisContext *c,const long *definition,
    BHActorBasisGroup *out,BHActorBasisIndices *indices,BHActorBasisScale *scales,
    unsigned long *count) {
    unsigned long cursor=0,i;long n,index;double a,b;BHActorBasisGroup *group;
    while(definition[cursor]>0) {
        n=definition[cursor++];group=out+*count;
        group->direction.x=group->direction.y=group->direction.z=0;
        group->torque.x=group->torque.y=group->torque.z=0;
        indices[*count].count=1;
        for(i=0;i<(unsigned long)n;++i) {
            index=definition[cursor++];indices[*count].sample[i]=index;
            ++indices[*count].count;
            add(&group->direction,&c->samples[index].direction);
            add(&group->torque,&c->samples[index].torque);
        }
        a=reciprocal(&group->direction);b=reciprocal(&group->torque);
        scaled(&group->direction,a,&group->unit_direction);scaled(&group->torque,b,&group->unit_torque);
        bh_round_float(&scales[*count].direction,a);bh_round_float(&scales[*count].torque,b);
        ++*count;
    }
}
int __cdecl bh_actor_basis_build_groups(BHActorBasisContext *context) {
    if(!context || !group_capacity(context))return 0;
    groups(context,bh_actor_basis_definition_a,context->groups_a,context->indices_a,context->scales_a,&context->a_count);
    groups(context,bh_actor_basis_definition_b,context->groups_b,context->indices_b,context->scales_b,&context->b_count);
    return 1;
}
static void sample(BHActorBasisContext *context,const BHVec3 *point,const BHVec3 *direction) {
    BHActorBasisSample *out=context->samples+context->sample_count;
    bh_vec3_normalize_to(direction,&out->direction);
    bh_vec3_cross(point,direction,&out->torque);out->point=*point;++context->sample_count;
}
int __cdecl bh_actor_basis_acquire(BHActorBasisContext *context) {
    BHVec3 point={0,0,0},direction={0,0,0};unsigned long i;BHActorBasisContext projected;
    static const float corners[4][2]={{.7f,1.5f},{-.7f,1.5f},{-.7f,-1.5f},{.7f,-1.5f}};
    if(!context)return 0;
    if(context->refcount){++context->refcount;return 1;}
    if(!context->samples || context->sample_count>context->sample_capacity || context->sample_capacity-context->sample_count<14)return 0;
    projected=*context;projected.sample_count+=14;
    if(!group_capacity(&projected))return 0;
    ++context->refcount;
    sample(context,&point,&direction);
    for(i=0;i<4;++i) {
        point.x=corners[i][0];point.y=0;point.z=corners[i][1];
        direction.x=(i==0 || i==3)?1.0f:-1.0f;direction.y=direction.z=0;sample(context,&point,&direction);
        direction.x=0;direction.y=1;sample(context,&point,&direction);
        direction.y=0;direction.z=(i<2)?1.0f:-1.0f;sample(context,&point,&direction);
    }
    point.x=point.z=0;point.y=-1;direction.x=direction.z=0;direction.y=-1;sample(context,&point,&direction);
    return bh_actor_basis_build_groups(context);
}
