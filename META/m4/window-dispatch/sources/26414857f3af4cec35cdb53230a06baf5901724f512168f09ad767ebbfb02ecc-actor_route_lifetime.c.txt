#include "../src/bh_actor_route_lifetime.h"
#include "../src/bh_fp.h"
#include <stdio.h>
#include <string.h>
static unsigned long checks,failures,query_calls;
#define CHECK(x,s) do {++checks;if(!(x)){++failures;printf("FAIL actor-route-lifetime: %s\n",s);}} while(0)
static void link(BHEntity *a,unsigned long offset,BHEntity *p){memcpy((unsigned char*)a+offset,&p,4);}
static BHEntity *linked(BHEntity *a,unsigned long offset){BHEntity *p;memcpy(&p,(unsigned char*)a+offset,4);return p;}
typedef struct Fixture {BHWorldGeometry worlds[3];BHGeometryModel model;BHPickupRoom rooms[3];BHActorRouteContext context;BHActorRouteState state;} Fixture;
static void fixture(Fixture *f,BHEntity *actor) {
    unsigned long i;long kind=2;BHPortal *p;memset(f,0,sizeof(*f));f->model.portal_count=2;
    for(i=0;i<3;++i){f->rooms[i].world=f->worlds+i;f->worlds[i].model=&f->model;f->worlds[i].navigation.count=2;}
    f->worlds[1].minimum.x=0;f->worlds[1].maximum.x=100;f->worlds[1].query_flags=8;
    for(i=0;i<2;++i){p=f->worlds[1].navigation.portals+i;p->direction=i?0:4;p->plane=i?100:0;
        p->scratch[0]=p->scratch[2]=-20;p->scratch[1]=p->scratch[3]=20;p->neighbor=&f->worlds[i?2:0].navigation;}
    f->context.rooms=f->rooms;f->context.room_count=3;f->context.actor_limit=16;
    f->state.current=f->worlds;f->state.next=f->worlds+1;f->state.speed=10;f->state.speed_scale=1;
    f->state.u=f->state.v=.5;f->state.sound=BH_SOUND_INVALID;f->state.random=123;
    f->rooms[0].actors=actor;link(actor,0xf4,NULL);link(actor,0xf8,NULL);memcpy(actor->payload+24,&kind,4);
    bh_actor_route_write(actor,&f->state);
}
static void due(BHEntity *actor) {
    static const unsigned short cases[][3]={{100,90,11},{100,90,10},{100,90,9},{3,65530,5},
        {65530,65535,10},{0,0,32768},{0,0,32767},{32767,0,0},{32768,0,0},{65535,0,0},{0,65535,0}};
    unsigned long i;unsigned short delta,expected;Fixture f;BHEntity before;BHActorRouteState s;
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        fixture(&f,actor);f.context.tick=cases[i][0];f.state.tick=cases[i][1];f.state.duration_ticks=cases[i][2];
        bh_actor_route_write(actor,&f.state);before=*actor;delta=(unsigned short)(cases[i][0]-cases[i][1]-cases[i][2]);
        CHECK(bh_actor_route_due(&f.context,actor),"complete signed16 due callback");bh_actor_route_read(actor,&s);
        if(delta&0x8000)CHECK(!memcmp(actor,&before,sizeof(before)),"negative wrapped elapsed preserves every actor byte");
        else {
            expected=(unsigned short)(cases[i][0]+delta);
            CHECK(s.current==f.worlds+1 && s.next==f.worlds+2 && s.tick==expected && s.duration_ticks==600,
                "one real route advance uses old elapsed then reloaded tick, preserving wrap/overshoot");
        }
    }
    fixture(&f,actor);f.context.tick=10;f.state.duration_ticks=1;bh_actor_route_write(actor,&f.state);
    f.worlds[1].navigation.portals[0].neighbor=f.worlds[1].navigation.portals[1].neighbor=NULL;
    /* The remaining portal is excluded, so source promotion fails explicitly. */
    CHECK(!bh_actor_route_due(&f.context,actor) && f.context.error==BH_ACTOR_ROUTE_TOPOLOGY,"failed real advance propagates source guard without fabricated completion");
    CHECK(!bh_actor_route_due(NULL,actor) && !bh_actor_route_due(&f.context,NULL),"due callback null guards");
}
static void __cdecl observed_query(BHEntity *entity,void *packet){++query_calls;}
static void release_position(unsigned long position) {
    BHEntityPool pool;BHEntity *nodes[3],*actor,*remaining[2];unsigned long i,j=0,generation;Fixture f;
    CHECK(bh_entities_init(&pool,3,NULL),"real cleanup pool initialized");
    for(i=0;i<3;++i)nodes[i]=bh_entity_acquire(&pool);actor=nodes[position];fixture(&f,actor);f.rooms[0].actors=nodes[0];
    for(i=0;i<3;++i){link(nodes[i],0xf8,i?nodes[i-1]:NULL);link(nodes[i],0xf4,i<2?nodes[i+1]:NULL);if(i!=position)remaining[j++]=nodes[i];}
    query_calls=0;CHECK(bh_entity_set_query(&pool,actor,observed_query) && bh_entities_focus(&pool,actor),"real query/focus lifetime bound");
    generation=actor->generation;
    CHECK(bh_actor_route_release(&f.context,&pool,actor),"complete cleanup at each original list position");
    CHECK(f.rooms[0].actors==remaining[0] && linked(remaining[0],0xf8)==NULL && linked(remaining[0],0xf4)==remaining[1] &&
        linked(remaining[1],0xf8)==remaining[0] && linked(remaining[1],0xf4)==NULL,"actual f4/f8 room list/head repaired before pool release");
    CHECK(pool.used==2 && pool.free_slots==actor && actor->generation==generation+1 && actor->query==NULL &&
        pool.query_head==NULL && !pool.focused && pool.focus==NULL && query_calls==0 && bh_entities_valid(&pool),
        "actual pool release updates lifetime,query,focus and allocator links without invoking a fabricated callback");
    CHECK(!bh_actor_route_release(&f.context,&pool,actor),"released actor cannot be cleaned up twice");
    CHECK(bh_entities_dispose(&pool),"real cleanup pool disposal");
}
static void release_sound(void) {
    BHEntityPool pool;BHEntity *actor;Fixture f;BHSoundQueue sound;BHSoundResource *resource;BHSoundVoice *voice;
    unsigned long handle;unsigned short generation;BHActorRouteState retained;
    CHECK(bh_entities_init(&pool,1,NULL) && bh_sound_queue_init(&sound,NULL,NULL,NULL),"real actor/sound cleanup owners");
    actor=bh_entity_acquire(&pool);fixture(&f,actor);resource=bh_sound_resource_allocate(&sound);CHECK(resource!=NULL,"real cleanup sound resource");
    if(resource){resource->streaming=1;resource->base_frequency=44100;handle=bh_sound_request(&sound,resource->index,1,1,NULL,NULL);voice=bh_sound_voice(&sound,handle);
        CHECK(voice!=NULL,"real cleanup voice request without playback");
        if(voice){generation=voice->generation;f.context.sound=&sound;f.state.sound=handle;f.state.sound_frames=99;bh_actor_route_write(actor,&f.state);
            CHECK(bh_actor_route_release(&f.context,&pool,actor),"cleanup calls actual sound stop regardless of countdown then releases actor");
            bh_actor_route_read(actor,&retained);
            CHECK(f.rooms[0].actors==NULL && pool.used==0 && resource->stream_head==NULL && voice->generation==(unsigned short)(generation+1) &&
                bh_sound_voice(&sound,handle)==NULL && retained.sound==handle && retained.sound_frames==99,
                "real voice/actor lifetimes end but original retained payload/handle bytes are not cleared");}}
    CHECK(bh_sound_queue_dispose(&sound) && bh_entities_dispose(&pool),"real cleanup owner disposal");
}
int bh_actor_route_lifetime_checkpoint(unsigned long *count) {
    BHEntityPool pool;BHEntity *actor;BHMathEnvironment env;unsigned long i;Fixture f;BHEntity before;
    checks=failures=0;CHECK(bh_math_environment_enter(&env),"real numerical environment");
    CHECK(bh_entities_init(&pool,1,NULL),"real due callback actor owner");actor=bh_entity_acquire(&pool);CHECK(actor!=NULL,"actual due actor acquired");
    if(actor){due(actor);fixture(&f,actor);before=*actor;f.state.sound=123;bh_actor_route_write(actor,&f.state);before=*actor;
        CHECK(!bh_actor_route_release(&f.context,&pool,actor) && !memcmp(actor,&before,sizeof(before)) && f.rooms[0].actors==actor && pool.used==1,
            "missing actual sound binding guard preserves list/pool/actor");}
    CHECK(bh_entities_dispose(&pool),"real due callback actor disposal");
    for(i=0;i<3;++i)release_position(i);release_sound();bh_math_environment_leave(&env);if(count)*count=checks;
    printf("{\"scenario\":\"actor-route-lifetime\",\"checks\":%lu,\"failures\":%lu,\"original_code_required\":false}\n",checks,failures);
    return failures!=0;
}
#ifdef BH_ACTOR_ROUTE_LIFETIME_STANDALONE
int main(void) {return bh_actor_route_lifetime_checkpoint(NULL);}
#endif
