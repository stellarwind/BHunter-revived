#include "bh_script_attached_sound.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
typedef char field_layout[sizeof(void*)==4 && offsetof(BHEntity,payload)==0xe4?1:-1];
static void get(BHEntity *e,unsigned long at,void *out){memcpy(out,(unsigned char*)e+at,4);}
static void put(BHEntity *e,unsigned long at,const void *value){memcpy((unsigned char*)e+at,value,4);}
static BHScriptLifetimeContext *context(BHEntity *e){BHScriptLifetimeContext *c;memcpy(&c,e->payload+400,4);return c;}
static const BHScriptTimerActor *actor_record(BHScriptLifetimeContext *c,BHEntity *actor){
    unsigned long i;for(i=0;i<c->timers.actor_count;++i)if(c->timers.actors[i].entity==actor)return c->timers.actors+i;return NULL;
}
int __cdecl bh_script_attached_sound_read(BHEntity *e,BHScriptAttachedSoundState *s){
    if(!e||!s)return 0;get(e,0xf0,&s->actor);get(e,0xf4,&s->script_loaded);
    get(e,0xfc,&s->handle);get(e,0x100,&s->resource);get(e,0x104,&s->level);get(e,0x108,&s->target);return 1;
}
int __cdecl bh_script_attached_sound_bind(BHScriptLifetimeContext *c,BHEntity *e,const BHScriptAttachedSoundState *s){
    BHScriptLifetimeState base;
    if(!c||!e||!s||!c->sounds||!actor_record(c,s->actor))return 0;
    /* Reuse the persistent real context/query installer, retaining all fields
       outside its specified binding footprint by restoring its unused writes. */
    {
        unsigned char retained[24];
        memcpy(retained,(unsigned char*)e+0x128,4);memcpy(retained+4,(unsigned char*)e+0x130,12);memcpy(retained+16,(unsigned char*)e+0x180,4);
        memset(&base,0,sizeof(base));base.actor=s->actor;base.script_loaded=s->script_loaded;base.sound=s->handle;
        if(!bh_script_lifetime_bind(c,e,&base,bh_script_sound_actor_query))return 0;
        memcpy((unsigned char*)e+0x128,retained,4);memcpy((unsigned char*)e+0x130,retained+4,12);memcpy((unsigned char*)e+0x180,retained+16,4);
    }
    put(e,0x100,&s->resource);put(e,0x104,&s->level);put(e,0x108,&s->target);e->step=bh_script_attached_sound_tick;return 1;
}
void __cdecl bh_script_attached_sound_tick(BHEntity *e){
    BHScriptAttachedSoundState s;BHScriptLifetimeContext *c=context(e);const BHScriptTimerActor *actor;
    BHVec3 zero={0,0,0};float gain;unsigned long next;
    bh_script_attached_sound_read(e,&s);actor=actor_record(c,s.actor);
    if(!actor){c->timers.error=BH_TIMER_ACTOR;return;}
    if(actor->owner)actor=actor->owner;
    if(s.level<s.target){
        next=(unsigned long)s.level+1UL;memcpy(&s.level,&next,4);put(e,0x104,&s.level);
        if(s.level==1){
            s.handle=bh_sound_request(c->sounds,s.resource,0.066666670143604278564453125f,1,&s.actor->position,&zero);
            put(e,0xfc,&s.handle);
        }else{
            bh_round_float(&gain,(double)s.level*0.06666666666666666574148081281236955);
            bh_sound_update(c->sounds,s.handle,gain,1,&s.actor->position,&zero);
        }
    }else if(s.level>s.target){
        next=(unsigned long)s.level-1UL;memcpy(&s.level,&next,4);put(e,0x104,&s.level);
        if(s.level==0){bh_sound_stop(c->sounds,s.handle);s.handle=BH_SOUND_INVALID;put(e,0xfc,&s.handle);}
        else{
            bh_round_float(&gain,(double)s.level*0.06666666666666666574148081281236955);
            bh_sound_update(c->sounds,s.handle,gain,1,&s.actor->position,&zero);
        }
    }
    if(s.handle==BH_SOUND_INVALID){if(actor->active){s.target=15;put(e,0x108,&s.target);}}
    else if(!actor->active){s.target=0;put(e,0x108,&s.target);}
}
