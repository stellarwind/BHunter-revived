#include "bh_debris_response.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <math.h>
#include <string.h>

typedef char blast_size[sizeof(BHDebrisBlastPacket)==24 ? 1:-1];
typedef char blast_radius[offsetof(BHDebrisBlastPacket,radius)==20 ? 1:-1];
typedef char bridge_size[sizeof(((BHEntity *)0)->payload)==636 && sizeof(void *)==4 ? 1:-1];
static BHDebrisBody *body_of(BHEntity *entity) {
    BHDebrisBody *body;
    if(!entity) return NULL;
    memcpy(&body,entity->payload+632,sizeof(body));
    return body;
}
static int live(BHDebrisContext *ctx,BHEntity *entity) {
    BHEntity *p;unsigned long steps=0;
    if(!ctx || !ctx->pool) return 0;
    for(p=ctx->pool->active;p && steps++<ctx->pool->capacity;p=p->next)
        if(p==entity) return 1;
    return 0;
}
static int registry_end(BHDebrisContext *ctx,unsigned long *end) {
    unsigned long i;
    if(!ctx || !ctx->registry) return 0;
    for(i=0;i<ctx->registry_capacity;++i) if(!ctx->registry[i]) {*end=i;return 1;}
    ctx->error=1;return 0;
}
static BHDebrisBody *checked(BHEntity *entity) {
    BHDebrisBody *b;
    if(!entity || (entity->query!=bh_debris_receive && entity->step!=bh_debris_remove)) return NULL;
    b=body_of(entity);
    if(!b || b->entity!=entity || b->generation!=entity->generation || !live(b->context,entity)) return NULL;
    return b;
}
int __cdecl bh_debris_context_init(BHDebrisContext *ctx,BHEntityPool *pool,
    BHSoundQueue *sound,unsigned long id,BHEntity **registry,unsigned long capacity) {
    BHDebrisContext next;unsigned long end;
    if(!ctx || !pool || !sound || !registry || !capacity || capacity>65536 || !bh_entities_valid(pool)) return 0;
    memset(&next,0,sizeof(next));next.pool=pool;next.sound=sound;next.contact_sound=id;
    next.registry=registry;next.registry_capacity=capacity;
    if(!registry_end(&next,&end)) return 0;
    *ctx=next;return 1;
}
int __cdecl bh_debris_context_dispose(BHDebrisContext *ctx) {
    BHEntity *p;BHDebrisBody *b;unsigned long steps=0;
    if(!ctx || !ctx->pool) return 0;
    for(p=ctx->pool->active;p && steps++<ctx->pool->capacity;p=p->next) {
        b=checked(p);if(b && b->context==ctx) {ctx->error=1;return 0;}
    }
    memset(ctx,0,sizeof(*ctx));return 1;
}
int __cdecl bh_debris_bind(BHDebrisContext *ctx,BHDebrisBody *body,BHEntity *entity,
    BHPhysicsState *physics,float mass,float friction,const BHVec3 *inertia) {
    BHDebrisBody next;
    if(!body || body->entity || !physics || !inertia || !live(ctx,entity) ||
       entity->query || entity->step==bh_debris_remove) return 0;
    memset(&next,0,sizeof(next));next.context=ctx;next.entity=entity;next.generation=entity->generation;
    next.physics=physics;next.mass=mass;next.friction=friction;next.inertia=*inertia;next.last_sound=BH_SOUND_INVALID;
    if(!bh_entity_set_query(ctx->pool,entity,bh_debris_receive)) return 0;
    *body=next;memcpy(entity->payload+632,&body,sizeof(body));return 1;
}
int __cdecl bh_debris_unbind(BHDebrisBody *body) {
    BHEntity *entity;
    if(!body || !(entity=body->entity) || checked(entity)!=body) return 0;
    if(!bh_entity_set_query(body->context->pool,entity,NULL)) return 0;
    if(entity->step==bh_debris_remove) entity->step=NULL;
    body->entity=NULL;return 1;
}
void __cdecl bh_debris_remove(BHEntity *entity) {
    BHDebrisBody *b=checked(entity);unsigned long end,i,j;
    if(!b || !registry_end(b->context,&end)) return;
    for(i=0;i<end;++i) if(b->context->registry[i]==entity) {
        for(j=i;j<end;++j) b->context->registry[j]=b->context->registry[j+1];
        break;
    }
    if(!bh_entity_release(b->context->pool,entity)) {b->context->error=1;return;}
    b->entity=NULL;
}
void __cdecl bh_debris_receive(BHEntity *entity,void *packet) {
    BHDebrisBody *b=checked(entity);
    BHPhysicsState *s;const BHMat34 *m;
    BHPhysicsContactPacket contact;BHDebrisBlastPacket blast;BHDebrisOwnerPacket owner;
    BHVec3 n,arm,impulse,torque,zero={0,0,0};
    double x,y,z,length,factor,normal_factor,mass_impulse;
    float y_force,pitch;
    long event;unsigned long i;
    if(!b || !packet) return;
    memcpy(&event,packet,4);s=b->physics;m=&s->pose;
    switch(event) {
    case 6: entity->step=NULL;return;
    case 17:
        memcpy(&owner,packet,sizeof(owner));if(owner.owner==b->owner) b->owner=NULL;
        return;
    case 20:
        entity->step=bh_debris_remove;
        if(!bh_entity_set_query(b->context->pool,entity,NULL)) {b->context->error=1;return;}
        entity->refresh=NULL;return;
    case 1:
        if(!bh_math_environment_supported()) {b->context->error=1;return;}
        memcpy(&blast,packet,sizeof(blast));
        x=(double)entity->position.x-blast.center.x;
        y=(double)entity->position.y-blast.center.y;
        z=(double)entity->position.z-blast.center.z;
        length=sqrt((x*x+y*y)+z*z);
        if(blast.radius<=length) return;
        factor=(((double)blast.strength/b->mass)*(1.0-length/blast.radius)*30.0)/length;
        bh_round_float(&s->acceleration.x,x*factor+s->acceleration.x);
        bh_round_float(&s->acceleration.y,y*factor+s->acceleration.y);
        bh_round_float(&s->acceleration.z,z*factor+s->acceleration.z);
        break;
    case 2:
        if(!bh_math_environment_supported()) {b->context->error=1;return;}
        memcpy(&contact,packet,sizeof(contact));
        /* Original column dots have different addition orders. */
        bh_round_float(&n.x,((double)contact.normal.y*m->m[1][0]+(double)contact.normal.z*m->m[2][0])+(double)contact.normal.x*m->m[0][0]);
        bh_round_float(&n.y,((double)contact.normal.x*m->m[0][1]+(double)contact.normal.z*m->m[2][1])+(double)contact.normal.y*m->m[1][1]);
        bh_round_float(&n.z,((double)contact.normal.y*m->m[1][2]+(double)contact.normal.z*m->m[2][2])+(double)contact.normal.x*m->m[0][2]);
        x=(double)contact.point.x-entity->position.x;
        y=(double)contact.point.y-entity->position.y;
        /* Z is spilled to float before the inverse column dots. */
        bh_round_float(&y_force,(double)contact.point.z-entity->position.z);z=y_force;
        bh_round_float(&arm.x,((double)m->m[0][0]*x+(double)m->m[1][0]*y)+(double)m->m[2][0]*z);
        bh_round_float(&arm.y,((double)m->m[0][1]*x+(double)m->m[1][1]*y)+(double)m->m[2][1]*z);
        bh_round_float(&arm.z,((double)m->m[0][2]*x+(double)m->m[2][2]*z)+(double)m->m[1][2]*y);
        bh_vec3_normalize(&arm);
        normal_factor=((double)b->friction*contact.impulse)*900.0;
        mass_impulse=(double)contact.impulse*b->mass;
        for(i=0;i<3;++i) bh_round_float(((float *)&impulse)+i,
            ((double)((float *)&n)[i]*mass_impulse)*270.0);
        bh_round_float(&y_force,(double)contact.normal.y*normal_factor);
        bh_round_float(&s->acceleration.x,(double)s->acceleration.x-contact.normal.x*normal_factor);
        bh_round_float(&s->acceleration.y,(double)s->acceleration.y-y_force);
        bh_round_float(&s->acceleration.z,(double)s->acceleration.z-contact.normal.z*normal_factor);
        bh_vec3_cross(&arm,&impulse,&torque);
        for(i=0;i<3;++i) {
            bh_round_float(((float *)&torque)+i,(double)((float *)&torque)[i]/((float *)&b->inertia)[i]);
            bh_round_float(((float *)&s->angular_acceleration)+i,
                (double)((float *)&s->angular_acceleration)[i]-((float *)&torque)[i]);
            bh_round_float(((float *)&s->angular_velocity)+i,(double)((float *)&s->angular_velocity)[i]*.85);
        }
        if(contact.severity>.05) {
            bh_round_float(&pitch,.9-(double)contact.severity*-.2);
            b->last_sound=bh_sound_request(b->context->sound,b->context->contact_sound,
                contact.severity,pitch,&contact.point,&zero);
        }
        break;
    default:return;
    }
    if(b->sleep_time>2) b->sleep_time=0;
}
