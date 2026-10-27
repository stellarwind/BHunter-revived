#include "bh_script_motion.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char motion_layout[sizeof(void*)==4 && offsetof(BHEntity,payload)==0xe4 &&
                           offsetof(BHEntity,origin)==0x48 ? 1:-1];
static void get(const BHEntity *e,unsigned long o,void *v,unsigned long n){memcpy(v,(const unsigned char*)e+o,n);}
static void put(BHEntity *e,unsigned long o,const void *v,unsigned long n){memcpy((unsigned char*)e+o,v,n);}
static BHScriptMotionContext *context(BHEntity *e){BHScriptMotionContext *c;memcpy(&c,e->payload+408,4);return c;}
static int live(BHEntityPool *pool,const BHEntity *e){BHEntity *p;unsigned long n=0;
    for(p=pool->active;p&&n++<pool->capacity;p=p->next)if(p==e)return 1;return 0;
}
int __cdecl bh_script_motion_actor_read(const BHEntity *e,BHScriptMotionActor *s){if(!e||!s)return 0;
    get(e,0x3c,&s->angles,12);get(e,0x1b0,&s->minimum,12);get(e,0x1bc,&s->maximum,12);
    get(e,0x150,&s->local_to_world,48);get(e,0x180,&s->world_to_local,48);get(e,0x12c,&s->attachments,4);return 1;
}
int __cdecl bh_script_motion_actor_write(BHEntity *e,const BHScriptMotionActor *s){if(!e||!s)return 0;
    put(e,0x3c,&s->angles,12);put(e,0x1b0,&s->minimum,12);put(e,0x1bc,&s->maximum,12);
    put(e,0x150,&s->local_to_world,48);put(e,0x180,&s->world_to_local,48);put(e,0x12c,&s->attachments,4);return 1;
}
BHEntity *__cdecl bh_script_motion_attachment_next(const BHEntity *e){BHEntity *next=NULL;if(e)get(e,0xec,&next,4);return next;}
void __cdecl bh_script_motion_attachment_link(BHEntity *e,BHEntity *next){if(e)put(e,0xec,&next,4);}
static BHEntity *attachments(BHScriptMotionContext *c,BHEntity *e,BHEntity *raw){unsigned long i;
    for(i=0;c->bindings&&i<c->binding_count;++i)if(c->bindings[i].entity==e&&c->bindings[i].attachments_owner)return *c->bindings[i].attachments_owner;
    return raw;
}
static int valid(BHScriptMotionContext *c,BHEntity *e,const BHScriptLifetimeState *s,int rotate){
    BHEntityPool *pool;BHEntity *p;BHScriptMotionActor a;unsigned long n=0;
    if(!c||!c->lifetime||!c->lifetime->timers.pool||!e||!s){if(c)c->error=BH_MOTION_ARGUMENT;return 0;}
    pool=c->lifetime->timers.pool;
    if(e==s->actor||!live(pool,e)||!live(pool,s->actor)){c->error=BH_MOTION_LIFETIME;return 0;}
    if(!bh_math_environment_supported()){c->error=BH_MOTION_NUMERICAL;return 0;}
    if(!rotate){bh_script_motion_actor_read(s->actor,&a);
        for(p=attachments(c,s->actor,a.attachments);p;p=bh_script_motion_attachment_next(p)){
            if(++n>pool->used||p==s->actor||p==e||!live(pool,p)){c->error=BH_MOTION_ATTACHMENTS;return 0;}
        }
    }
    c->error=BH_MOTION_OK;return 1;
}
int __cdecl bh_script_motion_bind(BHScriptMotionContext *c,BHEntity *e,const BHScriptLifetimeState *s,int rotate){
    if(!valid(c,e,s,rotate))return 0;
    if(!bh_script_lifetime_bind(c->lifetime,e,s,rotate?bh_script_sensor_query:bh_script_motion_query)){
        c->error=BH_MOTION_ARGUMENT;return 0;
    }
    memcpy(e->payload+408,&c,4);e->step=rotate?bh_script_rotate_tick:bh_script_translate_tick;return 1;
}
static void add(BHVec3 *v,const BHVec3 *d){
    bh_round_float(&v->x,(double)v->x+d->x);bh_round_float(&v->y,(double)v->y+d->y);bh_round_float(&v->z,(double)v->z+d->z);
}
static void publish(BHScriptMotionContext *c,BHEntity *e){unsigned long i;BHScriptMotionActor a;const BHScriptMotionBinding *b;
    for(i=0;c->bindings&&i<c->binding_count;++i){b=c->bindings+i;if(b->entity!=e)continue;
        if(!b->world){c->error=BH_MOTION_ARGUMENT;return;}
        bh_script_motion_actor_read(e,&a);b->world->position=e->position;
        b->world->transform=a.local_to_world;b->world->inverse=a.world_to_local;
        b->world->minimum=a.minimum;b->world->maximum=a.maximum;
        if(b->draw_angles)*b->draw_angles=a.angles;return;
    }
}
static int expire(BHEntity *e,float countdown){double next;
    if(!(countdown>0))return 0;next=(double)countdown-(1.0/30.0);
    bh_round_float(&countdown,next);put(e,0x128,&countdown,4);return !(next>0);
}
void __cdecl bh_script_translate_tick(BHEntity *e){BHScriptMotionContext *c=context(e);BHScriptLifetimeState s;
    BHScriptMotionActor a;BHEntity *p;BHVec3 delta;
    bh_script_lifetime_read(e,&s);if(!valid(c,e,&s,0))return;
    delta=s.delta;bh_script_motion_actor_read(s.actor,&a);
    add(&a.minimum,&delta);add(&a.maximum,&delta);add(&s.actor->position,&delta);
    bh_mat34_translate_world(delta.x,delta.y,delta.z,&a.local_to_world);
    bh_mat34_translate_local(-delta.x,-delta.y,-delta.z,&a.world_to_local);
    bh_script_motion_actor_write(s.actor,&a);
    publish(c,s.actor);
    for(p=attachments(c,s.actor,a.attachments);p;p=bh_script_motion_attachment_next(p)){add(&p->position,&delta);publish(c,p);}
    if(expire(e,s.countdown)){
        bh_round_float(&s.actor->origin.x,(double)s.actor->origin.x-delta.x);
        bh_round_float(&s.actor->origin.y,(double)s.actor->origin.y-delta.y);
        bh_round_float(&s.actor->origin.z,(double)s.actor->origin.z-delta.z);
        bh_entity_release(c->lifetime->timers.pool,e);
    }
}
/* Exact PE doubles00489498..004894b0, not float approximations and not fmod.
   One correction only. The low-side C0 branch includes unordered. */
static void wrap(float *v){
    if((double)*v>3.141592653589793)bh_round_float(v,(double)*v-6.283185307179586);
    else if(!((double)*v>=-3.141592653589793))bh_round_float(v,(double)*v-(-6.283185307179586));
}
void __cdecl bh_script_rotate_tick(BHEntity *e){BHScriptMotionContext *c=context(e);BHScriptLifetimeState s;BHVec3 angles;
    bh_script_lifetime_read(e,&s);if(!valid(c,e,&s,1))return;
    get(s.actor,0x3c,&angles,12);add(&angles,&s.delta);wrap(&angles.x);wrap(&angles.y);wrap(&angles.z);
    put(s.actor,0x3c,&angles,12);publish(c,s.actor);if(expire(e,s.countdown))bh_entity_release(c->lifetime->timers.pool,e);
}
